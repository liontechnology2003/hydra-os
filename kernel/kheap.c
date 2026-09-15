#include "kheap.h"
#include "pmm.h"
#include "paging.h"
#include "string.h"
#include "serial.h"

#define KHEAP_START     0x02000000
#define KHEAP_END       0x08000000
#define KHEAP_BLOCK_MIN 16
#define KHEAP_BLOCK_MAGIC 0xDEADBEEF

typedef struct block_header {
    uint32 magic;
    uint32 size;
    uint32 used;
    struct block_header *next;
    struct block_header *prev;
} block_header_t;

static block_header_t *heap_head = 0;
static uint32 heap_start = KHEAP_START;
static uint32 heap_current = KHEAP_START;
static uint32 heap_used = 0;
static uint32 heap_free = 0;

static void extend_heap(uint32 size)
{
    uint32 frames = (size + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;
    uint32 i;

    for (i = 0; i < frames; i++) {
        if (heap_current + i * PMM_FRAME_SIZE >= KHEAP_END) {
            serial_write("KHEAP: out of address space\n", 28);
            return;
        }
        uint32 frame = pmm_alloc_frame();
        if (!frame) {
            serial_write("KHEAP: out of physical memory\n", 30);
            return;
        }
        paging_map_page(heap_current + i * PMM_FRAME_SIZE, frame,
                        PAGE_PRESENT | PAGE_WRITABLE);
    }
    heap_current += frames * PMM_FRAME_SIZE;
    heap_free += frames * PMM_FRAME_SIZE;
}

void kheap_init(void)
{
    /* Extend by initial 64KB */
    extend_heap(64 * 1024);
    serial_write("KHEAP: initialized\n", 19);
}

void *kmalloc(uint32 size)
{
    block_header_t *blk;
    block_header_t *new_blk;
    uint32 total;

    if (size == 0) return 0;

    total = size + sizeof(block_header_t);
    if (total < KHEAP_BLOCK_MIN + sizeof(block_header_t)) {
        total = KHEAP_BLOCK_MIN + sizeof(block_header_t);
    }

    /* First-fit search */
    blk = heap_head;
    while (blk) {
        if (!blk->used && blk->size >= total) {
            /* Split block if large enough */
            if (blk->size >= total + sizeof(block_header_t) + KHEAP_BLOCK_MIN) {
                new_blk = (block_header_t *)((uint32)blk + total);
                new_blk->magic = KHEAP_BLOCK_MAGIC;
                new_blk->size = blk->size - total;
                new_blk->used = 0;
                new_blk->next = blk->next;
                new_blk->prev = blk;
                if (blk->next) blk->next->prev = new_blk;
                blk->next = new_blk;
                blk->size = total;
            }
            blk->used = 1;
            heap_used += blk->size;
            heap_free -= blk->size;
            return (void *)((uint32)blk + sizeof(block_header_t));
        }
        blk = blk->next;
    }

    /* No free block found, extend heap */
    extend_heap(total);

    /* Allocate at end of heap */
    blk = (block_header_t *)heap_head;
    if (!blk) {
        heap_head = (block_header_t *)heap_start;
        blk = heap_head;
    } else {
        while (blk->next) blk = blk->next;
        blk->next = (block_header_t *)((uint32)blk + blk->size);
        blk = blk->next;
    }

    blk->magic = KHEAP_BLOCK_MAGIC;
    blk->size = total;
    blk->used = 1;
    blk->next = 0;
    blk->prev = (block_header_t *)((uint32)blk - sizeof(block_header_t));

    /* Fix prev pointer of previous block */
    if (blk->prev && blk->prev->magic != KHEAP_BLOCK_MAGIC) {
        blk->prev = 0;
    }

    heap_used += total;
    return (void *)((uint32)blk + sizeof(block_header_t));
}

void kfree(void *ptr)
{
    block_header_t *blk;

    if (!ptr) return;

    blk = (block_header_t *)((uint32)ptr - sizeof(block_header_t));

    if (blk->magic != KHEAP_BLOCK_MAGIC) {
        serial_write("KHEAP: invalid free\n", 20);
        return;
    }

    blk->used = 0;
    heap_used -= blk->size;
    heap_free += blk->size;

    /* Merge with next block */
    if (blk->next && !blk->next->used) {
        blk->size += blk->next->size;
        blk->next = blk->next->next;
        if (blk->next) blk->next->prev = blk;
    }

    /* Merge with previous block */
    if (blk->prev && !blk->prev->used) {
        blk->prev->size += blk->size;
        blk->prev->next = blk->next;
        if (blk->next) blk->next->prev = blk->prev;
    }
}

void *kcalloc(uint32 num, uint32 size)
{
    uint32 total = num * size;
    void *ptr = kmalloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *krealloc(void *ptr, uint32 new_size)
{
    block_header_t *blk;
    void *new_ptr;

    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) { kfree(ptr); return 0; }

    blk = (block_header_t *)((uint32)ptr - sizeof(block_header_t));
    if (blk->magic != KHEAP_BLOCK_MAGIC) {
        return kmalloc(new_size);
    }

    uint32 data_size = blk->size - sizeof(block_header_t);
    if (new_size <= data_size) {
        return ptr;
    }

    new_ptr = kmalloc(new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, data_size);
        kfree(ptr);
    }
    return new_ptr;
}

void *kmalloc_aligned(uint32 size)
{
    uint32 addr = (uint32)kmalloc(size + 0x1000);
    if (!addr) return 0;
    uint32 aligned = (addr + 0xFFF) & ~0xFFF;
    return (void *)aligned;
}

uint32 kheap_get_used(void)
{
    return heap_used;
}

uint32 kheap_get_free(void)
{
    return heap_free;
}
