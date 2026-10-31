/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PUCCI_MILESTONE_H_
#define _PUCCI_MILESTONE_H_
void pucci_milestone(int band, unsigned int color);
void pucci_initcall_mark(int idx, int done);
void pucci_submark(int pos, unsigned int color);
void pucci_smark2(int pos, unsigned int color);
void pucci_dump32(int y, unsigned int val);
void pucci_dumphex(int y, u64 v, int ndigits);
#endif
