#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/cpu.h>
#include <kern/spinlock.h>
#include <kern/time.h>

static struct Taskstate ts;

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
static struct Trapframe *last_tf;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
		return "Hardware Interrupt";
	return "(unknown trap)";
}


void
trap_init(void)
{
	extern struct Segdesc gdt[];

	// LAB 3: Your code here.
	// Handlers defined in trapentry.S
	extern void handler_divide();
	extern void handler_debug();
	extern void handler_nmi();
	extern void handler_brkpt();
	extern void handler_oflow();
	extern void handler_bound();
	extern void handler_illop();
	extern void handler_device();
	extern void handler_dblflt();
	//extern void handler_coproc();
	extern void handler_tss();
	extern void handler_segnp();
	extern void handler_stack();
	extern void handler_gpflt();
	extern void handler_pgflt();
	//extern void handler_res();
	extern void handler_fperr();
	extern void handler_align();
	extern void handler_mchk();
	extern void handler_simderr();
	extern void handler_syscall();

	extern void handler_irq0();
	extern void handler_irq1();
	extern void handler_irq2();
	extern void handler_irq3();
	extern void handler_irq4();
	extern void handler_irq5();
	extern void handler_irq6();
	extern void handler_irq7();
	extern void handler_irq8();
	extern void handler_irq9();
	extern void handler_irq10();
	extern void handler_irq11();
	extern void handler_irq12();
	extern void handler_irq13();
	extern void handler_irq14();
	extern void handler_irq15();

	// Initialize entries in idt
	SETGATE(idt[T_DIVIDE], 0, GD_KT, handler_divide, DPL_KERN);
	SETGATE(idt[T_DEBUG], 0, GD_KT, handler_debug, DPL_KERN); // Don't allow interrupts while in debug mode
	SETGATE(idt[T_NMI], 0, GD_KT, handler_nmi, DPL_KERN);
	SETGATE(idt[T_BRKPT], 0, GD_KT, handler_brkpt, DPL_USER);
	SETGATE(idt[T_OFLOW], 0, GD_KT, handler_oflow, DPL_KERN);
	SETGATE(idt[T_BOUND], 0, GD_KT, handler_bound, DPL_KERN);
	SETGATE(idt[T_ILLOP], 0, GD_KT, handler_illop, DPL_KERN);
	SETGATE(idt[T_DEVICE], 0, GD_KT, handler_device, DPL_KERN);
	SETGATE(idt[T_DBLFLT], 0, GD_KT, handler_dblflt, DPL_KERN);
	//SETGATE(idt[T_COPROC], 0, GD_KT, handler_coproc, 0);
	SETGATE(idt[T_TSS], 0, GD_KT, handler_tss, DPL_KERN);
	SETGATE(idt[T_SEGNP], 0, GD_KT, handler_segnp, DPL_KERN);
	SETGATE(idt[T_STACK], 0, GD_KT, handler_stack, DPL_KERN);
	SETGATE(idt[T_GPFLT], 0, GD_KT, handler_gpflt, DPL_KERN);
	SETGATE(idt[T_PGFLT], 0, GD_KT, handler_pgflt, DPL_KERN);
	//SETGATE(idt[T_RES], 0, GD_KT, handler_res, 0);
	SETGATE(idt[T_FPERR], 0, GD_KT, handler_fperr, DPL_KERN);
	SETGATE(idt[T_ALIGN], 0, GD_KT, handler_align, DPL_KERN);
	SETGATE(idt[T_MCHK], 0, GD_KT, handler_mchk, DPL_KERN);
	SETGATE(idt[T_SIMDERR], 0, GD_KT, handler_simderr, DPL_KERN);

	//Initial system call entry
	SETGATE(idt[T_SYSCALL], 0, GD_KT, handler_syscall, DPL_USER);

	// Initialize IRQ entries
	SETGATE(idt[IRQ_OFFSET], 0, GD_KT, handler_irq0, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 1], 0, GD_KT, handler_irq1, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 2], 0, GD_KT, handler_irq2, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 3], 0, GD_KT, handler_irq3, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 4], 0, GD_KT, handler_irq4, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 5], 0, GD_KT, handler_irq5, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 6], 0, GD_KT, handler_irq6, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 7], 0, GD_KT, handler_irq7, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 8], 0, GD_KT, handler_irq8, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 9], 0, GD_KT, handler_irq9, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 10], 0, GD_KT, handler_irq10, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 11], 0, GD_KT, handler_irq11, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 12], 0, GD_KT, handler_irq12, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 13], 0, GD_KT, handler_irq13, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 14], 0, GD_KT, handler_irq14, DPL_KERN);
	SETGATE(idt[IRQ_OFFSET + 15], 0, GD_KT, handler_irq15, DPL_KERN);

	// Per-CPU setup 
	trap_init_percpu();
}

