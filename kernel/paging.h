#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include "types.h"

#define PAGE_PRESENT   0x01
#define PAGE_WRITABLE  0x02
#define PAGE_USER      0x04
#define PAGE_SIZE      4096

#define PAGE_DIR_INDEX(x)  ((x) >> 22)
#define PAGE_TABLE_INDEX(x) ((x) >> 12 & 0x3FF)
#define PAGE_FRAME(x)      ((x) & ~0xFFF)

typedef uint32 page_entry;
typedef uint32 page_table_entry;

typedef struct {
    page_entry entries[1024];
} page_directory_t;

typedef struct {
    page_table_entry entries[1024];
} page_table_t;

void     paging_init(uint32 total_mem_kb);
void     paging_enable(page_directory_t *dir);
void     paging_switch_directory(page_directory_t *dir);
page_directory_t *paging_get_directory(void);

int      paging_map_page(uint32 virt, uint32 phys, uint32 flags);
int      paging_unmap_page(uint32 virt);
uint32   paging_get_physical(uint32 virt);

page_directory_t *paging_create_directory(void);
void     paging_free_directory(page_directory_t *dir);

#endif /* KERNEL_PAGING_H */
