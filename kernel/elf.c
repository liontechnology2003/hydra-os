#include "elf.h"
#include "process.h"
#include "paging.h"
#include "pmm.h"
#include "kheap.h"
#include "string.h"
#include "serial.h"

/* Embedded test ELF binary (from user_hello_embed.c) */
extern const uint8 user_hello_elf[];
extern const uint32 user_hello_elf_size;

/* Ramdisk: simple linked list of named binary blobs in kernel memory */
#define RAMDISK_MAX_FILES 32
#define RAMDISK_NAME_MAX  64

typedef struct {
    char    name[RAMDISK_NAME_MAX];
    uint32  data;
    uint32  size;
} ramdisk_file_t;

static ramdisk_file_t ramdisk_files[RAMDISK_MAX_FILES];
static uint32 ramdisk_file_count = 0;

void elf_set_ramdisk(uint32 addr, uint32 size)
{
    /* For now, the ramdisk is just a raw binary blob.
     * We don't parse it yet — files are added via ramdisk_add_file(). */
    (void)addr;
    (void)size;
}

int ramdisk_add_file(const char *name, const uint8 *data, uint32 size)
{
    if (ramdisk_file_count >= RAMDISK_MAX_FILES) return -1;

    ramdisk_file_t *f = &ramdisk_files[ramdisk_file_count];
    strncpy(f->name, name, RAMDISK_NAME_MAX - 1);
    f->name[RAMDISK_NAME_MAX - 1] = '\0';

    /* Copy data into kernel heap */
    f->data = (uint32)kmalloc(size);
    if (!f->data) return -1;
    memcpy((void *)f->data, data, size);
    f->size = size;

    ramdisk_file_count++;
    return 0;
}

static const ramdisk_file_t *ramdisk_find(const char *name)
{
    uint32 i;
    for (i = 0; i < ramdisk_file_count; i++) {
        if (strcmp(ramdisk_files[i].name, name) == 0) {
            return &ramdisk_files[i];
        }
    }
    return 0;
}

int elf_load(const char *name, const uint8 *data, uint32 size)
{
    elf32_ehdr *ehdr;
    elf32_phdr *phdr;
    uint32 i;

    if (size < sizeof(elf32_ehdr)) {
        serial_write("ELF: file too small\n", 20);
        return -1;
    }

    ehdr = (elf32_ehdr *)data;

    /* Verify ELF magic */
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        serial_write("ELF: bad magic\n", 15);
        return -1;
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        serial_write("ELF: not 32-bit\n", 17);
        return -1;
    }

    if (ehdr->e_type != ET_EXEC) {
        serial_write("ELF: not executable\n", 20);
        return -1;
    }

    /* Create a new process for this ELF */
    int pid = process_create(name, (void *)ehdr->e_entry);
    if (pid < 0) {
        serial_write("ELF: process_create failed\n", 27);
        return -1;
    }

    /* Parse program headers */
    phdr = (elf32_phdr *)(data + ehdr->e_phoff);

    serial_write("ELF: loading '", 14);
    serial_write((char *)name, strlen(name));
    serial_write("' entry=0x", 10);
    {
        char buf[16];
        itoa(buf, ehdr->e_entry);
        serial_write(buf, strlen(buf));
    }
    serial_write("\n", 1);

    for (i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) continue;

        uint32 vaddr = phdr[i].p_vaddr;
        uint32 memsz = phdr[i].p_memsz;
        uint32 filesz = phdr[i].p_filesz;
        uint32 offset = phdr[i].p_offset;
        uint32 flags = phdr[i].p_flags;

        if (memsz == 0) continue;

        serial_write("ELF:   segment vaddr=0x", 23);
        {
            char buf[16];
            itoa(buf, vaddr);
            serial_write(buf, strlen(buf));
        }
        serial_write(" filesz=", 8);
        {
            char buf[16];
            itoa(buf, filesz);
            serial_write(buf, strlen(buf));
        }
        serial_write(" memsz=", 7);
        {
            char buf[16];
            itoa(buf, memsz);
            serial_write(buf, strlen(buf));
        }
        serial_write("\n", 1);

        /* Round down vaddr to page boundary */
        uint32 page_start = vaddr & ~0xFFF;
        uint32 page_end = (vaddr + memsz + 0xFFF) & ~0xFFF;
        uint32 num_pages = (page_end - page_start) / 0x1000;

        /* Allocate physical frames and map into virtual address space */
        uint32 j;
        for (j = 0; j < num_pages; j++) {
            uint32 phys = pmm_alloc_frame();
            if (!phys) {
                serial_write("ELF: OOM\n", 9);
                /* TODO: cleanup */
                return -1;
            }
            memset((void *)phys, 0, 0x1000);

            uint32 virt = page_start + j * 0x1000;
            uint32 pg_flags = PAGE_PRESENT | PAGE_USER;
            if (flags & PF_W) pg_flags |= PAGE_WRITABLE;

            paging_map_page(virt, phys, pg_flags);
        }

        /* Copy segment data from ELF image */
        if (filesz > 0 && offset + filesz <= size) {
            memcpy((void *)vaddr, data + offset, filesz);
        }

        /* Zero remaining memory (bss) */
        if (memsz > filesz) {
            memset((void *)(vaddr + filesz), 0, memsz - filesz);
        }
    }

    serial_write("ELF: loaded successfully, pid=", 30);
    {
        char buf[8];
        itoa(buf, pid);
        serial_write(buf, strlen(buf));
    }
    serial_write("\n", 1);

    return pid;
}

int elf_load_from_ramdisk(const char *name, const char *path)
{
    const ramdisk_file_t *f = ramdisk_find(path);
    if (!f) {
        serial_write("ELF: file not found: ", 21);
        serial_write((char *)path, strlen(path));
        serial_write("\n", 1);
        return -1;
    }
    return elf_load(name, (const uint8 *)f->data, f->size);
}
