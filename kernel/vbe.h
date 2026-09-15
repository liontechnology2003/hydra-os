#ifndef KERNEL_VBE_H
#define KERNEL_VBE_H

#include "types.h"

/* Bochs/QEMU VBE DISPI interface ports */
#define VBE_DISPI_INDEX_ID          0
#define VBE_DISPI_INDEX_XRES        1
#define VBE_DISPI_INDEX_YRES        2
#define VBE_DISPI_INDEX_BPP         3
#define VBE_DISPI_INDEX_ENABLE      4
#define VBE_DISPI_INDEX_BANK        5
#define VBE_DISPI_INDEX_VIRT_WIDTH  6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 7
#define VBE_DISPI_INDEX_X_OFFSET    8
#define VBE_DISPI_INDEX_Y_OFFSET    9
#define VBE_DISPI_INDEX_VIDEO_MEMORY 10

#define VBE_DISPI_IOPORT_INDEX      0x1CE
#define VBE_DISPI_IOPORT_DATA       0x1CF

#define VBE_DISPI_DISABLED          0x00
#define VBE_DISPI_ENABLED           0x01
#define VBE_DISPI_LFB_ENABLED       0x40

/* VBE framebuffer info */
typedef struct {
    uint8  *address;      /* LFB kernel virtual address */
    uint32 phys;          /* LFB physical address */
    uint32 pitch;         /* bytes per scanline */
    uint32 width;         /* pixels wide */
    uint32 height;        /* pixels tall */
    uint8  bpp;           /* bits per pixel */
    uint32 size;          /* total framebuffer size */
} vbe_info_t;

/* Set VBE mode via DISPI interface. Returns 0 on success. */
int vbe_set_mode(uint32 width, uint32 height, uint32 bpp);

/* Initialize VBE from multiboot (fallback if no mode set) */
int vbe_init(uint32 multiboot_info_addr);

/* Get the global VBE info */
vbe_info_t *vbe_get_info(void);

/* Check if VBE graphical mode is active */
int vbe_is_active(void);

/* Get the physical LFB address for user-space mapping */
uint32 vbe_get_lfb_physical(void);

#endif /* KERNEL_VBE_H */
