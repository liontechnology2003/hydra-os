#include "pmm.h"
#include "string.h"

#define PMM_MAX_FRAMES  131072
#define PMM_BITMAP_SIZE (PMM_MAX_FRAMES / 32)

extern uint32 kernel_start;
extern uint32 kernel_end;

static uint32 pmm_bitmap[PMM_BITMAP_SIZE];
static uint32 pmm_total_frames = 0;
static uint32 pmm_used_frames = 0;

static inline void bitmap_set(uint32 frame)
{
    pmm_bitmap[frame / 32] |= (1U << (frame % 32));
}

static inline void bitmap_clear(uint32 frame)
{
    pmm_bitmap[frame / 32] &= ~(1U << (frame % 32));
}

static inline int bitmap_test(uint32 frame)
{
    return pmm_bitmap[frame / 32] & (1U << (frame % 32));
}

void pmm_init(uint32 total_mem_kb)
{
    uint32 i;
    uint32 total_frames;
    uint32 kernel_start_frame;
    uint32 kernel_end_frame;

    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));

    total_frames = (total_mem_kb * 1024) / PMM_FRAME_SIZE;
    if (total_frames > PMM_MAX_FRAMES) {
        total_frames = PMM_MAX_FRAMES;
    }
    pmm_total_frames = total_frames;

    for (i = 0; i < total_frames; i++) {
        bitmap_clear(i);
    }

    /* Mark first frame as used (real mode IVT / BDA) */
    bitmap_set(0);

    /* Mark frames occupied by kernel */
    kernel_start_frame = ((uint32)&kernel_start) / PMM_FRAME_SIZE;
    kernel_end_frame   = ((uint32)&kernel_end + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;

    for (i = kernel_start_frame; i < kernel_end_frame; i++) {
        bitmap_set(i);
    }

    pmm_used_frames = kernel_end_frame;

    /* Mark first 1MB as reserved (VGA, BIOS, etc.) */
    for (i = 0; i < 256; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            pmm_used_frames++;
        }
    }
}

uint32 pmm_alloc_frame(void)
{
    uint32 i;
    uint32 j;

    for (i = 0; i < PMM_BITMAP_SIZE; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            for (j = 0; j < 32; j++) {
                if (!(pmm_bitmap[i] & (1U << j))) {
                    uint32 frame = i * 32 + j;
                    if (frame >= pmm_total_frames) {
                        return 0;
                    }
                    bitmap_set(frame);
                    pmm_used_frames++;
                    return frame * PMM_FRAME_SIZE;
                }
            }
        }
    }
    return 0;
}

void pmm_free_frame(uint32 frame_addr)
{
    uint32 frame = frame_addr / PMM_FRAME_SIZE;
    if (frame < pmm_total_frames && bitmap_test(frame)) {
        bitmap_clear(frame);
        pmm_used_frames--;
    }
}

uint32 pmm_alloc_frames(uint32 count)
{
    uint32 i;
    uint32 j;
    uint32 consecutive;

    for (i = 0; i < pmm_total_frames - count; i++) {
        if (!bitmap_test(i)) {
            consecutive = 1;
            for (j = 1; j < count; j++) {
                if (bitmap_test(i + j)) {
                    break;
                }
                consecutive++;
            }
            if (consecutive == count) {
                for (j = 0; j < count; j++) {
                    bitmap_set(i + j);
                }
                pmm_used_frames += count;
                return i * PMM_FRAME_SIZE;
            }
        }
    }
    return 0;
}

void pmm_free_frames(uint32 base, uint32 count)
{
    uint32 i;
    uint32 frame = base / PMM_FRAME_SIZE;

    for (i = 0; i < count; i++) {
        if (bitmap_test(frame + i)) {
            bitmap_clear(frame + i);
            pmm_used_frames--;
        }
    }
}

uint32 pmm_get_free_count(void)
{
    return pmm_total_frames - pmm_used_frames;
}

uint32 pmm_get_total_count(void)
{
    return pmm_total_frames;
}
