# RedLion OS — Full Qt Port Work Breakdown Structure

> **Goal:** Build a bare-metal i386 OS from scratch that runs Qt5 desktop applications  
> **Architecture:** i386 ELF, 1024x768x32 VBE framebuffer, FAT12 filesystem, custom compositor  
> **Total tasks:** 72 across 7 phases  
> **Estimated effort:** 6–11 weeks

---

## Phase 1: OS Foundations

> Fix critical kernel bugs and build ELF loading so we can run external programs.

- [ ] **1.1** Fix `process_context_switch` — save current regs before restoring next
- [ ] **1.2** Add preemptive scheduling — PIT IRQ0 calls `schedule()` with quantum
- [ ] **1.3** Fix `process_exit` — free kernel stack + page directory + user page tables
- [ ] **1.4** Add INT 14 page fault handler — kill faulting process with error info
- [ ] **1.5** Add `SYS_WAITPID` — reap zombie processes
- [ ] **1.6** ELF header structs — `elf32_ehdr`, `elf32_phdr`, constants in `kernel/elf.h`
- [ ] **1.7** ELF loader — `elf_load()` parse segments, alloc pages, map into user space
- [ ] **1.8** `SYS_EXEC` syscall — create process from ELF binary path
- [ ] **1.9** Ramdisk — embed binary blob at boot for testing ELF loading
- [ ] **1.10** Test — load and run a simple hello-world ELF from ramdisk

---

## Phase 2: Filesystem + Disk

> Add real disk I/O and a FAT12 filesystem so Qt apps and libraries can be loaded from disk.

- [ ] **2.1** ATA PIO driver — read/write sectors via ports 0x1F0–0x1F7
- [ ] **2.2** PCI enumeration — find ATA/AHCI controllers on PCI bus
- [ ] **2.3** FAT12 boot sector parser — BPB, cluster size, FAT location
- [ ] **2.4** FAT12 cluster chain walker — follow FAT entries to read files
- [ ] **2.5** FAT12 directory listing — parse root dir + subdirs
- [ ] **2.6** FAT12 file read — open + read file contents into memory
- [ ] **2.7** FAT12 file write — create + write files to disk
- [ ] **2.8** Block device abstraction — generic `read_sector`/`write_sector` interface
- [ ] **2.9** VFS layer — mount points, path resolution, unified file API
- [ ] **2.10** `SYS_OPEN` / `SYS_READ` / `SYS_WRITE` / `SYS_CLOSE` syscalls
- [ ] **2.11** `SYS_SEEK` / `SYS_STAT` / `SYS_OPENDIR` / `SYS_READDIR` syscalls
- [ ] **2.12** Test — mount FAT12 image, list files, read file contents

---

## Phase 3: Dynamic Linker

> Support shared libraries (.so) so Qt can be loaded as shared libs instead of static.

- [ ] **3.1** ELF dynamic section parser — `DT_NEEDED`, `DT_SYMTAB`, `DT_STRTAB`
- [ ] **3.2** Shared library loader — load `.so` at random base, relocate entries
- [ ] **3.3** PLT/GOT stub — lazy binding for function calls across libraries
- [ ] **3.4** `LD_PRELOAD` equivalent — inject libraries at process start
- [ ] **3.5** Dynamic linker syscall — `SYS_DLOPEN` / `SYS_DLSYM`
- [ ] **3.6** Test — load a shared library and call a function from it

---

## Phase 4: Windowing System / Compositor

> Build a Wayland-style compositor that Qt can render to via a QPlatform plugin.

- [ ] **4.1** Surface abstraction — window surface (backbuffer) creation/destroy
- [ ] **4.2** Compositor process — own the LFB, composite all window surfaces
- [ ] **4.3** Window management — create, destroy, move, resize, focus, z-order
- [ ] **4.4** Input multiplexer — route keyboard/mouse to focused window
- [ ] **4.5** IPC protocol — compositor ↔ app messaging (create surface, blit, input)
- [ ] **4.6** Shared memory transport — zero-copy surface sharing between app ↔ compositor
- [ ] **4.7** Mouse cursor rendering — hardware cursor or software cursor overlay
- [ ] **4.8** Drag and drop support — cross-window data transfer
- [ ] **4.9** Clipboard — copy/paste between windows
- [ ] **4.10** Taskbar / dock — app launcher, running apps indicator
- [ ] **4.11** Desktop — wallpaper, icon grid, right-click menu
- [ ] **4.12** Test — open two windows, drag them, click between them

