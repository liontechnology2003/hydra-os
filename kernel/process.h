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

typedef struct {
    uint32 eax, ebx, ecx, edx;
    uint32 esi, edi, ebp, esp;
    uint32 eip, eflags;
    uint32 cs, ss, ds, es, fs, gs;
} __attribute__((packed)) regs_t;

typedef struct process {
    int           pid;
    int           state;
    char          name[PROCESS_NAME_MAX];
    regs_t        regs;
    page_directory_t *page_dir;
    uint32        kernel_esp;
    uint32        user_esp;
    uint32        stack_base;
    struct process *next;
} process_t;

void     process_init(void);
int      process_create(const char *name, void (*entry)(void));
void     process_exit(void);
void     process_schedule(void);
process_t *process_current(void);
void     process_switch(process_t *proc);
int      process_fork(void);

extern process_t *current_proc;

/* Process query for shell/taskmgr */
#define PROCESS_QUERY_MAX 32
int process_query_all(char *buf, int maxlen);

/* Called from assembly */
void     process_context_switch(process_t *next);

#endif /* KERNEL_PROCESS_H */
