typedef struct CPU CPU;
typedef struct Insn Insn;
typedef struct ISA ISA;
typedef struct InsnType InsnType;

struct CPU
{
	u8int flg;
	u8int r[8];
	u16int SP;
	u16int PC;
	u8int intr;
};

enum{
	B,
	C,
	D,
	E,
	H,
	L,
	M, /* dummy */
	A,
};

enum{
	BC,
	DE,
	HL,
};

enum{
	Fcarry = 1<<0,
	Fparity = 1<<2,
	Fhcarry = 1<<4,
	Fzero = 1<<6,
	Fsign = 1<<7,
};

enum{
	Imask = 7<<0,
	Ienabled = 1<<3,
	Ipending = 7<<4,
};

struct Insn
{
	u16int pc;
	s8int op;
	u8int r1;
	u8int r2;
	u8int rp;
	u16int addr;
	u8int imm;
	u8int imm1;
};

enum{
	Oadc,
	Oadd,
	Oadi,
	Oana,
	Oani,
	Ocall,
	Ocm,
	Ocma,
	Ocmc,
	Ocmp,
	Ocnc,
	Ocpe,
	Ocpi,
	Ocz,
	Odaa,
	Odad,
	Odcr,
	Odcx,
	Odi,
	Oei,
	Oin,
	Oinr,
	Oinx,
	Ojc,
	Ojm,
	Ojmp,
	Ojnc,
	Ojnz,
	Ojp,
	Ojpe,
	Ojpo,
	Ojz,
	Olda,
	Oldax,
	Olxi,
	Omov,
	Omvi,
	Onop,
	Oora,
	Oori,
	Oout,
	Opchl,
	Opop,
	Opush,
	Orar,
	Orc,
	Oret,
	Orim,
	Orlc,
	Orm,
	Ornc,
	Ornz,
	Orp,
	Orpo,
	Orrc,
	Orst,
	Orz,
	Osbb,
	Osbi,
	Oshld,
	Osim,
	Osta,
	Ostax,
	Osub,
	Osui,
	Oxchg,
	Oxra,
	Oxri,
	Oxthl,
};

enum
{
	T0,
	Taddr,
	Timm,
	Tr012,
	Tr345,
	Trimm,
	Trp,
	Trpimm,
	Trr,
};

struct ISA
{
	char *opstr;
	int type;
	void (*exec)(CPU*, Insn*);
};

struct InsnType
{
	int len;
	int (*das)(Fmt*, Insn*);
	int (*dec)(Insn*, uchar*, long);
};

enum{
	ROMSZ = 8*1024,
	RAMSZ = 1*1024,
	VIDSZ = 7*1024,
	MEMSZ = ROMSZ+RAMSZ+VIDSZ,
};

extern u8int mem[MEMSZ];
extern u8int *rom;
extern u8int *ram;
extern u8int *vid;

enum
{
	Tpush,
	Tpop,
};

typedef struct TraceOp TraceOp;
struct TraceOp
{
	u16int addr;
	uchar op;
};

typedef struct BrkPt BrkPt;
struct BrkPt
{
	u16int addr;
	uchar enabled;
};

enum{
	MAXBRKPTS = 10,
	MAXTRACEOPS = 10,
};

extern Biobuf *stdin;
extern Biobuf *stdout;
extern Biobuf *stderr;

extern CPU ocpu, cpu;
extern Insn insn;
extern int debug;
extern int tracing;
extern int ntraceops;
extern TraceOp traceops[MAXTRACEOPS];
extern int nbrkpts;
extern BrkPt brkpts[MAXBRKPTS];
extern jmp_buf trapjmp;
