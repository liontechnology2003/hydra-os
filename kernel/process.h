#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include "types.h"
#include "paging.h"

#define PROCESS_MAX      32
#define PROCESS_NAME_MAX 32
#define PROCESS_STACK_SIZE 4096

#define PROC_UNUSED   0
#define PROC_READY    1
#define PROC_RUNNING  2
#define PROC_BLOCKED  3
#define PROC_ZOMBIE   4

/* Register save area — layout must match process_asm.s offsets exactly.
 * Offsets: eax=0  ebx=4  ecx=8  edx=12  esi=16  edi=20
 *          ebp=24 esp=28 eip=32 eflags=36 cs=40  ss=44
 *          ds=48  es=52  fs=56  gs=60  (total: 64 bytes) */
typedef struct {
    uint32 eax, ebx, ecx, edx;
    uint32 esi, edi, ebp, esp;
    uint32 eip, eflags;
    uint32 cs, ss;
    uint32 ds, es, fs, gs;
} __attribute__((packed)) regs_t;

typedef struct process {
    int           pid;
    int           state;
    char          name[PROCESS_NAME_MAX];
    regs_t        regs;
    page_directory_t *page_dir;
    uint32        kernel_esp;       /* top of kernel stack */
    uint32        kernel_stack_base; /* bottom of kernel stack (for freeing) */
    uint32        stack_base;       /* bottom of user stack (for freeing) */
    struct process *next;
} process_t;

void     process_init(void);
int      process_create(const char *name, void (*entry)(void));
void     process_exit(void);
void     process_schedule(void);
process_t *process_current(void);
void     process_switch(process_t *proc);
int      process_fork(void);
int      process_waitpid(int pid);

extern process_t *current_proc;

/* Process query for shell/taskmgr */
#define PROCESS_QUERY_MAX 32
int process_query_all(char *buf, int maxlen);

/* Called from assembly */
void     process_context_switch(process_t *next);

#endif /* KERNEL_PROCESS_H */
