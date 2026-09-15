#include "kheap.h"
#include "serial.h"
#include "types.h"

/* =========================================================
 *  operator new / delete
 * ========================================================= */

void *operator new(size_t size)
{
    if (size == 0) size = 1;
    void *p = kmalloc(size);
    if (!p) {
        serial_write("operator new: out of memory\n", 28);
        while (1) { __asm__ volatile ("hlt"); }
    }
    return p;
}

void *operator new[](size_t size)
{
    if (size == 0) size = 1;
    void *p = kmalloc(size);
    if (!p) {
        serial_write("operator new[]: out of memory\n", 30);
        while (1) { __asm__ volatile ("hlt"); }
    }
    return p;
}

void operator delete(void *ptr)
{
    if (ptr) kfree(ptr);
}

void operator delete[](void *ptr)
{
    if (ptr) kfree(ptr);
}

void operator delete(void *ptr, size_t)
{
    if (ptr) kfree(ptr);
}

void operator delete[](void *ptr, size_t)
{
    if (ptr) kfree(ptr);
}

/* =========================================================
 *  Pure virtual call handler
 * ========================================================= */

extern "C" void __cxa_pure_virtual(void)
{
    serial_write("__cxa_pure_virtual: pure virtual method called!\n", 48);
    while (1) { __asm__ volatile ("hlt"); }
}

/* =========================================================
 *  Guard variables (thread-safe static init)
 *  Single-threaded OS: just use a byte flag per guard.
 * ========================================================= */

struct __guard_obj {
    signed char flag;
};

extern "C" int __cxa_guard_acquire(__guard_obj *guard)
{
    return !guard->flag;
}

extern "C" void __cxa_guard_release(__guard_obj *guard)
{
    guard->flag = 1;
}

extern "C" void __cxa_guard_abort(__guard_obj *)
{
    /* nothing */
}

/* =========================================================
 *  Exception handling (no-exceptions stubs)
 *  If -fno-exceptions, these won't be called at runtime,
 *  but we define them so the linker is satisfied.
 * ========================================================= */

extern "C" {
    struct _Unwind_Exception;
    struct _Unwind_Context;

    /* Called by the personality routine to perform cleanup (stack unwinding) */
    typedef void (*_Unwind_Exception_Cleanup_Fn)(int, _Unwind_Exception *);

    struct _Unwind_Exception {
        uint64 exception_class;
        _Unwind_Exception_Cleanup_Fn exception_cleanup;
        uint64 private_1;
        uint64 private_2;
    };

    void __cxa_allocate_exception(uint32 size)
    {
        void *p = kmalloc(size);
        if (!p) {
            serial_write("__cxa_allocate_exception: OOM\n", 30);
            while (1) { __asm__ volatile ("hlt"); }
        }
        (void)p;
    }

    void __cxa_free_exception(void *ptr)
    {
        if (ptr) kfree(ptr);
    }

    void __cxa_throw(void *exception_object, void *tinfo, void (*dest)(void *))
    {
        (void)exception_object; (void)tinfo; (void)dest;
        serial_write("__cxa_throw: exception thrown but exceptions disabled\n", 53);
        while (1) { __asm__ volatile ("hlt"); }
    }

    void __cxa_begin_catch(void *exception_object)
    {
        (void)exception_object;
        serial_write("__cxa_begin_catch: catch not supported\n", 39);
        while (1) { __asm__ volatile ("hlt"); }
    }

    void __cxa_end_catch(void)
    {
    }

    void __cxa_rethrow(void)
    {
        serial_write("__cxa_rethrow: rethrow not supported\n", 37);
        while (1) { __asm__ volatile ("hlt"); }
    }

    void __cxa_bad_typeid(void)
    {
        serial_write("__cxa_bad_typeid\n", 17);
        while (1) { __asm__ volatile ("hlt"); }
    }

    void __cxa_bad_cast(void)
    {
        serial_write("__cxa_bad_cast\n", 15);
        while (1) { __asm__ volatile ("hlt"); }
    }

    void __cxa_bad_array_new_length(void)
    {
        serial_write("__cxa_bad_array_new_length\n", 27);
        while (1) { __asm__ volatile ("hlt"); }
    }
}

/* =========================================================
 *  Constructor / Destructor arrays
 *  Called from kmain() before handing off to user-space.
 * ========================================================= */

extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);
extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);

extern "C" void __libc_init_array(void)
{
    uint32 count = (uint32)(__init_array_end - __init_array_start);
    uint32 i;
    for (i = 0; i < count; i++) {
        __init_array_start[i]();
    }
}

extern "C" void __libc_fini_array(void)
{
    uint32 count = (uint32)(__fini_array_end - __fini_array_start);
    uint32 i;
    for (i = 0; i < count; i++) {
        __fini_array_start[i]();
    }
}
