/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#ifndef _SHIM_TLS_H_
#define _SHIM_TLS_H_

#ifdef __ASSEMBLER__

#define SHIM_TLS_CANARY $xdeadbeef

#else /* !__ASSEMBLER__ */

#define SHIM_TLS_CANARY 0xdeadbeef

struct lock_record {
    enum { NO_LOCK, SEM_LOCK, READ_LOCK, WRITE_LOCK } type;
    void * lock;
    const char * filename;
    int lineno;
};

#define NUM_LOCK_RECORD      32
#define NUM_LOCK_RECORD_MASK (NUM_LOCK_RECORD - 1)

struct shim_regs {
    unsigned long           r15;
    unsigned long           r14;
    unsigned long           r13;
    unsigned long           r12;
    unsigned long           r11;
    unsigned long           r10;
    unsigned long           r9;
    unsigned long           r8;
    unsigned long           rcx;
    unsigned long           rdx;
    unsigned long           rsi;
    unsigned long           rdi;
    unsigned long           rbx;
    unsigned long           rbp;
};

struct shim_context {
    unsigned long           syscall_nr;
    void *                  sp;
    void *                  ret_ip;
    struct shim_regs *      regs;
    struct shim_context *   next;
    uint64_t                enter_time;
    uint64_t                preempt;
};

#ifdef IN_SHIM

#include <shim_defs.h>

#define SIGNAL_DELAYED       (0x80000000UL)

#endif /* IN_SHIM */

typedef struct {
    uint64_t                canary;
    void *                  self;
    void *                  tp;
    struct shim_context     context;
    unsigned int            tid;
    int                     pal_errno;
    void *                  debug_buf;

    /* This record is for testing the memory of user inputs.
     * If a segfault occurs with the range [start, end],
     * the code addr is set to cont_addr to alert the caller. */
    struct {
        void * start, * end;
        void * cont_addr;
    } test_range;
} shim_tcb_t;

#ifdef IN_SHIM

#include <stddef.h>

void init_tcb (shim_tcb_t * tcb);

#ifdef SHIM_TCB_USE_GS
typedef struct __libc_tcb
{
    /* nothing here. just type to point to libc tls
     * LibOS doesn't access this structure as it's private to libc.
     */
} __libc_tcb_t;

static inline shim_tcb_t * SHIM_GET_TLS(void)
{
    PAL_TCB * tcb = pal_get_tcb();
    return (shim_tcb_t*)tcb->libos_tcb;
}

static inline bool SHIM_TLS_CHECK_CANARY(void)
{
    /* optimize to use single movq %gs:<offset> */
    shim_tcb_t * shim_tcb = SHIM_GET_TLS();
    uint64_t __canary = shim_tcb->canary;
    return __canary == SHIM_TLS_CANARY;
}
#else
typedef struct
{
    void *                  tcb, * dtv, * self;
    int                     mthreads, gscope;
    uintptr_t               sysinfo, sg, pg;
    unsigned long int       vgetcpu_cache[2];
    int                     __unused1;
    shim_tcb_t              shim_tcb;
} __libc_tcb_t;

static inline bool SHIM_TLS_CHECK_CANARY(void)
{
    uint64_t __canary;
    asm ("movq %%fs:%c1,%q0" : "=r" (__canary)
         : "i" (offsetof(__libc_tcb_t, shim_tcb.canary)));
    return __canary == SHIM_TLS_CANARY;
}

static inline shim_tcb_t * SHIM_GET_TLS(void)
{
    shim_tcb_t *__self;
    asm ("movq %%fs:%c1,%q0" : "=r" (__self)
         : "i" (offsetof(__libc_tcb_t, shim_tcb.self)));
    return __self;
}
#endif

#endif /* IN_SHIM */

#endif /* !__ASSEMBLER__ */

#endif /* _SHIM_H_ */
