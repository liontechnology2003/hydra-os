#ifndef KERNEL_KHEAP_H
#define KERNEL_KHEAP_H

#include "types.h"

void *kmalloc(uint32 size);
void *kmalloc_aligned(uint32 size);
void  kfree(void *ptr);
void *kcalloc(uint32 num, uint32 size);
void *krealloc(void *ptr, uint32 new_size);

void  kheap_init(void);
uint32 kheap_get_used(void);
uint32 kheap_get_free(void);

#endif /* KERNEL_KHEAP_H */
