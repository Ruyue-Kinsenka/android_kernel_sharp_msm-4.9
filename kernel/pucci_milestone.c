// SPDX-License-Identifier: GPL-2.0
/*
 * pucci bring-up milestone marker.
 *
 * The UEFI splash framebuffer lives in cont_splash_region@0x9d400000 and
 * the DSI continuous-splash keeps scanning it while the kernel boots.
 * Drawing a 40px color band per boot stage lets us see - without a serial
 * console - how far the kernel got before a hang.
 */
#define pr_fmt(fmt) "pucci_ms: " fmt
#include <linux/types.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/memory.h>
#include <asm/cacheflush.h>

#define FB_PHYS		0x9d400000ULL
#define FB_SIZE		0x2400000ULL
#define FB_WIDTH	1440
#define BAND_H		40

static void __iomem *ms_fb;

static void ms_setup(void)
{
	if (!ms_fb)
		ms_fb = (void __iomem *)phys_to_virt(FB_PHYS);
}

/* push CPU cache lines out to DRAM so the DSI scanout DMA sees them */
static void ms_flush(void *addr, size_t len)
{
	__flush_dcache_area(addr, len);
}

void pucci_milestone(int band, unsigned int color)
{
	int i;
	u32 *p;

	ms_setup();
	if (!ms_fb)
		return;
	if (band == 0) {
		/*
		 * DRAM survives warm resets - stale marks from a previous
		 * boot would confuse readings.  Wipe the marker zone first.
		 */
		p = (u32 __force *)ms_fb;
		for (i = 0; i < 1200 * FB_WIDTH; i++)
			p[i] = 0xFF000000;
		ms_flush(p, 1200 * FB_WIDTH * 4);
	}
	p = (u32 __force *)ms_fb + band * BAND_H * FB_WIDTH;
	for (i = 0; i < BAND_H * FB_WIDTH; i++)
		p[i] = color;
	ms_flush(p, BAND_H * FB_WIDTH * 4);
}

/*
 * Per-initcall trace: a small square per initcall, drawn red before the
 * call runs and flipped green when it returns.  A lone red square on a
 * hung screen identifies the exact initcall that deadlocked; the index
 * maps back to a function via `nm vmlinux | grep __initcall` ordering.
 *
 * Grid: 160 columns of 9px cells, 12px rows, starting at y=340.
 */
#define IC_COLS		160
#define IC_CELL_W	9
#define IC_CELL_H	12
#define IC_Y0		340

#define ST_Y		600	/* fat status band y */
#define ST_H		40
#define DIG_Y		(ST_Y + ST_H + 20)
#define DIG_SCALE	10	/* each font pixel = 10x10 screen pixels */
#define DIG_W		(3 * DIG_SCALE)
#define DIG_H		(5 * DIG_SCALE)
#define DIG_PITCH	(DIG_W + DIG_SCALE)

static void ms_fill(u32 *fb, int x, int y, int w, int h, u32 color)
{
	u32 *p = fb + y * FB_WIDTH + x;
	int i, j;

	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
			p[j * FB_WIDTH + i] = color;
	ms_flush(p, h * FB_WIDTH * 4);
}

/* 5x7 font scaled by BIG (each font cell -> BIG x BIG block) */
#define BIG	40

static void ms_big_digit(u32 *fb, int x, int y, int d)
{
	static const u8 font[10][7] = {
		{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},	/* 0 */
		{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},	/* 1 */
		{0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},	/* 2 */
		{0x1F,0x01,0x02,0x06,0x01,0x11,0x0E},	/* 3 */
		{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},	/* 4 */
		{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},	/* 5 */
		{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},	/* 6 */
		{0x1F,0x01,0x02,0x04,0x08,0x08,0x08},	/* 7 */
		{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},	/* 8 */
		{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},	/* 9 */
	};
	int r, c;

	/* clear the glyph cell first - redraws must not stack old digits */
	ms_fill(fb, x, y, 5 * BIG, 7 * BIG, 0xFF000000);
	for (r = 0; r < 7; r++)
		for (c = 0; c < 5; c++)
			if (font[d][r] & (1 << (4 - c)))
				ms_fill(fb, x + c * BIG, y + r * BIG,
					BIG, BIG, 0xFFFFFFFF);
}