---

## Phase 5: Qt Cross-Compilation

> Cross-compile Qt5 for the OS and write the custom QPlatform plugin.

- [ ] **5.1** Set up cross-compile toolchain — `clang --target=i386-elf` + sysroot
- [ ] **5.2** Build POSIX stubs — `mmap`, `pthread`, `fork`, `socket`, `ioctl` stubs
- [ ] **5.3** Build zlib (Qt dependency) for target
- [ ] **5.4** Build freetype (Qt font rendering) for target
- [ ] **5.5** Build harfbuzz (Qt text shaping) for target
- [ ] **5.6** Build libpng (Qt image support) for target
- [ ] **5.7** Cross-compile Qt Core — configure with minimal features
- [ ] **5.8** Cross-compile Qt GUI — no OpenGL, no X11, no Wayland
- [ ] **5.9** Cross-compile Qt Widgets — static build for minimal footprint
- [ ] **5.10** Custom `QPlatformIntegration` plugin — bridge Qt to our compositor IPC
- [ ] **5.11** Custom `QPlatformWindow` — maps to our window surfaces
- [ ] **5.12** Custom `QPlatformBackingStore` — software rendering to framebuffer
- [ ] **5.13** Custom `QPlatformFontDatabase` — load fonts from FAT12 `/boot/fonts/`
- [ ] **5.14** Custom `QPlatformInputContext` — keyboard layout mapping
- [ ] **5.15** Test — build a minimal Qt Widgets app (hello world)

---

## Phase 6: Qt Integration + Apps

> Package everything into a bootable ISO with real Qt desktop apps.

- [ ] **6.1** Create disk image — FAT12 with `/bin`, `/lib`, `/boot`, `/etc`
- [ ] **6.2** Package Qt libs — `libQt5Core.so`, `libQt5Gui.so`, `libQt5Widgets.so` on disk
- [ ] **6.3** Qt file manager app — tree view, file icons, open/copy/delete
- [ ] **6.4** Qt terminal emulator — replace current terminal with Qt version
- [ ] **6.5** Qt text editor — basic notepad with menus, toolbar, syntax highlight
- [ ] **6.6** Qt calculator — simple calculator widget
- [ ] **6.7** Qt system monitor — CPU, memory, process list (uses `SYS_GET_TIME` etc)
- [ ] **6.8** Qt desktop shell — taskbar + app launcher + window manager in Qt
- [ ] **6.9** Qt settings/preferences app
- [ ] **6.10** Final ISO build — boot into Qt desktop environment

---

## Phase 7: Polish + Optimization

> Visual polish, power management, and extended hardware support.

- [ ] **7.1** Anti-aliased font rendering — subpixel or grayscale AA
- [ ] **7.2** Alpha blending / shadows on windows
- [ ] **7.3** Smooth window resize — live redraw during drag
- [ ] **7.4** Animated transitions — window open/close animations
- [ ] **7.5** Multi-monitor support — detect VBE modes, span across displays
- [ ] **7.6** Power management — shutdown, reboot, sleep (ACPI)
- [ ] **7.7** Sound support — PCI audio (AC97/HDA) for Qt multimedia
- [ ] **7.8** USB support — for keyboard/mouse/flash drives
- [ ] **7.9** Network stack — TCP/IP for Qt networking module
- [ ] **7.10** Package manager — install/remove Qt apps from package repo

---

## Dependency Graph

```
Phase 1 (Foundations)
  └── Phase 2 (Filesystem + Disk)
        └── Phase 3 (Dynamic Linker)
              └── Phase 4 (Windowing System)
                    └── Phase 5 (Qt Cross-Compilation)
                          └── Phase 6 (Qt Integration + Apps)
                                └── Phase 7 (Polish + Optimization)
```

Phase 7 items can be done in parallel once Phase 6 is complete.

---

## Current Status

**Completed:** Terminal module (ANSI parser, history, rendering), mouse driver fixes, GFX primitives, RTC driver, PIT ms timer, SYS_GET_TIME syscall

**In Progress:** None (starting Phase 1)

**Next Step:** 1.1 — Fix `process_context_switch` save/restore
