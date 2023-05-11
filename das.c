#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

static void Xadi(CPU*, Insn*);
static void Xana(CPU*, Insn*);
static void Xani(CPU*, Insn*);
static void Xcall(CPU*, Insn*);
static void Xcmp(CPU*, Insn*);
static void Xcpi(CPU*, Insn*);
static void Xcz(CPU*, Insn*);
static void Xdaa(CPU*, Insn*);
static void Xdad(CPU*, Insn*);
static void Xdcr(CPU*, Insn*);
static void Xdcx(CPU*, Insn*);
static void Xdi(CPU*, Insn*);
static void Xei(CPU*, Insn*);
static void Xin(CPU*, Insn*);
static void Xinr(CPU*, Insn*);
static void Xinx(CPU*, Insn*);
static void Xjc(CPU*, Insn*);
static void Xjm(CPU*, Insn*);
static void Xjmp(CPU*, Insn*);
static void Xjnc(CPU*, Insn*);
static void Xjnz(CPU*, Insn*);
static void Xjp(CPU*, Insn*);
static void Xjz(CPU*, Insn*);
static void Xlda(CPU*, Insn*);
static void Xldax(CPU*, Insn*);
static void Xlxi(CPU*, Insn*);
static void Xmov(CPU*, Insn*);
static void Xmvi(CPU*, Insn*);
static void Xnop(CPU*, Insn*);
static void Xora(CPU*, Insn*);
static void Xori(CPU*, Insn*);
static void Xout(CPU*, Insn*);
static void Xpchl(CPU*, Insn*);
static void Xpop(CPU*, Insn*);
static void Xpush(CPU*, Insn*);
static void Xrar(CPU*, Insn*);
static void Xrc(CPU*, Insn*);
static void Xret(CPU*, Insn*);
static void Xrim(CPU*, Insn*);
static void Xrlc(CPU*, Insn*);
static void Xrnc(CPU*, Insn*);
static void Xrnz(CPU*, Insn*);
static void Xrrc(CPU*, Insn*);
static void Xrz(CPU*, Insn*);
static void Xsbi(CPU*, Insn*);
static void Xshld(CPU*, Insn*);
static void Xsta(CPU*, Insn*);
static void Xstax(CPU*, Insn*);
static void Xsui(CPU*, Insn*);
static void Xxchg(CPU*, Insn*);
static void Xxra(CPU*, Insn*);
static void Xxthl(CPU*, Insn*);

static int dec0(Insn*, uchar*, long);
static int decaddr(Insn*, uchar*, long);
static int decimm(Insn*, uchar*, long);
static int decr00000xxx(Insn*, uchar*, long);
static int decr00xxx000(Insn*, uchar*, long);
static int decrimm(Insn*, uchar*, long);
static int decrp(Insn*, uchar*, long);
static int decrpimm(Insn*, uchar*, long);
static int decrr(Insn*, uchar*, long);

static int das0(Fmt*, Insn*);
static int dasaddr(Fmt*, Insn*);
static int dasimm(Fmt*, Insn*);
static int dasr(Fmt*, Insn*);
static int dasrimm(Fmt*, Insn*);
static int dasrp(Fmt*, Insn*);
static int dasrpimm(Fmt*, Insn*);
static int dasrr(Fmt*, Insn*);

static InsnType insntypes[] = {
	[T0]		= { 1, das0, dec0 },
	[Taddr] 	= { 3, dasaddr, decaddr },
	[Timm]		= { 2, dasimm, decimm },
	[Tr012]		= { 1, dasr, decr00000xxx },
	[Tr345]		= { 1, dasr, decr00xxx000 },
	[Trimm]		= { 2, dasrimm, decrimm },
	[Trp]		= { 1, dasrp, decrp },
	[Trpimm]	= { 3, dasrpimm, decrpimm },
	[Trr]		= { 1, dasrr, decrr },
};

