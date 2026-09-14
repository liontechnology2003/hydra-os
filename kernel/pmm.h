#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H

#include "types.h"

#define PMM_FRAME_SIZE  4096
#define PMM_FRAME_SHIFT 12

void     pmm_init(uint32 total_mem_kb);
uint32   pmm_alloc_frame(void);
void     pmm_free_frame(uint32 frame_addr);
uint32   pmm_alloc_frames(uint32 count);
void     pmm_free_frames(uint32 base, uint32 count);
uint32   pmm_get_free_count(void);
uint32   pmm_get_total_count(void);

#endif /* KERNEL_PMM_H */
