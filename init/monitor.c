// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <linux/types.h>
#include <linux/extable.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/binfmts.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/stackprotector.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/bootmem.h>
#include <linux/acpi.h>
#include <linux/console.h>
#include <linux/nmi.h>
#include <linux/percpu.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/kernel_stat.h>
#include <linux/start_kernel.h>
#include <linux/security.h>
#include <linux/smp.h>
#include <linux/profile.h>
#include <linux/rcupdate.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>
#include <linux/writeback.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/cgroup.h>
#include <linux/efi.h>
#include <linux/tick.h>
#include <linux/interrupt.h>
#include <linux/taskstats_kern.h>
#include <linux/delayacct.h>
#include <linux/unistd.h>
#include <linux/rmap.h>
#include <linux/mempolicy.h>
#include <linux/key.h>
#include <linux/buffer_head.h>
#include <linux/page_ext.h>
#include <linux/debug_locks.h>
#include <linux/debugobjects.h>
#include <linux/lockdep.h>
#include <linux/kmemleak.h>
#include <linux/pid_namespace.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/init.h>
#include <linux/signal.h>
#include <linux/idr.h>
#include <linux/kgdb.h>
#include <linux/ftrace.h>
#include <linux/async.h>
#include <linux/kmemcheck.h>
#include <linux/sfi.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/perf_event.h>
#include <linux/ptrace.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/sched_clock.h>
#include <linux/sched/task.h>
#include <linux/sched/task_stack.h>
#include <linux/context_tracking.h>
#include <linux/random.h>
#include <linux/list.h>
#include <linux/integrity.h>
#include <linux/proc_ns.h>
#include <linux/io.h>
#include <linux/cache.h>
#include <linux/rodata_test.h>

#include <asm/io.h>
#include <asm/bugs.h>
#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/cacheflush.h>

struct hw_trapframe {
};

#define CMDBUF_SIZE	80	// enough for one VGA text line

typedef struct command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char **argv, struct hw_trapframe *hw_tf);
} command_t;

static command_t commands[];

/***** Implementations of basic kernel monitor commands *****/