// Initialize and load the per-CPU TSS and IDT
void
trap_init_percpu(void)
{
	// The example code here sets up the Task State Segment (TSS) and
	// the TSS descriptor for CPU 0. But it is incorrect if we are
	// running on other CPUs because each CPU has its own kernel stack.
	// Fix the code so that it works for all CPUs.
	//
	// Hints:
	//   - The macro "thiscpu" always refers to the current CPU's
	//     struct Cpu;
	//   - The ID of the current CPU is given by cpunum() or
	//     thiscpu->cpu_id;
	//   - Use "thiscpu->cpu_ts" as the TSS for the current CPU,
	//     rather than the global "ts" variable;
	//   - Use gdt[(GD_TSS0 >> 3) + i] for CPU i's TSS descriptor;
	//   - You mapped the per-CPU kernel stacks in mem_init_mp()
	//
	// ltr sets a 'busy' flag in the TSS selector, so if you
	// accidentally load the same TSS on more than one CPU, you'll
	// get a triple fault.  If you set up an individual CPU's TSS
	// wrong, you may not get a fault until you try to return from
	// user space on that CPU.
	//
	// LAB 4: Your code here:
	thiscpu->cpu_ts.ts_esp0 = KSTACKTOP - cpunum() * (KSTKSIZE + KSTKGAP);
	thiscpu->cpu_ts.ts_ss0 = GD_KD;

	// Initialize the TSS slot of the gdt.
	gdt[(GD_TSS0 >> 3) + cpunum()] = SEG16(STS_T32A, (uint32_t) (&thiscpu->cpu_ts),
					sizeof(struct Taskstate), 0);
	gdt[(GD_TSS0 >> 3) + cpunum()].sd_s = 0;

	// Load the TSS selector (like other segment selectors, the
	// bottom three bits are special; we leave them 0)
	ltr(((GD_TSS0 >> 3) + cpunum()) << 3);

	// Load the IDT
	lidt(&idt_pd);
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p from CPU %d\n", tf, cpunum());
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
		cprintf("  cr2  0x%08x\n", rcr2());
	cprintf("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	if (tf->tf_trapno == T_PGFLT)
		cprintf(" [%s, %s, %s]\n",
			tf->tf_err & 4 ? "user" : "kernel",
			tf->tf_err & 2 ? "write" : "read",
			tf->tf_err & 1 ? "protection" : "not-present");
	else
		cprintf("\n");
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	if ((tf->tf_cs & 3) != 0) {
		cprintf("  esp  0x%08x\n", tf->tf_esp);
		cprintf("  ss   0x----%04x\n", tf->tf_ss);
	}
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static void
trap_dispatch(struct Trapframe *tf)
{
	// Handle processor exceptions.
	// LAB 3: Your code here.
	switch(tf->tf_trapno) {
	case T_PGFLT:
		page_fault_handler(tf);
		return;
	case T_BRKPT:
	case T_DEBUG:
		monitor(tf);
		return;
	case T_SYSCALL:
		tf->tf_regs.reg_eax = syscall(tf->tf_regs.reg_eax, // syscall #
					tf->tf_regs.reg_edx, // arg1
					tf->tf_regs.reg_ecx, // arg2
					tf->tf_regs.reg_ebx, // arg3
					tf->tf_regs.reg_edi, // arg4
					tf->tf_regs.reg_esi);// arg5
		return;
	}

	// Handle spurious interrupts
	// The hardware sometimes raises these because of noise on the
	// IRQ line or other reasons. We don't care.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
		cprintf("Spurious interrupt on irq 7\n");
		print_trapframe(tf);
		return;
	}

	// Handle clock interrupts. Don't forget to acknowledge the
	// interrupt using lapic_eoi() before calling the scheduler!
	// Add time tick increment to clock interrupts.
	// Be careful! In multiprocessors, clock interrupts are
	// triggered on every CPU.
	// LAB 4: Your code here.
	// LAB 6: Your code here.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_TIMER) {
		time_tick();
		lapic_eoi();
		sched_yield();
		return;
	}

	// Handle keyboard and serial interrupts.
	// LAB 7: Your code here.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SERIAL) {
		serial_intr();
		return;
	}

	if (tf->tf_trapno == IRQ_OFFSET + IRQ_KBD) {
		kbd_intr();
		return;
	}

	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel");
	else {
		env_destroy(curenv);
		return;
	}
}