static ISA isa[] = {
	[Oadc]{"ADC", Tr012},
	[Oadd]{"ADD", Tr012},
	[Oadi]{"ADI", Timm, Xadi},
	[Oana]{"ANA", Tr012, Xana},
	[Oani]{"ANI", Timm, Xani},
	[Ocall]{"CALL", Taddr, Xcall},
	[Ocm]{"CM", Taddr},
	[Ocma]{"CMA", T0},
	[Ocmc]{"CMC", T0},
	[Ocmp]{"CMP", Tr012, Xcmp},
	[Ocnc]{"CNC", Taddr},
	[Ocpe]{"CPE", Taddr},
	[Ocpi]{"CPI", Timm, Xcpi},
	[Ocz]{"CZ", Taddr, Xcz},
	[Odaa]{"DAA", T0, Xdaa},
	[Odad]{"DAD", Trp, Xdad},
	[Odcr]{"DCR", Tr345, Xdcr},
	[Odcx]{"DCX", Trp, Xdcx},
	[Odi]{"DI", T0, Xdi},
	[Oei]{"EI", T0, Xei},
	[Oin]{"IN", Timm, Xin},
	[Oinr]{"INR", Tr345, Xinr},
	[Oinx]{"INX", Trp, Xinx},
	[Ojc]{"JC", Taddr, Xjc},
	[Ojm]{"JM", Taddr, Xjm},
	[Ojmp]{"JMP", Taddr, Xjmp},
	[Ojnc]{"JNC", Taddr, Xjnc},
	[Ojnz]{"JNZ", Taddr, Xjnz},
	[Ojp]{"JP", Taddr, Xjp},
	[Ojpo]{"JPO", Taddr},
	[Ojz]{"JZ", Taddr, Xjz},
	[Olda]{"LDA", Taddr, Xlda},
	[Oldax]{"LDAX", Trp, Xldax},
	[Olxi]{"LXI", Trpimm, Xlxi},
	[Omov]{"MOV", Trr, Xmov},
	[Omvi]{"MVI", Trimm, Xmvi},
	[Onop]{"NOP", T0, Xnop},
	[Oora]{"ORA", Tr012, Xora},
	[Oori]{"ORI", Timm, Xori},
	[Oout]{"OUT", Timm, Xout},
	[Opchl]{"PCHL", T0, Xpchl},
	[Opop]{"POP", Trp, Xpop},
	[Opush]{"PUSH", Trp, Xpush},
	[Orar]{"RAR", T0, Xrar},
	[Orc]{"RC", T0, Xrc},
	[Oret]{"RET", T0, Xret},
	[Orim]{"RIM", T0},
	[Orlc]{"RLC", T0, Xrlc},
	[Orm]{"RM", T0},
	[Ornc]{"RNC", T0, Xrnc},
	[Ornz]{"RNZ", T0, Xrnz},
	[Orp]{"RP", T0},
	[Orpo]{"RPO", T0},
	[Orrc]{"RRC", T0, Xrrc},
	[Orst]{"RST", Timm},
	[Orz]{"RZ", T0, Xrz},
	[Osbb]{"SBB", Tr012},
	[Osbi]{"SBI", Timm, Xsbi},
	[Oshld]{"SHLD", Taddr, Xshld},
	[Osim]{"SIM", T0},
	[Osta]{"STA", Taddr, Xsta},
	[Ostax]{"STAX", Trp, Xstax},
	[Osub]{"SUB", Tr012},
	[Osui]{"SUI", Timm, Xsui},
	[Oxchg]{"XCHG", T0, Xxchg},
	[Oxra]{"XRA", Tr012, Xxra},
	[Oxri]{"XRI", Timm},
	[Oxthl]{"XTHL", T0, Xxthl},
};

static char*
opstr(int op)
{
	if(op < 0 || op >= nelem(isa))
		return "XXX";
	return isa[op].opstr;
}

static int
das0(Fmt *fmt, Insn *insn)
{
	return fmtprint(fmt, "%s", opstr(insn->op));
}

static int
dasr(Fmt *fmt, Insn *insn)
{
	return fmtprint(fmt, "%s %s", opstr(insn->op), rnam(insn->r1));
}

static int
dasrr(Fmt *fmt, Insn *insn)
{
	return fmtprint(fmt, "%s %s, %s", opstr(insn->op),
		rnam(insn->r1), rnam(insn->r2));
}

static int
dasimm(Fmt *fmt, Insn *insn)
{
	char *fmtstr;

	if(insn->op == Orst)
		fmtstr = "%s %uhhd";
	else
		fmtstr = "%s #$%#.2uhhx";
	return fmtprint(fmt, fmtstr, opstr(insn->op), insn->imm);
}