int mon_help(int argc, char **argv, struct hw_trapframe *hw_tf)
{
	int i;

	for (i = 0; commands[i].name; i++)
		printk("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int mon_ps(int argc, char** argv, struct hw_trapframe *hw_tf)
{
	return 0;
}

int mon_kerninfo(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	extern char _start[], etext[], end[];

	printk("Special kernel symbols:\n");
	printk("  _start %016x (virt)  %016x (phys)\n", _start, (uintptr_t)(_start - KERNBASE));
	printk("  etext  %016x (virt)  %016x (phys)\n", etext, (uintptr_t)(etext - KERNBASE));
	printk("  end    %016x (virt)  %016x (phys)\n", end, (uintptr_t)(end - KERNBASE));
	printk("Kernel executable memory footprint: %dKB\n",
		(uint32_t)(end-_start+1023)/1024);
#endif
	return 0;
}

static int __backtrace(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	uintptr_t pc, fp;
	if (argc == 1) {
		backtrace();
		return 0;
	}
	if (argc != 3) {
		printk("Need either no arguments, or two (PC and FP) in hex\n");
		return 1;
	}
	pc = strtol(argv[1], 0, 16);
	fp = strtol(argv[2], 0, 16);
	printk("Backtrace from instruction %p, with frame pointer %p\n", pc, fp);
	backtrace_frame(pc, fp);
#endif
	return 0;
}

int mon_backtrace(int argc, char **argv, struct hw_trapframe *hw_tf)
{
	return __backtrace(argc, argv, hw_tf);
}

int mon_reboot(int argc, char **argv, struct hw_trapframe *hw_tf)
{
	printk("[Scottish Accent]: She's goin' down, Cap'n!\n");
	//reboot();

	// really, should never see this
	panic("Sigh....\n");
	return 0;
}

static int __showmapping(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	struct proc *p;
	uintptr_t start;
	size_t size;
	pgdir_t pgdir;
	pid_t pid;
	if (argc < 3) {
		printk("Shows virtual -> physical mappings for a virt addr range.\n");
		printk("Usage: showmapping PID START_ADDR [END_ADDR]\n");
		printk("    PID == 0 for the boot pgdir\n");
		return 1;
	}
	pid = strtol(argv[1], 0, 10);
	if (!pid) {
		pgdir = boot_pgdir;
	} else {
		p = pid2proc(pid);
		if (!p) {
			printk("No proc with pid %d\n", pid);
			return 1;
		}
		pgdir = p->env_pgdir;
	}
	start = ROUNDDOWN(strtol(argv[2], 0, 16), PGSIZE);
	size = (argc == 3) ? 1 : strtol(argv[3], 0, 16) - start;
	if (size/PGSIZE > 512) {
		printk("Not going to do this for more than 512 items\n");
		return 1;
	}
	show_mapping(pgdir, start, size);
#endif
	return 0;
}

int mon_showmapping(int argc, char **argv, struct hw_trapframe *hw_tf)
{
	return __showmapping(argc, argv, hw_tf);
}

int mon_sm(int argc, char **argv, struct hw_trapframe *hw_tf)
{
	return __showmapping(argc, argv, hw_tf);
}
#if 0
static void print_info_handler(struct hw_trapframe *hw_tf, void *data)
{

	uint64_t tsc = read_tsc();

	spin_lock_irqsave(&print_info_lock);
	printk("----------------------------\n");
	printk("This is Core %d\n", core_id());
	printk("Timestamp = %lld\n", tsc);
#ifdef CONFIG_X86
	printk("Hardware core %d\n", hw_core_id());
	printk("MTRR_DEF_TYPE = 0x%08x\n", read_msr(IA32_MTRR_DEF_TYPE));
	printk("MTRR Phys0 Base = 0x%016llx, Mask = 0x%016llx\n",
	        read_msr(0x200), read_msr(0x201));
	printk("MTRR Phys1 Base = 0x%016llx, Mask = 0x%016llx\n",
	        read_msr(0x202), read_msr(0x203));
	printk("MTRR Phys2 Base = 0x%016llx, Mask = 0x%016llx\n",
	        read_msr(0x204), read_msr(0x205));
	printk("MTRR Phys3 Base = 0x%016llx, Mask = 0x%016llx\n",
	        read_msr(0x206), read_msr(0x207));
	printk("MTRR Phys4 Base = 0x%016llx, Mask = 0x%016llx\n",
	        read_msr(0x208), read_msr(0x209));
	printk("MTRR Phys5 Base = 0x%016llx, Mask = 0x%016llx\n",
	        read_msr(0x20a), read_msr(0x20b));
	printk("MTRR Phys6 Base = 0x%016llx, Mask = 0x%016llx\n",
	        read_msr(0x20c), read_msr(0x20d));
	printk("MTRR Phys7 Base = 0x%016llx, Mask = 0x%016llx\n",
	        read_msr(0x20e), read_msr(0x20f));
#endif // CONFIG_X86
	printk("----------------------------\n");
	spin_unlock_irqsave(&print_info_lock);

}
#endif
#if 0
static bool print_all_info(void)
{

	printk("\nCORE 0 asking all cores to print info:\n");
	smp_call_function_all(print_info_handler, NULL, 0);
	printk("\nDone!\n");
	return true;
}
#endif

int mon_cpuinfo(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	printk("Number of Cores detected: %d\n", num_cores);
	printk("Calling CPU's ID: 0x%08x\n", core_id());

	if (argc < 2)
		smp_call_function_self(print_info_handler, NULL, 0);
	else
		smp_call_function_single(strtol(argv[1], 0, 10),
		                         print_info_handler, NULL, 0);
#endif
	return 0;
}

int mon_manager(int argc, char** argv, struct hw_trapframe *hw_tf)
{
#if 0
	manager();
#endif
	panic("should never get here");
	return 0;
}

int mon_bin_ls(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	struct dirent dir = {0};
	struct file *bin_dir;
	int retval = 0;

	bin_dir = do_file_open("/bin", O_READ, 0);
	if (!bin_dir) {
		printk("No /bin directory!\n");
		return 1;
	}
	printk("Files in /bin:\n-------------------------------\n");
	do {
		retval = bin_dir->f_op->readdir(bin_dir, &dir);
		printk("%s\n", dir.d_name);
	} while (retval == 1);
	kref_put(&bin_dir->f_kref);
#endif
	return 0;
}

int mon_bin_run(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	if (argc < 2) {
		printk("Usage: bin_run FILENAME\n");
		return 1;
	}
	struct file *program;
	int retval = 0;
	char buf[5 + MAX_FILENAME_SZ + 1] = "/bin/";	/* /bin/ + max + \0 */

	strlcpy(buf, "/bin/", sizeof(buf));
	if (strlcat(buf, argv[1], sizeof(buf)) > sizeof(buf)) {
		printk("Filename '%s' too long!\n", argv[1]);
		return 1;
	}
	program = do_file_open(buf, O_READ, 0);
	if (!program) {
		printk("No such program!\n");
		return 1;
	}
	char **p_argv = kmalloc(sizeof(char*) * argc, 0);	/* bin_run's argc */
	for (int i = 0; i < argc - 1; i++)
		p_argv[i] = argv[i + 1];
	p_argv[argc - 1] = 0;
	/* super ugly: we need to stash current, so that proc_create doesn't pick up
	 * on random processes running here and assuming they are the parent */
	struct proc *old_cur = current;
	current = 0;
	struct proc *p = proc_create(program, p_argv, NULL);
	current = old_cur;
	kfree(p_argv);
	proc_wakeup(p);
	proc_decref(p); /* let go of the reference created in proc_create() */
	kref_put(&program->f_kref);
	/* Make a scheduling decision.  You might not get the process you created,
	 * in the event there are others floating around that are runnable */
	run_scheduler();
	/* want to idle, so we un the process we just selected.  this is a bit
	 * hackish, but so is the monitor. */
	smp_idle();
	assert(0);
#endif
	return 0;
}

int mon_procinfo(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	int verbosity = 0;

	if (argc < 2) {
		printk("Usage: procinfo OPTION\n");
		printk("\tall: show all active pids\n");
		printk("\tpid NUM: show a lot of info for proc NUM\n");
		printk("\tunlock: unlock the lock for the ADDR (OMG!!!)\n");
		printk("\tkill NUM: destroy proc NUM\n");
		return 1;
	}
	if (!strcmp(argv[1], "all")) {
		print_allpids();
	} else if (!strcmp(argv[1], "pid")) {
		if (argc < 3) {
			printk("Give me a pid number.\n");
			return 1;
		}
		if (argc >= 4)
			verbosity = strtol(argv[3], 0, 0);
		print_proc_info(strtol(argv[2], 0, 0), verbosity);
	} else if (!strcmp(argv[1], "unlock")) {
		if (argc != 3) {
			printk("Gimme lock address!  Me want lock address!.\n");
			return 1;
		}
		spinlock_t *lock = (spinlock_t*)strtol(argv[2], 0, 16);
		if (!lock) {
			printk("Null address...\n");
			return 1;
		}
		spin_unlock(lock);
	} else if (!strcmp(argv[1], "kill")) {
		if (argc != 3) {
			printk("Give me a pid number.\n");
			return 1;
		}
		struct proc *p = pid2proc(strtol(argv[2], 0, 0));
		if (!p) {
			printk("No such proc\n");
			return 1;
		}
		proc_destroy(p);
		proc_decref(p);
	} else {
		printk("Bad option\n");
		return 1;
	}
#endif
	return 0;
}

int mon_kill(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	struct proc *p;

	if (argc < 2) {
		printk("Usage: kill PID\n");
		return 1;
	}
	p = pid2proc(strtol(argv[1], 0, 0));
	if (!p) {
		printk("No such proc\n");
		return 1;
	}
	p->exitcode = 1;	/* typical EXIT_FAILURE */
	proc_destroy(p);
	proc_decref(p);
#endif
	return 0;
}

int mon_exit(int argc, char **argv, struct hw_trapframe *hw_tf)
{
	return -1;
}

int mon_kfunc(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	long ret;
	long (*func)(void *arg, ...);

	if (argc < 2) {
		printk("Usage: kfunc FUNCTION [arg1] [arg2] [etc]\n");
		printk("Use 0x with hex arguments.  Can take 6 args.\n");
		return 1;
	}
	func = (void*)get_symbol_addr(argv[1]);
	if (!func) {
		printk("Function not found.\n");
		return 1;
	}
	/* Not elegant, but whatever.  maybe there's a better syntax, or we can do
	 * it with asm magic. */
	switch (argc) {
	case 2: /* have to fake one arg */
		ret = func((void*)0);
		break;
	case 3: /* the real first arg */
		ret = func((void*)strtol(argv[2], 0, 0));
		break;
	case 4:
		ret = func((void*)strtol(argv[2], 0, 0),
		                  strtol(argv[3], 0, 0));
		break;
	case 5:
		ret = func((void*)strtol(argv[2], 0, 0),
		                  strtol(argv[3], 0, 0),
		                  strtol(argv[4], 0, 0));
		break;
	case 6:
		ret = func((void*)strtol(argv[2], 0, 0),
		                  strtol(argv[3], 0, 0),
		                  strtol(argv[4], 0, 0),
		                  strtol(argv[5], 0, 0));
		break;
	case 7:
		ret = func((void*)strtol(argv[2], 0, 0),
		                  strtol(argv[3], 0, 0),
		                  strtol(argv[4], 0, 0),
		                  strtol(argv[5], 0, 0),
		                  strtol(argv[6], 0, 0));
		break;
	case 8:
		ret = func((void*)strtol(argv[2], 0, 0),
		                  strtol(argv[3], 0, 0),
		                  strtol(argv[4], 0, 0),
		                  strtol(argv[5], 0, 0),
		                  strtol(argv[6], 0, 0),
		                  strtol(argv[7], 0, 0));
		break;
	default:
		printk("Bad number of arguments.\n");
		return -1;
	}
	printk("%s (might have) returned %p\n", argv[1], ret);
#endif
	return 0;
}
/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16


int onecmd(int argc, char *argv[], struct hw_trapframe *hw_tf) {
	int i;
	if (!argc)
		return -1;
	for (i = 0; commands[i].name; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, hw_tf);
	}
	return -1;
}

void __run_mon(uint32_t srcid, long a0, long a1, long a2)
{
	void monitor(void);
	monitor();
}

static int runcmd(char *real_buf, struct hw_trapframe *hw_tf) {
	char * buf = real_buf;
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			printk("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		//This will get fucked at runtime..... in the ASS
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; commands[i].name; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, hw_tf);
	}
	return 0;
}

