#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include "types.h"

/* ELF Identification */
#define EI_MAG0    0
#define EI_MAG1    1
#define EI_MAG2    2
#define EI_MAG3    3
#define EI_CLASS   4
#define EI_DATA    5
#define EI_VERSION 6
#define EI_OSABI   7
#define EI_PAD     8

#define ELFMAG0    0x7F
#define ELFMAG1    'E'
#define ELFMAG2    'L'
#define ELFMAG3    'F'

#define ELFCLASS32 1
#define ELFDATA2LSB 1

/* ELF Header */
typedef struct {
    uint8  e_ident[16];
    uint16 e_type;
    uint16 e_machine;
    uint32 e_version;
    uint32 e_entry;
    uint32 e_phoff;
    uint32 e_shoff;
    uint32 e_flags;
    uint16 e_ehsize;
    uint16 e_phentsize;
    uint16 e_phnum;
    uint16 e_shentsize;
    uint16 e_shnum;
    uint16 e_shstrndx;
} __attribute__((packed)) elf32_ehdr;

/* ELF Program Header */
typedef struct {
    uint32 p_type;
    uint32 p_offset;
    uint32 p_vaddr;
    uint32 p_paddr;
    uint32 p_filesz;
    uint32 p_memsz;
    uint32 p_flags;
    uint32 p_align;
} __attribute__((packed)) elf32_phdr;

/* ELF Section Header */
typedef struct {
    uint32 sh_name;
    uint32 sh_type;
    uint32 sh_flags;
    uint32 sh_addr;
    uint32 sh_offset;
    uint32 sh_size;
    uint32 sh_link;
    uint32 sh_info;
    uint32 sh_addralign;
    uint32 sh_entsize;
} __attribute__((packed)) elf32_shdr;

/* Program header types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4

/* Program header flags */
#define PF_X       0x1
#define PF_W       0x2
#define PF_R       0x4

/* ELF types */
#define ET_NONE    0
#define ET_REL     1
#define ET_EXEC    2
#define ET_DYN     3

/* Load an ELF binary from a memory buffer.
 * Creates a new process with the ELF's entry point.
 * Returns the new process's PID, or -1 on error. */
int elf_load(const char *name, const uint8 *data, uint32 size);

/* Load an ELF binary from the ramdisk.
 * ramdisk_addr and ramdisk_size are set during boot. */
void elf_set_ramdisk(uint32 addr, uint32 size);
int  elf_load_from_ramdisk(const char *name, const char *path);

/* Add a file to the in-memory ramdisk */
int ramdisk_add_file(const char *name, const uint8 *data, uint32 size);

#endif /* KERNEL_ELF_H */