static int
dasrimm(Fmt *fmt, Insn *insn)
{
	return fmtprint(fmt, "%s %s, #$%#.2uhhx", opstr(insn->op), rnam(insn->r1), insn->imm);
}

static int
dasrp(Fmt *fmt, Insn *insn)
{
	char *rp;

	rp = rpnam(insn->rp);
	if(insn->rp == 3){
		switch(insn->op){
		case Opush:
		case Opop:
			rp = "PSW";
			break;
		case Odad:
			rp = "SP";
			break;
		}
	}
	return fmtprint(fmt, "%s %s", opstr(insn->op), rp);
}

static int
dasaddr(Fmt *fmt, Insn *insn)
{
	return fmtprint(fmt, "%s %#.4x", opstr(insn->op), insn->addr);
}

static int
dasrpimm(Fmt *fmt, Insn *insn)
{
	return fmtprint(fmt, "%s %s, #$0x%.2uhhx%.2uhhx", opstr(insn->op),
		insn->rp == 3 ? "SP" : rpnam(insn->rp), insn->imm1, insn->imm);
}

static int
dec0(Insn*, uchar*, long)
{
	return 1;
}

static int
decr00000xxx(Insn *insn, uchar *mem, long)
{
	insn->r1 = mem[0]&0x7;
	return 1;
}

static int
decr00xxx000(Insn *insn, uchar *mem, long)
{
	insn->r1 = (mem[0]>>3)&0x7;
	return 1;
}

static int
decrimm(Insn *insn, uchar *mem, long len)
{
	if(len < 2)
		return 0;
	insn->r1 = (mem[0]>>3)&0x7;
	insn->imm = mem[1];
	return 2;
}

static int
decimm(Insn *insn, uchar *mem, long len)
{
	if(len < 2)
		return 0;
	insn->imm = mem[1];
	return 2;
}

static int
decaddr(Insn *insn, uchar *mem, long len)
{
	if(len < 3)
		return 0;
	insn->addr = (u8int)mem[1]|((u16int)mem[2])<<8;
	return 3;
}

static int
decrr(Insn *insn, uchar *mem, long)
{
	insn->r1 = (mem[0]>>3)&0x7;
	insn->r2 = mem[0]&0x7;
	if(insn->r1 == RM && insn->r2 == RM)
		insn->op = Onop;
	return 1;
}

static int
decrp(Insn *insn, uchar *mem, long)
{
	insn->rp = (mem[0]>>4)&0x3;
	return 1;
}

static int
decrpimm(Insn *insn, uchar *mem, long len)
{
	if(len < 3)
		return 0;
	insn->rp = (mem[0]>>4)&0x3;
	insn->imm = mem[1];
	insn->imm1 = mem[2];
	return 3;
}

int
insnfmt(Fmt *fmt)
{
	Insn *insn;

	insn = va_arg(fmt->args, Insn*);
	if(insn->op < 0 || insn->op >= nelem(isa))
		return fmtprint(fmt, "XXX");
	return insntypes[isa[insn->op].type].das(fmt, insn);
}

int
insnlen(u8int op)
{
	if(op >= nelem(isa))
		return 0;
	return insntypes[isa[op].type].len;
}

