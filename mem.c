#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

u8int
memread(u16int a)
{
	if(a >= MEMSZ){
		Bprint(stderr, "memread failed addr=%#.4x op=%#0.2x pc=%#.4x\n", a, insn.op, ocpu.PC);
		Bflush(stderr);
		trap();
	}
	return mem[a];
}

void
memwrite(u16int a, u8int x)
{
	if(a < ROMSZ || a >= MEMSZ){
		Bprint(stderr, "write failed addr=%#.4x op=%#0.2x pc=%#.4x\n", a, insn.op, ocpu.PC);
		Bflush(stderr);
		trap();
	}
	mem[a] = x;
}

u8int
ifetch(CPU *cpu)
{
	if(cpu->PC >= ROMSZ){
		Bprint(stderr, "ifetch failed pc=%#.4x\n", cpu->PC);
		Bflush(stderr);
		trap();
	}
	return memread(cpu->PC++);
}

u8int
pop8(CPU *cpu)
{
	return memread(cpu->SP++);
}

u16int
pop16(CPU *cpu)
{
	u16int x;

	x = memread(cpu->SP++);
	return x | memread(cpu->SP++)<<8;
}

void
push8(CPU *cpu, u8int x)
{
	memwrite(--cpu->SP, x);
}

void
push16(CPU *cpu, u16int x)
{
	memwrite(--cpu->SP, x>>8);
	memwrite(--cpu->SP, x&0xff);
}
