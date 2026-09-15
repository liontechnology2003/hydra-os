#include "paging.h"
#include "pmm.h"
#include "string.h"
#include "serial.h"

extern uint32 kernel_start;
extern uint32 kernel_end;

static page_directory_t *kernel_directory = 0;
static page_directory_t *current_directory = 0;

static page_table_t *kernel_tables[1024] = {0};

static page_table_t *create_table(void)
{
    uint32 phys = pmm_alloc_frame();
    page_table_t *table;

    if (!phys) {
        return 0;
    }

    table = (page_table_t *)phys;
    memset(table, 0, sizeof(page_table_t));
    return table;
}

static void map_table_page(page_directory_t *dir, uint32 page_idx,
                           page_table_t *table, uint32 phys_table)
{
    (void)table;
    dir->entries[page_idx] = phys_table | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
}

void paging_init(uint32 total_mem_kb)
{
    uint32 i;
    uint32 total_pages;
    uint32 kernel_end_addr = (uint32)&kernel_end;
    uint32 frames_needed;

    (void)total_mem_kb;

    kernel_directory = (page_directory_t *)pmm_alloc_frame();
    memset(kernel_directory, 0, sizeof(page_directory_t));

    total_pages = (total_mem_kb * 1024) / PMM_FRAME_SIZE;
    if (total_pages > 131072) {
        total_pages = 131072;
    }

    /* Identity-map ALL physical RAM */
    for (i = 0; i < total_pages; i++) {
        uint32 addr = i * PMM_FRAME_SIZE;
        uint32 di = PAGE_DIR_INDEX(addr);
        uint32 ti = PAGE_TABLE_INDEX(addr);

        if (!kernel_tables[di]) {
            kernel_tables[di] = create_table();
            if (!kernel_tables[di]) {
                serial_write("PAGING: OOM for page tables\n", 29);
                break;
            }
            map_table_page(kernel_directory, di, kernel_tables[di],
                           (uint32)kernel_tables[di]);
        }
        kernel_tables[di]->entries[ti] = addr | PAGE_PRESENT | PAGE_WRITABLE;
    }

    /* Map kernel into higher-half: 0xC0000000+ -> physical 0x100000+ */
    frames_needed = (kernel_end_addr - 0x100000 + 0x1000) / 0x1000;
    if (frames_needed < 1) frames_needed = 1;

    for (i = 0; i <= frames_needed; i++) {
        uint32 phys_addr = 0x100000 + i * 0x1000;
        uint32 hh_virt = 0xC0000000 + i * 0x1000;
        uint32 di = PAGE_DIR_INDEX(hh_virt);
        uint32 ti = PAGE_TABLE_INDEX(hh_virt);

        if (!kernel_tables[di]) {
            kernel_tables[di] = create_table();
            if (!kernel_tables[di]) continue;
            map_table_page(kernel_directory, di, kernel_tables[di],
                           (uint32)kernel_tables[di]);
        }
        kernel_tables[di]->entries[ti] = phys_addr | PAGE_PRESENT | PAGE_WRITABLE;
    }

    current_directory = kernel_directory;

    serial_write("PAGING: page tables created\n", 28);
}

void paging_enable(page_directory_t *dir)
{
    uint32 cr0;
    uint32 dir_phys = (uint32)dir;

    __asm__ volatile ("mov %0, %%cr3" : : "r"(dir_phys));

    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
}

void paging_switch_directory(page_directory_t *dir)
{
    uint32 dir_phys = (uint32)dir;
    current_directory = dir;
    __asm__ volatile ("mov %0, %%cr3" : : "r"(dir_phys));
}

page_directory_t *paging_get_directory(void)
{
    return current_directory;
}

int paging_map_page(uint32 virt, uint32 phys, uint32 flags)
{
    return paging_map_page_in_dir(current_directory, virt, phys, flags);
}

int paging_map_page_in_dir(page_directory_t *dir, uint32 virt, uint32 phys, uint32 flags)
{
    uint32 di = PAGE_DIR_INDEX(virt);
    uint32 ti = PAGE_TABLE_INDEX(virt);

    if (!dir) {
        return -1;
    }

    page_table_t *table;
    if (!(dir->entries[di] & PAGE_PRESENT)) {
        /* Allocate new page table */
        uint32 table_phys = pmm_alloc_frame();
        if (!table_phys) {
            return -1;
        }
        table = (page_table_t *)table_phys;
        memset(table, 0, sizeof(page_table_t));
        dir->entries[di] = table_phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    } else {
        table = (page_table_t *)(dir->entries[di] & ~0xFFF);
    }

    table->entries[ti] = (phys & ~0xFFF) | (flags | PAGE_PRESENT);

    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");

    return 0;
}

int paging_unmap_page(uint32 virt)
{
    uint32 di = PAGE_DIR_INDEX(virt);
    uint32 ti = PAGE_TABLE_INDEX(virt);

    if (!kernel_tables[di]) {
        return -1;
    }

    kernel_tables[di]->entries[ti] = 0;
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
    return 0;
}

uint32 paging_get_physical(uint32 virt)
{
    uint32 di = PAGE_DIR_INDEX(virt);
    uint32 ti = PAGE_TABLE_INDEX(virt);
    uint32 entry;

    if (!kernel_tables[di]) {
        return 0;
    }

    entry = kernel_tables[di]->entries[ti];
    if (!(entry & PAGE_PRESENT)) {
        return 0;
    }
    return entry & ~0xFFF;
}

page_directory_t *paging_create_directory(void)
{
    page_directory_t *dir;
    uint32 i;

    dir = (page_directory_t *)pmm_alloc_frame();
    if (!dir) {
        return 0;
    }
    memset(dir, 0, sizeof(page_directory_t));

    for (i = 0; i < 1024; i++) {
        if (kernel_directory->entries[i]) {
            dir->entries[i] = kernel_directory->entries[i];
        }
    }

    return dir;
}

void paging_free_directory(page_directory_t *dir)
{
    if (dir && dir != kernel_directory) {
        pmm_free_frame((uint32)dir);
    }
}