void
trap(struct Trapframe *tf)
{
	// The environment may have set DF and some versions
	// of GCC rely on DF being clear
	asm volatile("cld" ::: "cc");

	// Halt the CPU if some other CPU has called panic()
	extern char *panicstr;
	if (panicstr)
		asm volatile("hlt");

	// Check that interrupts are disabled.  If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
	assert(!(read_eflags() & FL_IF));

	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		// Acquire the big kernel lock before doing any
		// serious kernel work.
		// LAB 4: Your code here.
		lock_kernel();
		assert(curenv);

		// Garbage collect if current enviroment is a zombie
		if (curenv->env_status == ENV_DYING) {
			env_free(curenv);
			curenv = NULL;
			sched_yield();
		}

		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}

	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// If we made it to this point, then no other environment was
	// scheduled, so we should return to the current environment
	// if doing so makes sense.
	if (curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);
	else
		sched_yield();
}


void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.

	// LAB 3: Your code here.
	if (tf->tf_cs == GD_KT) {
		print_trapframe(tf);
		panic("kernel page fault va %08x ip %08x env %x\n",
		      fault_va, tf->tf_eip, curenv->env_id);
	}

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Call the environment's page fault upcall, if one exists.  Set up a
	// page fault stack frame on the user exception stack (below
	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
	//
	// The page fault upcall might cause another page fault, in which case
	// we branch to the page fault upcall recursively, pushing another
	// page fault stack frame on top of the user exception stack.
	//
	// The trap handler needs one word of scratch space at the top of the
	// trap-time stack in order to return.  In the non-recursive case, we
	// don't have to worry about this because the top of the regular user
	// stack is free.  In the recursive case, this means we have to leave
	// an extra word between the current top of the exception stack and
	// the new stack frame because the exception stack _is_ the trap-time
	// stack.
	//
	// If there's no page fault upcall, the environment didn't allocate a
	// page for its exception stack or can't write to it, or the exception
	// stack overflows, then destroy the environment that caused the fault.
	// Note that the grade script assumes you will first check for the page
	// fault upcall and print the "user fault va" message below if there is
	// none.  The remaining three checks can be combined into a single test.
	//
	// Hints:
	//   user_mem_assert() and env_run() are useful here.
	//   To change what the user environment runs, modify 'curenv->env_tf'
	//   (the 'tf' variable points at 'curenv->env_tf').

	// LAB 4: Your code here.
	if (!curenv->env_pgfault_upcall) {
		goto destroy;
	}

	// Check that exception stack is allocated
	user_mem_assert(curenv, (void *)(UXSTACKTOP - 4), 4, 0);

	uintptr_t exstack;
	struct UTrapframe *utf;
	
	// Figure out top where trapframe should end, leaving 1 word scratch space
	if (tf->tf_esp >= UXSTACKTOP-PGSIZE && tf->tf_esp <= UXSTACKTOP-1) {
		exstack = tf->tf_esp - 4; // recursive
	}
	else {
		exstack = UXSTACKTOP; // non-recursive
	}

	// Check if enough space to copy trapframe
	if ((exstack - sizeof(struct UTrapframe)) < UXSTACKTOP-PGSIZE) {
		goto destroy;
	}

	// Set up UTrapframe on exception stack
	utf = (struct UTrapframe *) (exstack - sizeof(struct UTrapframe));
	utf->utf_fault_va = fault_va;
	utf->utf_err = tf->tf_err;
	utf->utf_regs = tf->tf_regs;
	utf->utf_eip = tf->tf_eip;
	utf->utf_eflags = tf->tf_eflags;
	utf->utf_esp = tf->tf_esp;
	// Fix trapframe to return to user handler
	tf->tf_esp = (uintptr_t) utf;
	tf->tf_eip = (uintptr_t) curenv->env_pgfault_upcall;
	env_run(curenv);

	panic("Unreachable code!\n");
	
	destroy:
	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
}
