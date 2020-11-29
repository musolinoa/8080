#include <u.h>
#include <libc.h>
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
	case B: return "B";
	case C: return "C";
	case D: return "D";
	case E: return "E";
	case H: return "H";
	case L: return "L";
	case M: return "M";
	case A: return "A";
	}
	return "X";
}

char*
rpnam(u8int r)
{
	switch(r){
	case BC: return "BC";
	case DE: return "DE";
	case HL: return "HL";
	}
	return "XX";
}

void
dumpinst(void)
{
	fprint(2, "op: %#.2x [%#.2x %#.2x]\n",
		mem[insn.pc+0], mem[insn.pc+1], mem[insn.pc+2]);
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

void
dumpregs(void)
{
	int i;

	fprint(2, "A=%#.2x\n", cpu.r[A]);
	fprint(2, "B=%#.2x\n", cpu.r[B]);
	fprint(2, "C=%#.2x\n", cpu.r[C]);
	fprint(2, "D=%#.2x\n", cpu.r[D]);
	fprint(2, "E=%#.2x\n", cpu.r[E]);
	fprint(2, "H=%#.2x\n", cpu.r[H]);
	fprint(2, "L=%#.2x\n", cpu.r[L]);
	fprint(2, "F=%#.2x", cpu.flg);
	if(cpu.flg != 0){
		fprint(2, " (");
		for(i = 0; i < 8; i++)
			if((cpu.flg&1<<i) != 0)
				fprint(2, "%c", flagchar(i));
		fprint(2, ")");
	}
	fprint(2, "\n");
	fprint(2, "PC=%#.4x\n", cpu.PC);
	fprint(2, "SP=%#.4x\n", cpu.SP);
}

void
dumpmem(u16int s, u16int e)
{
	while(s < e){
		fprint(2, "%.4x: %.2x\n", s, mem[s]);
		s++;
	}
}

void
itrace0(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprint(2, "%#.4x ", insn.pc);
	vfprint(2, fmt, args);
	fprint(2, "\n");
	va_end(args);
}

void
fatal(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprint(2, fmt, args);
	fprint(2, "\n");
	va_end(args);
	dumpinst();
	dumpregs();
	exits("fatal");
}
