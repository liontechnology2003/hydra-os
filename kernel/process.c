#include "process.h"
#include "kheap.h"
#include "paging.h"
#include "gdt.h"
#include "string.h"
#include "serial.h"
#include "pmm.h"

static process_t process_table[PROCESS_MAX];
process_t *current_proc = 0;
static process_t *ready_list = 0;
static int next_pid = 1;

extern void switch_to_user_mode(void *entry, uint32 stack);

void process_init(void)
{
    memset(process_table, 0, sizeof(process_table));
    current_proc = 0;
    ready_list = 0;
    next_pid = 1;
    serial_write("PROCESS: initialized\n", 20);
}

static process_t *alloc_process(void)
{
    int i;
    for (i = 0; i < PROCESS_MAX; i++) {
        if (process_table[i].state == PROC_UNUSED) {
            memset(&process_table[i], 0, sizeof(process_t));
            process_table[i].pid = next_pid++;
            return &process_table[i];
        }
    }
    return 0;
}

static void enqueue_ready(process_t *proc)
{
    proc->next = ready_list;
    ready_list = proc;
    proc->state = PROC_READY;
}

int process_create(const char *name, void (*entry)(void))
{
    process_t *proc;
    uint32 user_stack;
    uint32 kernel_stack;

    proc = alloc_process();
    if (!proc) {
        return -1;
    }

    strncpy(proc->name, name, PROCESS_NAME_MAX - 1);

    /* Create page directory for the process (shares kernel mappings) */
    proc->page_dir = paging_create_directory();
    if (!proc->page_dir) {
        proc->state = PROC_UNUSED;
        return -1;
    }

    /* Allocate user stack (4KB) */
    user_stack = (uint32)kmalloc(PROCESS_STACK_SIZE);
    if (!user_stack) {
        paging_free_directory(proc->page_dir);
        proc->state = PROC_UNUSED;
        return -1;
    }
    memset((void *)user_stack, 0, PROCESS_STACK_SIZE);
    proc->stack_base = user_stack;

    /* Map user stack into the process's address space */
    {
        uint32 j;
        uint32 phys;
        for (j = 0; j < PROCESS_STACK_SIZE; j += PMM_FRAME_SIZE) {
            phys = paging_get_physical(user_stack + j);
            if (phys) {
                paging_map_page(user_stack + j, phys,
                                PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
            }
        }
    }

    /* Allocate kernel stack (4KB) */
    kernel_stack = (uint32)kmalloc(PROCESS_STACK_SIZE);
    if (!kernel_stack) {
        kfree((void *)user_stack);
        paging_free_directory(proc->page_dir);
        proc->state = PROC_UNUSED;
        return -1;
    }
    memset((void *)kernel_stack, 0, PROCESS_STACK_SIZE);

    proc->kernel_esp = kernel_stack + PROCESS_STACK_SIZE;

    /* Set up initial register state */
    proc->regs.eip = (uint32)entry;
    proc->regs.esp = user_stack + PROCESS_STACK_SIZE;
    proc->regs.ebp = user_stack + PROCESS_STACK_SIZE;
    proc->regs.cs = 0x1B;    /* User code segment (GDT index 3, ring 3) */
    proc->regs.ss = 0x23;    /* User data segment (GDT index 4, ring 3) */
    proc->regs.ds = 0x23;
    proc->regs.es = 0x23;
    proc->regs.fs = 0x23;
    proc->regs.gs = 0x23;
    proc->regs.eflags = 0x202; /* IF=1 */

    serial_write("PROCESS: created '", 18);
    serial_write((char *)name, strlen(name));
    serial_write("' pid=", 5);
    {
        char buf[8];
        itoa(buf, proc->pid);
        serial_write(buf, strlen(buf));
    }
    serial_write("\n", 1);

    enqueue_ready(proc);
    return proc->pid;
}

void process_exit(void)
{
    if (!current_proc) return;

    serial_write("PROCESS: exiting pid=", 20);
    {
        char buf[8];
        itoa(buf, current_proc->pid);
        serial_write(buf, strlen(buf));
    }
    serial_write("\n", 1);

    /* Free user stack */
    if (current_proc->stack_base) {
        kfree((void *)current_proc->stack_base);
    }

    current_proc->state = PROC_ZOMBIE;
    current_proc = 0;

    /* Schedule next process */
    process_schedule();
}

process_t *process_current(void)
{
    return current_proc;
}

void process_schedule(void)
{
    process_t *next;

    if (!ready_list) {
        return;
    }

    next = ready_list;
    ready_list = ready_list->next;
    next->next = 0;

    if (current_proc && current_proc->state == PROC_RUNNING) {
        current_proc->state = PROC_READY;
        enqueue_ready(current_proc);
    }

    current_proc = next;
    current_proc->state = PROC_RUNNING;

    /* Switch page directory */
    paging_switch_directory(current_proc->page_dir);

    /* Set kernel stack for TSS */
    tss_set_kernel_stack(current_proc->kernel_esp);

    /* Context switch */
    process_context_switch(current_proc);
}

int process_fork(void)
{
    process_t *child;
    process_t *parent = current_proc;

    if (!parent) return -1;

    child = alloc_process();
    if (!child) return -1;

    strncpy(child->name, parent->name, PROCESS_NAME_MAX - 1);
    child->page_dir = paging_create_directory();
    if (!child->page_dir) {
        child->state = PROC_UNUSED;
        return -1;
    }

    /* Copy registers */
    child->regs = parent->regs;
    child->regs.eax = 0;  /* child gets 0 from fork */

    /* Allocate and map user stack */
    {
        uint32 user_stack = (uint32)kmalloc(PROCESS_STACK_SIZE);
        if (!user_stack) {
            paging_free_directory(child->page_dir);
            child->state = PROC_UNUSED;
            return -1;
        }
        memcpy((void *)user_stack, (void *)parent->stack_base, PROCESS_STACK_SIZE);
        child->stack_base = user_stack;
        child->regs.esp = user_stack + (parent->regs.esp - parent->stack_base);
        child->regs.ebp = user_stack + (parent->regs.ebp - parent->stack_base);

        {
            uint32 i;
            for (i = 0; i < PROCESS_STACK_SIZE; i += PMM_FRAME_SIZE) {
                uint32 phys = paging_get_physical(user_stack + i);
                if (phys) {
                    paging_map_page(user_stack + i, phys,
                                    PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
                }
            }
        }
    }

    /* Allocate kernel stack */
    {
        uint32 kernel_stack = (uint32)kmalloc(PROCESS_STACK_SIZE);
        if (!kernel_stack) {
            kfree((void *)child->stack_base);
            paging_free_directory(child->page_dir);
            child->state = PROC_UNUSED;
            return -1;
        }
        memset((void *)kernel_stack, 0, PROCESS_STACK_SIZE);
        child->kernel_esp = kernel_stack + PROCESS_STACK_SIZE;
    }

    enqueue_ready(child);
    return child->pid;
}

int process_query_all(char *buf, int maxlen)
{
    int i;
    int pos = 0;

    for (i = 0; i < PROCESS_MAX && pos < maxlen - 1; i++) {
        if (process_table[i].state != PROC_UNUSED) {
            int j;
            char numbuf[8];

            /* PID */
            itoa(numbuf, process_table[i].pid);
            for (j = 0; numbuf[j] && pos < maxlen - 1; j++) {
                buf[pos++] = numbuf[j];
            }
            buf[pos++] = ' ';

            /* Name */
            for (j = 0; process_table[i].name[j] && pos < maxlen - 1; j++) {
                buf[pos++] = process_table[i].name[j];
            }
            buf[pos++] = ' ';

            /* State */
            {
                const char *state_str;
                switch (process_table[i].state) {
                    case PROC_READY:   state_str = "ready"; break;
                    case PROC_RUNNING: state_str = "running"; break;
                    case PROC_BLOCKED: state_str = "blocked"; break;
                    case PROC_ZOMBIE:  state_str = "zombie"; break;
                    default:           state_str = "unknown"; break;
                }
                for (j = 0; state_str[j] && pos < maxlen - 1; j++) {
                    buf[pos++] = state_str[j];
                }
            }
            buf[pos++] = '\n';
        }
    }

    if (pos < maxlen) buf[pos] = '\0';
    return pos;
}
