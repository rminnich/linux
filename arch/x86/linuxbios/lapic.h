#ifndef CPU_X86_LAPIC_H
#define CPU_X86_LAPIC_H

#include "lapic_def.h"

static inline __attribute__((always_inline)) unsigned long lapic_read(
	unsigned long reg)
{
	return *((volatile unsigned long *)(LAPIC_DEFAULT_BASE+reg));
}

static inline __attribute__((always_inline)) void lapic_write(unsigned long reg,
	unsigned long v)
{
	*((volatile unsigned long *)(LAPIC_DEFAULT_BASE+reg)) = v;
}

static inline __attribute__((always_inline)) void lapic_wait_icr_idle(void)
{
	do { } while (lapic_read(LAPIC_ICR) & LAPIC_ICR_BUSY);
}

static inline void enable_lapic(void)
{

	u64 msr;
	rdmsrl(LAPIC_BASE_MSR, msr);
	msr &= 0xffffff0000000000ULL;
	msr &= 0x000007ff;
	msr |= LAPIC_DEFAULT_BASE | (1 << 11);
	wrmsrl(LAPIC_BASE_MSR, msr);
}

static inline void disable_lapic(void)
{
	u64 msr;
	rdmsrl(LAPIC_BASE_MSR, msr);
	msr &= ~(1 << 11);
	wrmsrl(LAPIC_BASE_MSR, msr);
}

static inline __attribute__((always_inline)) unsigned long lapicid(void)
{
	return lapic_read(LAPIC_ID) >> 24;
}

static inline void lapic_write_atomic(unsigned long reg, unsigned long v)
{
	(void)xchg((volatile unsigned long *)(LAPIC_DEFAULT_BASE+reg), v);
}



# define FORCE_READ_AROUND_WRITE 0
# define lapic_read_around(x) lapic_read(x)
# define lapic_write_around(x, y) lapic_write((x), (y))

static inline int lapic_remote_read(int apicid, int reg, unsigned long *pvalue)
{
	int timeout;
	unsigned long status;
	int result;
	lapic_wait_icr_idle();
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(apicid));
	lapic_write_around(LAPIC_ICR, LAPIC_DM_REMRD | (reg >> 4));
	timeout = 0;
	do {
#if 0
		udelay(100);
#endif
		status = lapic_read(LAPIC_ICR) & LAPIC_ICR_RR_MASK;
	} while (status == LAPIC_ICR_RR_INPROG && timeout++ < 1000);

	result = -1;
	if (status == LAPIC_ICR_RR_VALID) {
		*pvalue = lapic_read(LAPIC_RRR);
		result = 0;
	}
	return result;
}

void setup_lapic(void);

#endif /* CPU_X86_LAPIC_H */