static int getchar(void){
	while ((inb(0x3f8+5) & 0x1) == 0)
		;
	return inb(0x3f8);
}
static int readline(char *buf, int buflen, char *prompt, int _)
{
	char c;
	int cnt;
	memset(buf, 0, buflen);
	printk(prompt, _);
	for(cnt = 0; cnt < buflen - 1; cnt++) {
		c = getchar();
		if (c == '\r') {
			c = '\n';
		}
		
		outb(c, 0x3f8);
		if (c == '\n') {
			break;
		}
		buf[cnt] = c;
	}
	return cnt;
}

void monitor(void)
{
	#define MON_CMD_LENGTH 256
	char buf[MON_CMD_LENGTH];
	int cnt;

	/* they are always disabled, since we have this irqsave lock */
	printk("Type 'help' for a list of commands.\n");

	while (1) {
		/* on occasion, the kernel monitor can migrate (like if you run
		 * something that blocks / syncs and wakes up on another core) */
		cnt = readline(buf, MON_CMD_LENGTH, "ROS(Core %d)> ", 0);
		if (cnt > 0) {
			buf[cnt] = 0;
			if (runcmd(buf, NULL) < 0)
				break;
		}
	}
}
#if 0
static void show_msr(struct hw_trapframe *unused, void *v)
{

	int core = core_id();
	uint64_t val;
	uint32_t msr = *(uint32_t *)v;
	val = read_msr(msr);
	printk("%d: %08x: %016llx\n", core, msr, val);

	printk("complete me\n");
	return;
	
}
#endif
struct set {
	uint32_t msr;
	uint64_t val;
};
#if 0
static void set_msr(struct hw_trapframe *unused, void *v)
{

	int core = core_id();
	struct set *s = v;
	uint32_t msr = s->msr;
	uint64_t val = s->val;
	write_msr(msr, val);
	val = read_msr(msr);
	printk("%d: %08x: %016llx\n", core, msr, val);

	printk("complete write \n");
}
#endif
int mon_msr(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	
	uint64_t val;
	uint32_t msr;
	if (argc < 2 || argc > 3) {
		printk("Usage: msr register [value]\n");
		return 1;
	}
	msr = strtoul(argv[1], 0, 16);
	handler_wrapper_t *w;
	smp_call_function_all(show_msr, &msr, &w);
	smp_call_wait(w);

	if (argc < 3)
		return 0;
	/* somewhat bogus on 32 bit. */
	val = strtoul(argv[2], 0, 16);

	struct set set;
	set.msr = msr;
	set.val = val;
	smp_call_function_all(set_msr, &set, &w);
	smp_call_wait(w);
	printk("complete me\n");
#endif
	return 0;
}