int
decodeop(u8int b)
{
	if((b&0xc0) == 0x40)
		return Omov;
	switch(b){
	case 0x0f:	return Orrc;
	case 0x1f:	return Orar;
	case 0x27:	return Odaa;
	case 0x2f:	return Ocma;
	case 0x3f:	return Ocmc;
	case 0xc0:	return Ornz;
	case 0xc2:	return Ojnz;
	case 0xc3:	return Ojmp;
	case 0xc4:	return Ocz;
	case 0xc6:	return Oadi;
	case 0xc8:	return Orz;
	case 0xc9:	return Oret;
	case 0xca:	return Ojz;
	case 0xcc:	return Ocz;
	case 0xcd:	return Ocall;
	case 0xd0:	return Ornc;
	case 0xd2:	return Ojnc;
	case 0xd3:	return Oout;
	case 0xd4:	return Ocnc;
	case 0xd6:	return Osui;
	case 0xd8:	return Orc;
	case 0xda:	return Ojc;
	case 0xdb:	return Oin;
	case 0xde:	return Osbi;
	case 0xe0:	return Orpo;
	case 0xe2:	return Ojpo;
	case 0xe3:	return Oxthl;
	case 0xe6:	return Oani;
	case 0xe9:	return Opchl;
	case 0xea:	return Ojpe;
	case 0xeb:	return Oxchg;
	case 0xec:	return Ocpe;
	case 0xee:	return Oxri;
	case 0xf0:	return Orp;
	case 0xf2:	return Ojp;
	case 0xf3:	return Odi;
	case 0xf6:	return Oori;
	case 0xf8:	return Orm;
	case 0xfa:	return Ojm;
	case 0xfb:	return Oei;
	case 0xfc:	return Ocm;
	case 0xfe:	return Ocpi;
	}
	if((b&0x80) != 0){
		switch(b&0xf8){
		case 0x80:	return Oadd;
		case 0x88:	return Oadc;
		case 0x90:	return Osub;
		case 0x98:	return Osbb;
		case 0xa0:	return Oana;
		case 0xa8:	return Oxra;
		case 0xb0:	return Oora;
		case 0xb8:	return Ocmp;
		}
		switch(b&0x7){
		case 0x1:	return Opop;
		case 0x5:
			switch(b&0xf){
			case 0x5:	return Opush;
			}
			break;
		case 0x7:	return Orst;
		}
	}else{
		if((b&0xc0) == 0x40)
			return Omov;
		switch(b&0x0f){
		case 0x00:
			switch(b){
			case 0x00:	return Onop;
			case 0x10:	return Onop;
			case 0x20:	return Orim;
			case 0x30:	return Osim;
			}
			break;
		case 0x01:	return Olxi;
		case 0x02:
			switch((b>>5)&1){
			case 0:	return Ostax;
			case 1:
				switch(b){
				case 0x22:	return Oshld;
				case 0x32:	return Osta;
				}
				break;
			}
			break;
		case 0x03:	return Oinx;
		case 0x07:	return Orlc;
		case 0x08:	return Onop;
		case 0x09:	return Odad;
		case 0x0a:
			switch((b>>5)&1){
			case 0:	return Oldax;
			case 1:	return Olda;
			}
			break;
		case 0x0b:	return Odcx;
		}
		switch(b&0x07){
		case 0x04:	return Oinr;
		case 0x05:	return Odcr;
		case 0x06:	return Omvi;
		}
	}
	return -1;
}

int
decodeinsn(Insn *insn, uchar *mem, long len)
{
	if(len < 1)
		return 0;
	if(insn->op == -1)
		return -1;
	return insntypes[isa[insn->op].type].dec(insn, mem, len);
}

static int
decode(Insn *insn, uchar *mem, long len)
{
	if(len < 1)
		return 0;
	insn->op = decodeop(mem[0]);
	return decodeinsn(insn, mem, len);
}

int
das1(uchar *s, long len)
{
	int n;
	Insn insn;

	if((n = decode(&insn, s, len)) < 0)
		return -1;
	Bprint(stdout, "%#.4ullx:\t%I\n", s - mem, &insn);
	//Bflush(stdout);
	return n;
}

int
das(uchar *s, uchar *e, int max)
{
	int n, isz;

	n = 0;
	for(;;){
		if(s >= e)
			break;
		if(max >= 0 && n >= max)
			break;
		if((isz = das1(s, e-s)) < 0){
			n = -1;
			break;
		}
		if(isz == 0)
			break;
		n++;
		s += isz;
	}
	Bflush(stdout);
	return n;
}

int
dasfile(char *file)
{
	Biobuf *r;
	Insn insn;
	int buflen, n;
	u16int addr;
	uchar buf[1024];
	uchar *bp;

	r = Bopen(file, OREAD);
	if(r == nil)
		return -1;
	addr = 0;
	buflen = 0;
	bp = buf;
	for(;;){
		memmove(buf, bp, buflen);
		bp = buf;
		n = Bread(r, buf + buflen, sizeof(buf) - buflen);
		if(n < 0){
			Bterm(r);
			return -1;
		}
		if(n == 0){
			Bterm(r);
			break;
		}
		buflen += n;
		while(buflen > 0){
			n = decode(&insn, bp, buflen);
			if(n == 0)
				break;
			if(n < 0){
				print("%#.4ux\t???\t%#.2uhhx\n", addr, bp[0]);
				addr++;
				bp++;
				buflen--;
			}else{
				print("%#.4ux\t%I\n", addr, &insn);
				addr += n;
				bp += n;
				buflen -= n;
			}
		}
	}
	while(buflen > 0){
		Bprint(stdout, "%#.4x\t???\t%#.2x\n", addr, buf[0]);
		addr++;
		bp++;
		buflen--;
	}
	Bflush(stdout);
	return 0;
}

