#include <u.h>
#include "dat.h"
#include "fns.h"

typedef struct Shifter Shifter;
struct Shifter {
	u16int reg;
	u8int off;
};

static Shifter shifter;

u8int
ior(u16int a)
{
	switch(a){
	case 0x3:
		return shifter.reg>>shifter.off;
	}
	return 0;
}

void
iow(u16int a, u8int v)
{
	switch(a){
	case 0x2:
		shifter.off = v&0x7;
		break;
	case 0x4:
		shifter.reg >>= 8;
		shifter.reg |= ((u16int)v)<<8;
		break;
	}
}
