#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include "dat.h"
#include "fns.h"

jmp_buf trapjmp;

static void
deltrap(void *u, char *msg)
{
	if(strcmp(msg, "interrupt") == 0){
		notejmp(u, trapjmp, 1);
		noted(NCONT);
	}
	noted(NDFLT);
}

void
trapinit(void)
{
	static int init = 0;
	if(init == 0){
		notify(deltrap);
		init++;
	}
}

void
trap(void)
{
	longjmp(trapjmp, 1);
}

char*
rnam(u8int r)
{
	switch(r){
	case RB: return "B";
	case RC: return "C";
	case RD: return "D";
	case RE: return "E";
	case RH: return "H";
	case RL: return "L";
	case RM: return "M";
	case RA: return "A";
	}
	return "X";
}

char*
rpnam(u8int r)
{
	switch(r){
	case RBC: return "BC";
	case RDE: return "DE";
	case RHL: return "HL";
	}
	return "XX";
}

static void
dumpinst0(void)
{
	Bprint(stderr, "op: %#.2x [%#.2x %#.2x]\n",
		mem[insn.pc+0], mem[insn.pc+1], mem[insn.pc+2]);
}

void
dumpinst(void)
{
	dumpinst0();
	Bflush(stderr);
}

static char
flagchar(int f)
{
	switch(1<<f){
	case Fcarry:	return 'C';
	case Fparity:	return 'P';
	case Fhcarry:	return 'H';
	case Fzero:		return 'Z';
	case Fsign:		return 'S';
	}
	return 0;
}

static void
dumpregs0(void)
{
	int i;

	Bprint(stderr, "A=%#.2x\n", cpu.r[RA]);
	Bprint(stderr, "B=%#.2x\n", cpu.r[RB]);
	Bprint(stderr, "C=%#.2x\n", cpu.r[RC]);
	Bprint(stderr, "D=%#.2x\n", cpu.r[RD]);
	Bprint(stderr, "E=%#.2x\n", cpu.r[RE]);
	Bprint(stderr, "H=%#.2x\n", cpu.r[RH]);
	Bprint(stderr, "L=%#.2x\n", cpu.r[RL]);
	Bprint(stderr, "F=%#.2x", cpu.flg);
	if(cpu.flg != 0){
		Bprint(stderr, " (");
		for(i = 0; i < 8; i++)
			if((cpu.flg&1<<i) != 0)
				Bprint(stderr, "%c", flagchar(i));
		Bprint(stderr, ")");
	}
	Bprint(stderr, "\n");
	Bprint(stderr, "PC=%#.4x\n", cpu.PC);
	Bprint(stderr, "SP=%#.4x\n", cpu.SP);
}

void
dumpregs(void)
{
	dumpregs0();
	Bflush(stderr);
}

void
dumpmem(u16int s, u16int e)
{
	while(s < e){
		Bprint(stderr, "%.4x: %.2x\n", s, mem[s]);
		s++;
	}
	Bflush(stderr);
}

void
itrace0(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	Bprint(stderr, "%#.4x ", insn.pc);
	Bvprint(stderr, fmt, args);
	Bprint(stderr, "\n");
	Bflush(stderr);
	va_end(args);
}

void
fatal(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	Bvprint(stderr, fmt, args);
	Bprint(stderr, "\n");
	va_end(args);
	dumpinst0();
	dumpregs0();
	Bflush(stderr);
	threadexitsall("fatal");
}
