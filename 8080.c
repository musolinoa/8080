#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <draw.h>
#include <thread.h>
#include "dat.h"
#include "fns.h"

Biobuf *stdin;
Biobuf *stdout;
Biobuf *stderr;

CPU ocpu, cpu;
Insn insn;

enum{
	Width = 256,
	Height = 224,
};

uchar mem[MEMSZ];
uchar *rom = &mem[0];
uchar *ram = &mem[ROMSZ];
uchar *vmem = &mem[ROMSZ+RAMSZ];

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
	fprint(2, "usage: %s [-b addr] [-i script] [-t addr] [-T addr]\n", argv0);
	threadexitsall("usage");
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

	for(i = 0; i < nbrkpts; i++){
		if(brkpts[i].enabled && brkpts[i].hit == 0 && cpu.PC == brkpts[i].addr){
			Bprint(stderr, "hit breakpoint %d @ pc=%#.4uhx\n", i+1, cpu.PC);
			Bflush(stderr);
			brkpts[i].hit = 1;
			trap();
		}
	}

	insn.pc = cpu.PC;
	insn.op = decodeop(buf[0] = ifetch(&cpu));
	if(insn.op == -1){
		Bprint(stderr, "illegal opcode %#.2uhhx @ pc=%#.4uhx\n", buf[0], cpu.PC);
		Bflush(stderr);
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
			brkpts[i].hit = 0;

	for(i = 0; i < ntraceops; i++)
		if(cpu.PC == traceops[i].addr)
			if(traceops[i].op == Tpop){
				dprint("@@@@ Tpop @ %2x\n", traceops[i].addr);
				tracing--;
			}

	ocpu = cpu;
}

enum
{
	Cerror = -1,
	Cnone,
	Cdas,
	Cbpls,
	Cbpset,
	Cbpdel,
	Cload,
	Creg,
	Crun,
	Cstep,
	Cexit,
	Creset,
};

static char *cmdtab[] = {
	[Cnone] "",
	[Cdas] "das",
	[Cbpls] "bpls",
	[Cbpset] "bpset",
	[Cbpdel] "bpdel",
	[Cload] "load",
	[Creg] "reg",
	[Crun] "run",
	[Cstep] "step",
	[Cexit] "exit",
	[Creset] "reset",
};

typedef struct Cmd Cmd;
struct Cmd
{
	char *buf;
	int argc;
	char *argv[16];
};

static int
cmd(char *s)
{
	int i;

	for(i = 0; i < nelem(cmdtab); i++){
		if(strcmp(s, cmdtab[i]) == 0)
			return i;
	}
	return Cerror;
}

static void
docmd(Cmd *c)
{
	int i;

	switch(cmd(c->argv[0])){
	case Cerror:
		Bprint(stderr, "unknown command: %s\n", c->argv[0]);
		Bflush(stderr);
		break;
	case Cnone:
		break;
	case Cdas:
		das(mem+cpu.PC,mem+nelem(mem), -1);
		break;
	case Cbpls:
		if(c->argc != 1){
			Bprint(stderr, "usage: bpls\n");
			Bflush(stderr);
			break;
		}
		for(i = 0; i < nbrkpts; i++){
			if(brkpts[i].enabled == 0)
				continue;
			Bprint(stdout, "bp %d @ %#.4uhx\n", i+1, brkpts[i].addr);
		}
		Bflush(stdout);
		break;
	case Cbpset:
		if(c->argc != 2){
			Bprint(stderr, "usage: bpset ADDR\n");
			Bflush(stderr);
			break;
		}
		if(nbrkpts >= MAXBRKPTS){
			Bprint(stderr, "too many breakpoints");
			Bflush(stderr);
			break;
		}
		brkpts[nbrkpts].addr = strtoul(c->argv[1], 0, 0);
		brkpts[nbrkpts].enabled = 1;
		nbrkpts++;
		break;
	case Cbpdel:
		if(c->argc != 2){
			Bprint(stderr, "usage: bpdel NUM\n");
			Bflush(stderr);
			break;
		}
		i = strtoul(c->argv[1], 0, 0);
		if(i < 1 || i > nbrkpts){
			Bprint(stderr, "bad breakpoint number\n");
			Bflush(stderr);
			break;
		}
		if(brkpts[i-1].enabled == 0){
			Bprint(stderr, "no such breakpoint\n");
			Bflush(stderr);
			break;
		}
		brkpts[i-1].enabled = 0;
		break;
	case Cload:
		if(c->argc != 2){
			Bprint(stderr, "usage: load ROM\n");
			Bflush(stderr);
			break;
		}
		if(loadrom(c->argv[1], 0) < 0){
			Bprint(stderr, "loading %s failed: %r\n", c->argv[1]);
			Bflush(stderr);
		}
		break;
	case Creg:
		dumpregs();
		break;
	case Crun:
		if(c->argc != 1){
			Bprint(stderr, "usage: run\n");
			Bflush(stderr);
			break;
		}
		if(wastrap()){
			Bflush(stdout);
			cpu = ocpu;
		}else{
			for(;;){
				//Bprint(stdout, "%#.4uhx\t", cpu.PC);
				//das1(mem+cpu.PC,nelem(mem)-cpu.PC);
				cpustep();
			}
		}
		break;
	case Cstep:
		if(wastrap()){
			cpu = ocpu;
		}else{
			print("%#.4uhx\t", cpu.PC);
			das1(mem+cpu.PC,nelem(mem)-cpu.PC);
			cpustep();
		}
		break;
	case Cexit:
		threadexitsall(0);
		break;
	case Creset:
		cpureset();
		break;
	}
}

