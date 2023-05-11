#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>

enum{
	Width = 224,
	Height = 256,
};

uchar videomem[Width*Height/8];

static void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	threadexitsall("usage");
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
scanout(Channel *c, uchar *vmem)
{
	int i;
	uchar *p;
	Image *line;
	Rectangle r;

	line = allocimage(display, Rect(0,0,Width,1), GREY1, 0, DNofill);

	for(;;){
		p = vmem;
		r.min = screen->r.min;
		r.max = Pt(screen->r.max.x, screen->r.min.y+1);
		for(i = 0; i < Height/2; i++){
			loadimage(line, Rect(0,0,Width,1), p, Width/8);
			draw(screen, r, line, nil, ZP);
			r = rectaddpt(r, Pt(0, 1));
			p += Width/8;
		}
		flushimage(display, 1);
		if(c != nil)
			nbsendul(c, 0);
		for(; i < Height; i++){
			loadimage(line, Rect(0,0,Width,1), p, Width/8);
			draw(screen, r, line, nil, ZP);
			r = rectaddpt(r, Pt(0, 1));
			p += Width/8;
		}
		flushimage(display, 1);
		if(c != nil)
			nbsendul(c, 1);
	}
}

static void
initvmem(void)
{
	uchar *p;
	int i;

	p = videomem;
	srand(time(nil));
	for(i = 0; i < nelem(videomem); i++)
		*p++ = rand();
}

void
threadmain(int argc, char **argv)
{
	ARGBEGIN{
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
	draw(screen, screen->r, display->black, nil, ZP);
	flushimage(display, 1);
	initvmem();
	scanout(nil, videomem);
	for(;;)
		sleep(1000);
}