void
cpuexec(CPU *cpu, Insn *insn)
{
	if(isa[insn->op].exec == nil){
		Bprint(stderr, "%s (%#.2uhhx) not implemented!\n", opstr(insn->op), insn->op);
		Bflush(stderr);
		trap();
	}
	itrace(opstr(insn->op));
	isa[insn->op].exec(cpu, insn);
}

static u16int
rpair(CPU *cpu, u8int rp)
{
	switch(rp){
	case RBC: return cpu->r[RB]<<8 | cpu->r[RC];
	case RDE: return cpu->r[RD]<<8 | cpu->r[RE];
	case RHL: return cpu->r[RH]<<8 | cpu->r[RL];
	}
	fatal("unknown register pair %d", rp);
	return 0;
}

static void
wpair(CPU *cpu, u8int rp, u16int x)
{
	cpu->r[(rp<<1)] = x>>8;
	cpu->r[(rp<<1)+1] = x;
}

static void
Xnop(CPU*, Insn*)
{
}

static void
Xjp(CPU *cpu, Insn *insn)
{
	if((cpu->flg & Fsign) == 0)
		cpu->PC = insn->addr;
}

static void
Xjmp(CPU *cpu, Insn *insn)
{
	cpu->PC = insn->addr;
}

static void
Xlxi(CPU *cpu, Insn *insn)
{
	if(insn->rp == 3)
		cpu->SP = insn->imm1<<8|insn->imm;
	else
		wpair(cpu, insn->rp, insn->imm1<<8|insn->imm);
}

static void
Xmvi(CPU *cpu, Insn *insn)
{
	u16int addr;

	if(insn->r1 == RM){
		addr = rpair(cpu, RHL);
		memwrite(addr, insn->imm);
	}else{
		cpu->r[insn->r1] = insn->imm;
	}
}

static void
Xcall(CPU *cpu, Insn *insn)
{
	push16(cpu, cpu->PC);
	cpu->PC = insn->addr;
}

static void
Xldax(CPU *cpu, Insn *insn)
{
	cpu->r[RA] = memread(rpair(cpu, insn->rp));
}

static void
Xmov(CPU *cpu, Insn *insn)
{
	if(insn->r2 == RM && insn->r1 == RM)
		return;
	if(insn->r2 == RM)
		cpu->r[insn->r1] = memread(rpair(cpu, RHL));
	else if(insn->r1 == RM)
		memwrite(rpair(cpu, RHL), cpu->r[insn->r2]);
	else
		cpu->r[insn->r1] = cpu->r[insn->r2];
}

static void
Xinx(CPU *cpu, Insn *insn)
{
	wpair(cpu, insn->rp, rpair(cpu, insn->rp) + 1);
}

static void
Xdcr(CPU *cpu, Insn *insn)
{
	u16int a, x;

	if(insn->r1 == RM){
		a = rpair(cpu, RHL);
		x = memread(a);
	}else{
		a = 0;
		x = cpu->r[insn->r1];
	}

	if(--x == 0)
		cpu->flg |= Fzero;
	else
		cpu->flg &= ~Fzero;

	if((x&0x80) != 0)
		cpu->flg |= Fsign;
	else
		cpu->flg &= ~Fsign;

	if(insn->r1 == RM)
		memwrite(a, x);
	else
		cpu->r[insn->r1] = x;
}

static void
Xret(CPU *cpu, Insn*)
{
	cpu->PC = pop16(cpu);
}

static void
Xdad(CPU *cpu, Insn *insn)
{
	u32int x;

	if(insn->rp == 3){
		x = cpu->SP;
	}else{
		x = rpair(cpu, insn->rp);
	}
	x += rpair(cpu, RHL);
	if(x>>16 > 0)
		cpu->flg |= Fcarry;
	else
		cpu->flg &= ~Fcarry;
	wpair(cpu, RHL, x);
}