static Cmd prev;

static int
proc(Biobuf *in)
{
	char *buf;
	Cmd cmd;

	trapinit();
	buf = Brdstr(in, '\n', 0);
	Bflush(stderr);
	if(Blinelen(in) <= 0)
		return Blinelen(in);
	while(isspace(*buf))
		buf++;
	if(*buf == '#')
		return Blinelen(in);
	cmd.argc = tokenize(buf, cmd.argv, nelem(cmd.argv));
	if(cmd.argc == 0){
		free(buf);
		if(prev.argc > 0)
			docmd(&prev);
	}else{
		cmd.buf = buf;
		docmd(&cmd);
		free(prev.buf);
		prev = cmd;
	}
	return Blinelen(in);
}

static void
procfile(char *file)
{
	Biobuf *b;

	b = Bopen(file, OREAD);
	if(b == nil)
		sysfatal("could not open %s: %r", file);
	for(;;){
		switch(proc(b)){
		case -1:
			sysfatal("error processing %s", file);
		case 0:
			Bterm(b);
			return;
		}
	}
}

static int
isatty(void)
{
	char buf[64];

	if(fd2path(0, buf, sizeof(buf)) != 0)
		return 0;
	return strcmp(buf, "/dev/cons") == 0;
}

static void
resize(int w, int h)
{
	int fd;

	if((fd = open("/dev/wctl", OWRITE)) < 0)
		return;
	fprint(fd, "resize -dx %d -dy %d", w, h);
	close(fd);
}

static void
scanout(void*)
{
	int i;
	uchar *p;
	Image *line;
	Rectangle r;

	line = allocimage(display, Rect(0,0,Width,1), GREY1, 0, DNofill);

	for(;;){
		p = vmem;
		r.min = Pt(screen->r.min.x, screen->r.max.y-1);
		r.max = Pt(screen->r.max.x, screen->r.max.y);
		for(i = 0; i < Height/2; i++){
			loadimage(line, Rect(0,0,Width,1), p, Width/8);
			draw(screen, r, line, nil, ZP);
			r = rectaddpt(r, Pt(0, -1));
			p += Width/8;
		}
		flushimage(display, 1);
		sleep(1000/30);
		for(; i < Height; i++){
			loadimage(line, Rect(0,0,Width,1), p, Width/8);
			draw(screen, r, line, nil, ZP);
			r = rectaddpt(r, Pt(0, -1));
			p += Width/8;
		}
		flushimage(display, 1);
		sleep(1000/30);
	}
}

void
threadmain(int argc, char **argv)
{
	stdin = Bfdopen(0, OREAD);
	stdout = Bfdopen(1, OWRITE);
	stderr = Bfdopen(2, OWRITE);

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
	case 'i':
		procfile(EARGF(usage()));
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
	if(newwindow(nil) < 0)
		sysfatal("newwindow: %r");
	resize(Width, Height);
	if(initdraw(nil, nil, nil) < 0)
		sysfatal("initdraw: %r");
	proccreate(scanout, nil, 1024);
	interactive = isatty();
	fmtinstall('I', insnfmt);
	cpureset();
	for(;;){
		if(interactive){
			Bprint(stdout, "8080> ");
			Bflush(stdout);
		}
		switch(proc(stdin)){
		case -1:
			sysfatal("error reading stdin: %r");
		case 0:
			threadexitsall(0);
		}
	}
}
