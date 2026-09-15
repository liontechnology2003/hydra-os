#include "vbe.h"
#include "serial.h"
#include "io.h"
#include "paging.h"

static vbe_info_t vbe_info;
static int vbe_active = 0;

static void dispi_write(uint16 index, uint16 value)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}

static uint16 dispi_read(uint16 index)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

static uint32 pci_config_read32(uint8 bus, uint8 dev, uint8 func, uint8 offset)
{
    uint32 addr = (1u << 31)
                | ((uint32)bus << 16)
                | ((uint32)dev << 11)
                | ((uint32)func << 8)
                | (offset & 0xFC);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

static uint16 pci_config_read16(uint8 bus, uint8 dev, uint8 func, uint8 offset)
{
    uint32 val = pci_config_read32(bus, dev, func, offset & ~3);
    return (uint16)((val >> ((offset & 2) * 8)) & 0xFFFF);
}

static uint8 pci_config_read8(uint8 bus, uint8 dev, uint8 func, uint8 offset)
{
    uint32 val = pci_config_read32(bus, dev, func, offset & ~3);
    return (uint8)((val >> ((offset & 3) * 8)) & 0xFF);
}

int vbe_init(uint32 multiboot_info_addr)
{
    uint16 id;
    uint32 bar0;
    uint32 lfb_phys;
    uint32 lfb_virt;
    uint8 bus, dev;

    (void)multiboot_info_addr;

    /* Check if DISPI interface is present */
    id = dispi_read(VBE_DISPI_INDEX_ID);
    serial_write("VBE: DISPI id=0x", 16);
    serial_write_hex(id);
    serial_write("\n", 1);

    if (id != 0xB0C5 && id != 0xB0C4 && id != 0xB0C3 && id != 0xB0C2) {
        serial_write("VBE: DISPI not found\n", 22);
        return -1;
    }

    /* Scan PCI bus 0 for VGA-compatible display controller (class 0x03, subclass 0x00) */
    lfb_phys = 0;
    for (bus = 0; bus < 2; bus++) {
        for (dev = 0; dev < 32; dev++) {
            uint16 vendor = pci_config_read16(bus, dev, 0, 0x00);
            if (vendor == 0xFFFF) continue;

            uint8 class = pci_config_read8(bus, dev, 0, 0x0B);
            uint8 subclass = pci_config_read8(bus, dev, 0, 0x0A);

            if (class == 0x03) {
                serial_write("VBE: PCI VGA at ", 16);
                serial_write_int_dec(bus);
                serial_write(":", 1);
                serial_write_int_dec(dev);
                serial_write(" class=", 7);
                serial_write_hex(class);
                serial_write(" sub=", 5);
                serial_write_hex(subclass);
                serial_write("\n", 1);

                /* BAR0 is the LFB address (memory-mapped, bits [31:4] are address, bit [0] is 0 for memory) */
                bar0 = pci_config_read32(bus, dev, 0, 0x10);
                if (bar0 != 0 && bar0 != 0xFFFFFFFF) {
                    lfb_phys = bar0 & 0xFFFFFFF0;
                    serial_write("VBE: BAR0=0x", 12);
                    serial_write_hex(bar0);
                    serial_write(" LFB=0x", 8);
                    serial_write_hex(lfb_phys);
                    serial_write("\n", 1);
                }
                break;
            }
        }
        if (lfb_phys) break;
    }

    if (!lfb_phys) {
        serial_write("VBE: no PCI VGA found, trying 0xFD000000\n", 43);
        lfb_phys = 0xFD000000;
    }

    /* Set mode via DISPI: 1024x768x32 */
    dispi_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    dispi_write(VBE_DISPI_INDEX_XRES, 1024);
    dispi_write(VBE_DISPI_INDEX_YRES, 768);
    dispi_write(VBE_DISPI_INDEX_BPP, 32);
    dispi_write(VBE_DISPI_INDEX_VIRT_WIDTH, 1024);
    dispi_write(VBE_DISPI_INDEX_VIRT_HEIGHT, 768);
    dispi_write(VBE_DISPI_INDEX_X_OFFSET, 0);
    dispi_write(VBE_DISPI_INDEX_Y_OFFSET, 0);
    dispi_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    uint16 xres = dispi_read(VBE_DISPI_INDEX_XRES);
    uint16 yres = dispi_read(VBE_DISPI_INDEX_YRES);
    if (xres != 1024 || yres != 768) {
        serial_write("VBE: mode set failed (got ", 26);
        serial_write_int_dec(xres);
        serial_write("x", 1);
        serial_write_int_dec(yres);
        serial_write(")\n", 2);
        return -1;
    }

    /* Map LFB into kernel virtual space at a safe address.
     * The LFB can be up to 1024*768*4 = 3145728 bytes (3MB).
     * Map at 0xF0000000 which is in kernel space and unlikely to conflict. */
    lfb_virt = 0xF0000000;
    uint32 lfb_size = 1024 * 768 * 4;
    uint32 lfb_pages = (lfb_size + 0xFFF) / 0x1000;
    if (lfb_pages < 1) lfb_pages = 1;

    serial_write("VBE: mapping LFB 0x", 19);
    serial_write_hex(lfb_phys);
    serial_write(" -> 0x", 7);
    serial_write_hex(lfb_virt);
    serial_write(" (", 2);
    serial_write_int_dec(lfb_pages);
    serial_write(" pages)\n", 8);

    /* Temporarily identity-map the physical address to test it works */
    for (uint32 i = 0; i < lfb_pages; i++) {
        paging_map_page(lfb_virt + i * 0x1000, lfb_phys + i * 0x1000,
                        PAGE_PRESENT | PAGE_WRITABLE);
    }

    vbe_info.address  = (uint8 *)lfb_virt;
    vbe_info.phys     = lfb_phys;
    vbe_info.width    = 1024;
    vbe_info.height   = 768;
    vbe_info.bpp      = 32;
    vbe_info.pitch    = 1024 * 4;
    vbe_info.size     = 1024 * 768 * 4;
    vbe_active = 1;

    serial_write("VBE: 1024x768x32 OK\n", 21);

    return 0;
}

vbe_info_t *vbe_get_info(void)
{
    return &vbe_info;
}

int vbe_is_active(void)
{
    return vbe_active;
}

uint32 vbe_get_lfb_physical(void)
{
    return vbe_info.phys;
}