static void
Xsta(CPU *cpu, Insn *insn)
{
	memwrite(insn->addr, cpu->r[RA]);
}

static void
Xxra(CPU *cpu, Insn *insn)
{
	u8int x;

	if(insn->r1 == RM)
		x = memread(rpair(cpu, RHL));
	else
		x = cpu->r[insn->r1];

	cpu->r[RA] ^= x;

	if(cpu->r[RA] == 0)
		cpu->flg |= Fzero;
	else
		cpu->flg &= ~Fzero;

	if((cpu->r[RA] & 0x80) != 0)
		cpu->flg |= Fsign;
	else
		cpu->flg &= ~Fsign;

	cpu->flg &= ~(Fcarry|Fhcarry);
}

static void
Xout(CPU *cpu, Insn *insn)
{
	iow(insn->imm<<8|insn->imm, cpu->r[RA]);
}

static void
Xora(CPU *cpu, Insn *insn)
{
	u8int x;

	if(insn->r1 == RM)
		x = memread(rpair(cpu, RHL));
	else
		x = cpu->r[insn->r1];

	cpu->r[RA] |= x;

	if(cpu->r[RA] == 0)
		cpu->flg |= Fzero;
	else
		cpu->flg &= ~Fzero;

	if((cpu->r[RA] & 0x80) != 0)
		cpu->flg |= Fsign;
	else
		cpu->flg &= ~Fsign;

	cpu->flg &= ~(Fcarry|Fhcarry);
}

static void
Xlda(CPU *cpu, Insn *insn)
{
	cpu->r[RA] = memread(insn->addr);
}

static void
Xana(CPU *cpu, Insn *insn)
{
	u8int x;

	if(insn->r1 == RM)
		x = memread(rpair(cpu, RHL));
	else
		x = cpu->r[insn->r1];

	cpu->r[RA] &= x;

	if(cpu->r[RA] == 0)
		cpu->flg |= Fzero;
	else
		cpu->flg &= ~Fzero;

	if((cpu->r[RA] & 0x80) != 0)
		cpu->flg |= Fsign;
	else
		cpu->flg &= ~Fsign;

	cpu->flg &= ~(Fcarry|Fhcarry);
}

static void
Xpush(CPU *cpu, Insn *insn)
{
	if(insn->rp == RMM){
		push8(cpu, cpu->r[RA]);
		push8(cpu, cpu->flg);
	}else{
		push16(cpu, rpair(cpu, insn->rp));
	}
}

static void
Xpop(CPU *cpu, Insn *insn)
{
	if(insn->rp == RMM){
		cpu->flg = pop8(cpu);
		cpu->r[RA] = pop8(cpu);
	}else{
		wpair(cpu, insn->rp, pop16(cpu));
	}
}

static void
Xxchg(CPU *cpu, Insn*)
{
	u16int x;

	x = rpair(cpu, RHL);
	wpair(cpu, RHL, rpair(cpu, RDE));
	wpair(cpu, RDE, x);
}

static void
Xinr(CPU *cpu, Insn *insn)
{
	u8int x;
	u16int a;

	if(insn->r1 == RM){
		a = memread(rpair(cpu, RHL));
		x = memread(a) + 1;
		memwrite(a, x);
	}else{
		x = cpu->r[insn->r1] + 1;
		cpu->r[insn->r1] = x;
	}

	if(x == 0)
		cpu->flg |= Fzero;
	else
		cpu->flg &= ~Fzero;

	if((x & 0x80) != 0)
		cpu->flg |= Fsign;
	else
		cpu->flg &= ~Fsign;

	cpu->flg &= ~Fhcarry;
}

static int
evenpar(u8int x)
{
	u8int r;

	r = 1;
	while(x != 0){
		r ^= x&1;
		x >>= 1;
	}
	return r;
}

static void
Xani(CPU *cpu, Insn *insn)
{
	cpu->r[RA] &= insn->imm;

	if(cpu->r[RA] == 0)
		cpu->flg |= Fzero;
	else
		cpu->flg &= ~Fzero;

	if((cpu->r[RA] & 0x80) != 0)
		cpu->flg |= Fsign;
	else
		cpu->flg &= ~Fsign;

	if(evenpar(cpu->r[RA]))
		cpu->flg |= Fparity;
	else
		cpu->flg &= ~Fparity;

	cpu->flg &= ~(Fcarry|Fhcarry);
}