/* Prints info about a core.  Optional first arg == coreid. */
int mon_coreinfo(int argc, char **argv, struct hw_trapframe *hw_tf)
{
#if 0
	struct per_cpu_info *pcpui;
	struct kthread *kth;
	int coreid = core_id();

	if (argc >= 2)
		coreid = strtol(argv[1], 0, 0);
	pcpui = &per_cpu_info[coreid];
	printk("Core %d:\n\tcur_proc %d\n\towning proc %d, owning vc %d\n",
	       coreid, pcpui->cur_proc ? pcpui->cur_proc->pid : 0,
	       pcpui->owning_proc ? pcpui->owning_proc->pid : 0,
	       pcpui->owning_vcoreid != 0xdeadbeef ? pcpui->owning_vcoreid : 0);
	kth = pcpui->cur_kthread;
	if (kth) {
		/* kth->proc is only used when the kthread is sleeping.  when it's
		 * running, we care about cur_proc.  if we're here, proc should be 0
		 * unless the kth is concurrently sleeping (we called this remotely) */
		printk("\tkthread %p (%s), sysc %p (%d)\n", kth, kth->name,
		       kth->sysc, kth->sysc ? kth->sysc->num : -1);
	} else {
		/* Can happen during early boot */
		printk("\tNo kthread!\n");
	}
#endif
	return 0;
}

static command_t commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "reboot", "Take a ride to the South Bay", mon_reboot },
	{ "showmapping", "Shows VA->PA mappings", mon_showmapping},
	{ "sm", "Shows VA->PA mappings", mon_sm},
	{ "cpuinfo", "Prints CPU diagnostics", mon_cpuinfo},
	{ "exit", "Leave the monitor", mon_exit},
	{ "e", "Leave the monitor", mon_exit},
	{ "msr", "read/write msr: msr msr [value]", mon_msr},
	{ "coreinfo", "Print diagnostics for a core", mon_coreinfo},
	{},
};

