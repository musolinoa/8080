/* debug */
void	dumpregs(void);
void	dumpmem(u16int, u16int);
void	fatal(char *fmt, ...);
void	itrace0(char *fmt, ...);
char*	rnam(u8int);
char*	rpnam(u8int);

#define dprint(...) if(debug)Bprint(stderr, __VA_ARGS__)
#define itrace(...) if(tracing>0)itrace0(__VA_ARGS__)

/* disassembler */
#pragma	   varargck    type  "I"   Insn*
int	insnfmt(Fmt*);
int	das(uchar*, long);
int dasfile(char*);

/* isa */
int	decodeop(u8int);
int	decodeinsn(Insn*, uchar*, long);
int	insnlen(u8int);
void	cpuexec(CPU*, Insn*);

/* traps */
void	trapinit(void);
void	trap(void);
#define wastrap() setjmp(trapjmp)

/* memory */
u8int	memread(u16int);
void	memwrite(u16int, u8int);
u8int	ifetch(CPU*);
u8int	pop8(CPU*);
u16int	pop16(CPU*);
void	push8(CPU*, u8int);
void	push16(CPU*, u16int);