void pucci_initcall_mark(int idx, int done)
{
	u32 *fb;
	int x, y;
	u32 color = done ? 0xFF00FF00 : 0xFFFF0000;

	if (idx < 0 || idx >= 1200)
		return;
	ms_setup();
	if (!ms_fb)
		return;
	fb = (u32 __force *)ms_fb;

	x = (idx % IC_COLS) * IC_CELL_W;
	y = IC_Y0 + (idx / IC_COLS) * IC_CELL_H;
	ms_fill(fb, x, y, IC_CELL_W - 1, IC_CELL_H - 2, color);

	/* fat status band: red = an initcall is currently running */
	ms_fill(fb, 0, ST_Y, FB_WIDTH, ST_H, color);

	/* progress bar: white, width in px == idx */
	ms_fill(fb, 0, DIG_Y, FB_WIDTH, 80, 0xFF000000);
	ms_fill(fb, 0, DIG_Y, idx, 80, 0xFFFFFFFF);

	/* yellow tick exactly at x == idx for edge visibility */
	ms_fill(fb, idx, DIG_Y - 20, 20, 120, 0xFFFFFF00);

	/* giant digits: 4 digits, each ~150px wide x 350px tall */
	{
		int d0 = (idx / 1000) % 10, d1 = (idx / 100) % 10;
		int d2 = (idx / 10) % 10, d3 = idx % 10;

		ms_big_digit(fb, 90,   900, d0);
		ms_big_digit(fb, 420,  900, d1);
		ms_big_digit(fb, 750,  900, d2);
		ms_big_digit(fb, 1080, 900, d3);
	}
}
/*
 * Sub-step trace inside a single function: 50px squares in the free zone
 * at y=800.  Position encodes the step number.
 */
void pucci_submark(int pos, unsigned int color)
{
	u32 *fb;

	ms_setup();
	if (!ms_fb)
		return;
	if (pos < 0 || pos > 22)
		return;
	fb = (u32 __force *)ms_fb;
	/* bar width encodes the step: (pos+1)*130 px at y=800, hard to miss */
	ms_fill(fb, 0, 800, min((pos + 1) * 130, FB_WIDTH), 70, color);
}

/* second strip below the submark bar: y=880, (pos+1)*60 px wide */
void pucci_smark2(int pos, unsigned int color)
{
	u32 *fb;

	ms_setup();
	if (!ms_fb)
		return;
	if (pos < 0 || pos > 23)
		return;
	fb = (u32 __force *)ms_fb;
	ms_fill(fb, 0, 880, (pos + 1) * 60, 15, color);
}

/*
 * Bit-dump for camera readout: 32 cells, white=1 dark=0.
 * MSB at left.  Each cell 40px pitch / 36px wide / 55px tall.
 */
void pucci_dump32(int y, unsigned int val)
{
	u32 *fb;
	int i;

	ms_setup();
	if (!ms_fb)
		return;
	fb = (u32 __force *)ms_fb;
	for (i = 0; i < 32; i++)
		ms_fill(fb, i * 40, y, 36, 55,
			(val & (1U << (31 - i))) ? 0xFFFFFFFF : 0xFF202020);
}

/*
 * Hex-dump for camera readout: renders ndigits nibbles of v as real
 * 5x7 hex glyphs (0-9, A-F), centered horizontally so the rounded
 * screen corners don't clip the outer digits.  Scale 10.
 */
#define HX_S	10
#define HX_GW	(5 * HX_S)
#define HX_P	(HX_GW + HX_S)

static const u8 hxfont[16][7] = {
	{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},	/* 0 */
	{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},	/* 1 */
	{0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},	/* 2 */
	{0x1F,0x01,0x02,0x06,0x01,0x11,0x0E},	/* 3 */
	{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},	/* 4 */
	{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},	/* 5 */
	{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},	/* 6 */
	{0x1F,0x01,0x02,0x04,0x08,0x08,0x08},	/* 7 */
	{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},	/* 8 */
	{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},	/* 9 */
	{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},	/* A */
	{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},	/* B */
	{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},	/* C */
	{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},	/* D */
	{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},	/* E */
	{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},	/* F */
};

void pucci_dumphex(int y, u64 v, int ndigits)
{
	u32 *fb;
	int i, x, r, c, nib;
	int total = ndigits * HX_P + (ndigits > 4 ? HX_S * 2 : 0) +
		    (ndigits > 8 ? HX_S * 2 : 0) + (ndigits > 12 ? HX_S * 2 : 0);

	ms_setup();
	if (!ms_fb)
		return;
	fb = (u32 __force *)ms_fb;
	ms_fill(fb, 0, y - 4, FB_WIDTH, 7 * HX_S + 8, 0xFF000000);
	x = (FB_WIDTH - total) / 2;
	for (i = ndigits - 1; i >= 0; i--) {
		nib = (v >> (i * 4)) & 0xF;
		for (r = 0; r < 7; r++)
			for (c = 0; c < 5; c++)
				if (hxfont[nib][r] & (1 << (4 - c)))
					ms_fill(fb, x + c * HX_S, y + r * HX_S,
						HX_S, HX_S, 0xFFFFFFFF);
		x += HX_P;
		if ((i % 4) == 0 && i)
			x += HX_S * 2;
	}
}
EXPORT_SYMBOL_GPL(pucci_dumphex);
EXPORT_SYMBOL_GPL(pucci_dump32);
EXPORT_SYMBOL_GPL(pucci_smark2);
EXPORT_SYMBOL_GPL(pucci_submark);
EXPORT_SYMBOL_GPL(pucci_initcall_mark);