static void
Xrar(CPU *cpu, Insn*)
{
	u8int ocarry;

	ocarry = (cpu->flg&Fcarry) != 0;
	if((cpu->r[RA]&1) != 0)
		cpu->flg |= Fcarry;
	else
		cpu->flg &= ~Fcarry;
	cpu->r[RA] = ocarry<<7|((cpu->r[RA]>>1)&0x7f);
}

static void
Xori(CPU *cpu, Insn *insn)
{
	cpu->r[RA] |= insn->imm;

	if(cpu->r[RA] == 0)
		cpu->flg |= Fzero;
	else
		cpu->flg &= ~Fzero;

	if((cpu->r[RA] & 0x80) != 0)
		cpu->flg |= Fsign;
	else
		cpu->flg &= ~Fsign;

	cpu->flg &= ~(Fcarry|Fhcarry);
}

static void
Xcmp(CPU *cpu, Insn *insn)
{
	if(cpu->r[RA] == insn->r1)
		cpu->flg |= Fzero;
	else{
		cpu->flg &= ~Fzero;
		if(cpu->r[RA] < insn->r1)
			cpu->flg |= Fcarry;
		else
			cpu->flg &= ~Fcarry;
	}
}

static void
Xrlc(CPU *cpu, Insn*)
{
	u8int ncarry;

	ncarry = (cpu->r[RA]&0x80) != 0;
	if(ncarry != 0)
		cpu->flg |= Fcarry;
	else
		cpu->flg &= ~Fcarry;
	cpu->r[RA] = ((cpu->r[RA]<<1)&0xfe)|ncarry;
}

static void
Xrrc(CPU *cpu, Insn*)
{
	u8int ncarry;

	ncarry = cpu->r[RA]&1;
	if(ncarry != 0)
		cpu->flg |= Fcarry;
	else
		cpu->flg &= ~Fcarry;
	cpu->r[RA] = ncarry<<7|((cpu->r[RA]>>1)&0x7f);
}

static void
Xdcx(CPU *cpu, Insn *insn)
{
	if(insn->rp == 3)
		cpu->SP--;
	else
		wpair(cpu, insn->rp, rpair(cpu, insn->rp) - 1);
}

static void
Xstax(CPU *cpu, Insn *insn)
{
	memwrite(rpair(cpu, insn->rp), cpu->r[RA]);
}

static void
Xcpi(CPU *cpu, Insn *insn)
{
	if(cpu->r[RA] == insn->imm)
		cpu->flg |= Fzero;
	else{
		cpu->flg &= ~Fzero;
		if(cpu->r[RA] < insn->imm)
			cpu->flg |= Fcarry;
		else
			cpu->flg &= ~Fcarry;
	}
}

static void
psz(CPU *cpu, int f)
{
	u8int x, p;

	if(cpu->r[RA] == 0){
		if(f & Fzero) cpu->flg |= Fzero;
		if(f & Fsign) cpu->flg &= ~Fsign;
	}else{
		if(f & Fzero) cpu->flg &= ~Fzero;
		if((cpu->r[RA] & 0x80) != 0)
			if(f & Fsign) cpu->flg |= Fsign;
	}

	if(f & Fparity){
		x = cpu->r[RA];
		p = 0;
		p += (x&1); x >>= 1;
		p += (x&1); x >>= 1;
		p += (x&1); x >>= 1;
		p += (x&1); x >>= 1;
		p += (x&1); x >>= 1;
		p += (x&1); x >>= 1;
		p += (x&1); x >>= 1;
		p += (x&1);
		if((p & 1) != 0)
			cpu->flg |= Fparity;
		else
			cpu->flg &= ~Fparity;
	}
}

static void
Xadi(CPU *cpu, Insn *insn)
{
	u16int x;

	x = cpu->r[RA] + insn->imm;
	if((x>>8) > 0)
		cpu->flg |= Fcarry;
	else
		cpu->flg &= ~Fcarry;
	cpu->r[RA] = x;
	psz(cpu, Fparity|Fsign|Fzero);
}

