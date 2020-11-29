#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

CPU ocpu, cpu;
Insn insn;

uchar mem[MEMSZ];
uchar *rom = &mem[0];
uchar *ram = &mem[ROMSZ];
uchar *vid = &mem[ROMSZ+RAMSZ];

int interactive;
int debug;
int nbrkpts;
int tracing;
int ntraceops;
TraceOp traceops[MAXTRACEOPS];
BrkPt brkpts[MAXBRKPTS];

static void
usage(void)
{
	fprint(2, "usage: %s [-b addr] rom\n", argv0);
	exits("usage");
}

int
loadrom(char *file, u16int base)
{
	uchar *mp, *romend;
	int c;
	Biobuf *r;

	r = Bopen(file, OREAD);
	if(r == nil)
		return -1;
	romend = rom + ROMSZ;
	for(;;){
		for(mp = &rom[base]; mp < romend; mp++){
			c = Bgetc(r);
			if(c < 0)
				goto Done;
			*mp = c;
		}
	}
Done:
	Bterm(r);
	return 0;
}

static void
cpureset(void)
{
	memset(&ocpu, 0, sizeof(ocpu));
	memset(&cpu, 0, sizeof(cpu));
}

static void
cpustep(void)
{
	int i;
	int ilen;
	uchar buf[3];
	static u64int counter;

	for(i = 0; i < ntraceops; i++)
		if(cpu.PC == traceops[i].addr)
			if(traceops[i].op == Tpush){
				dprint("@@@@ Tpush @ %2x\n", traceops[i].addr);
				tracing++;
			}

	for(i = 0; i < nbrkpts; i++)
		if(brkpts[i].enabled && cpu.PC == brkpts[i].addr){
			brkpts[i].enabled = 0;
			trap();
		}

	insn.pc = cpu.PC;
	insn.op = decodeop(buf[0] = ifetch(&cpu));
	if(insn.op == -1){
		fprint(2, "illegal opcode %#.2uhhx @ pc=%#.4uhx\n", buf[0], insn.pc);
		trap();
	}
	switch(ilen = insnlen(insn.op)){
	case 2:
		buf[1] = ifetch(&cpu);
		break;
	case 3:
		buf[1] = ifetch(&cpu);
		buf[2] = ifetch(&cpu);
		break;
	}
	decodeinsn(&insn, buf, ilen);
	cpuexec(&cpu, &insn);

	counter++;
	if((cpu.intr&Ienabled) != 0){
		if(counter % 512 == 0){
			push16(&cpu, cpu.PC);
			cpu.PC = 2<<3;
		}else if(counter % 256 == 0){
			push16(&cpu, cpu.PC);
			cpu.PC = 1<<3;
		}
	}

	for(i = 0; i < nbrkpts; i++)
		if(cpu.PC == brkpts[i].addr)
			brkpts[i].enabled = 1;

	for(i = 0; i < ntraceops; i++)
		if(cpu.PC == traceops[i].addr)
			if(traceops[i].op == Tpop){
				dprint("@@@@ Tpop @ %2x\n", traceops[i].addr);
				tracing--;
			}

	ocpu = cpu;
}

static void
prompt(void)
{
	static char prev[256] = "";
	int n;
	char buf[256];

	trapinit();
	if(interactive)
		print("8080> ");
	n = read(0, buf, sizeof(buf) - 1);
	if(n <= 0)
		exits("eof");
	if(buf[n-1] != '\n')
		exits("nl");
	buf[n-1] = 0;
	if(strcmp(buf, "") == 0){
		if(strcmp(prev, "") == 0)
			return;
		strcpy(buf, prev);
	}
	if(strcmp(buf, "das") == 0){
		//das(&insn, mem);
	}else if(strcmp(buf, "bpset") == 0){
	}else if(strcmp(buf, "load") == 0){
		if(loadrom("invaders.rom", 0) < 0)
			fprint(2, "load failed: %r\n");
	}else if(strcmp(buf, "reg") == 0){
		dumpregs();
	}else if(strcmp(buf, "run") == 0){
		if(wastrap())
			cpu = ocpu;
		else
			for(;;){
				//print("%#.4uhx\t", cpu.PC);
				//das(mem+cpu.PC,nelem(mem));
				cpustep();
			}
	}else if(strcmp(buf, "step") == 0){
		if(wastrap())
			cpu = ocpu;
		else{
			print("%#.4uhx\t", cpu.PC);
			das(mem+cpu.PC,nelem(mem));
			cpustep();
		}
	}else if(strcmp(buf, "exit") == 0){
		exits(0);
	}else if(strcmp(buf, "reset") == 0){
		cpureset();
	}else{
		fprint(2, "unknown command: %s\n", buf);
		buf[0] = 0;
	}
	strcpy(prev, buf);
}

static int
isatty(void)
{
	char buf[64];

	if(fd2path(0, buf, sizeof(buf)) != 0)
		return 0;
	return strcmp(buf, "/dev/cons") == 0;
}

void
main(int argc, char **argv)
{
	ARGBEGIN{
	case 'b':
		if(nbrkpts >= MAXBRKPTS)
			sysfatal("too many breakpoints");
		brkpts[nbrkpts].addr = strtoul(EARGF(usage()), 0, 0);
		brkpts[nbrkpts].enabled = 1;
		nbrkpts++;
		break;
	case 'd':
		debug++;
		break;
	case 't':
	case 'T':
		if(ntraceops >= MAXTRACEOPS)
			sysfatal("too many trace ops");
		traceops[ntraceops].addr = strtoul(EARGF(usage()), 0, 0);
		traceops[ntraceops].op = ARGC() == 'T' ? Tpush : Tpop;
		ntraceops++;
		break;
	default:
		usage();
	}ARGEND;
	if(argc != 0)
		usage();
	interactive = isatty();
	fmtinstall('I', insnfmt);
	cpureset();
	if(loadrom("invaders.rom", 0) < 0)
		fprint(2, "load failed: %r\n");
	for(;;)
		prompt();
}