static void
Xei(CPU *cpu, Insn*)
{
	cpu->intr |= Ienabled;
}

static void
Xdi(CPU *cpu, Insn*)
{
	cpu->intr &= ~Ienabled;
}

static void
Xin(CPU *cpu, Insn *insn)
{
	cpu->r[RA] = ior(insn->addr);
}

static void
Xjc(CPU *cpu, Insn *insn)
{
	if((cpu->flg&Fcarry) != 0)
		cpu->PC = insn->addr;
}

static void
Xjz(CPU *cpu, Insn *insn)
{
	if((cpu->flg&Fzero) != 0)
		cpu->PC = insn->addr;
}

static void
Xjnc(CPU *cpu, Insn *insn)
{
	if((cpu->flg&Fcarry) == 0)
		cpu->PC = insn->addr;
}

static void
Xjnz(CPU *cpu, Insn *insn)
{
	if((cpu->flg&Fzero) == 0)
		cpu->PC = insn->addr;
}

static void
Xrc(CPU *cpu, Insn*)
{
	if((cpu->flg&Fcarry) != 0)
		cpu->PC = pop16(cpu);
}

static void
Xrnc(CPU *cpu, Insn*)
{
	if((cpu->flg&Fcarry) == 0)
		cpu->PC = pop16(cpu);
}

static void
Xrz(CPU *cpu, Insn*)
{
	if((cpu->flg&Fzero) != 0)
		cpu->PC = pop16(cpu);
}

static void
Xrnz(CPU *cpu, Insn*)
{
	if((cpu->flg&Fzero) == 0)
		cpu->PC = pop16(cpu);
}

static void
Xsui(CPU *cpu, Insn *insn)
{
	u8int a, b;
	u16int x;

	a = cpu->r[RA];
	b = -insn->imm;
	if((((a&0xf)+(b&0xf))&0x10) != 0)
		cpu->flg |= Fhcarry;
	else
		cpu->flg &= Fhcarry;
	x = a + b;
	if((x&0x100) != 0)
		cpu->flg &= ~Fcarry;
	else
		cpu->flg |= Fcarry;
	cpu->r[RA] = x;
	psz(cpu, Fparity|Fsign|Fzero);
}

static void
Xxthl(CPU *cpu, Insn*)
{
	u8int x;

	x = memread(cpu->SP+0);
	memwrite(cpu->SP+0, cpu->r[RL]);
	cpu->r[RL] = x;
	x = memread(cpu->SP+1);
	memwrite(cpu->SP+1, cpu->r[RH]);
	cpu->r[RH] = x;
}

static void
Xpchl(CPU *cpu, Insn*)
{
	cpu->PC = rpair(cpu, RHL);
}

static void
Xcz(CPU *cpu, Insn *insn)
{
	if((cpu->flg&Fzero) != 0){
		push16(cpu, cpu->PC);
		cpu->PC = insn->addr;
	}
}

static void
Xdaa(CPU *cpu, Insn*)
{
	u16int x;

	if((cpu->flg&Fhcarry) != 0 || cpu->r[RA] > 9){
		x = (cpu->r[RA]&0xf)+6;
		if(x>>4 > 0)
			cpu->flg |= Fhcarry;
		else
			cpu->flg &= ~Fhcarry;
		cpu->r[RA] = (cpu->r[RA]&0xf0)|(x&0x0f);
	}

	if((cpu->flg&Fcarry) != 0 || (cpu->r[RA]>>4) > 9){
		x = (cpu->r[RA]>>4)+6;
		if(x>>4 > 0)
			cpu->flg |= Fcarry;
		else
			cpu->flg &= ~Fcarry;
		cpu->r[RA] = (x<<4)|(cpu->r[RA]&0xf);
	}
}

static void
Xjm(CPU *cpu, Insn *insn)
{
	if((cpu->flg&Fsign) != 0)
		cpu->PC = insn->addr;
}

static void
Xsbi(CPU *cpu, Insn *insn)
{
	insn->imm = -insn->imm;
	Xadi(cpu, insn);
}

static void
Xshld(CPU *cpu, Insn *insn)
{
	memwrite(insn->addr+0, cpu->r[RL]);
	memwrite(insn->addr+1, cpu->r[RH]);
}
