/*!
    \file  GDTUtil.cpp
    \brief class functions for set up GDT

    Copyright (c) 2003 HigePon
    WITHOUT ANY WARRANTY

    \author  HigePon
    \version $Revision$
    \date   create:2003/06/06 update:$Date$
*/

#include<global.h>
#include<operator.h>
#include<kernel.h>
#include<GDTUtil.h>

/*!
    \brief set segment descriptor

    \param  desc  segment descriptor
    \param  base  base address 32bit
    \param  limit limit        24bit include G, D/B, 0, AVL
    \param  type  segment type 8bit  include P, DPL, S, TYPE
    \author HigePon
    \date   create:2003/06/06 update:
*/
void GDTUtil::setSegDesc(SegDesc* desc, dword base, dword limit, byte type) {

    desc->baseL  = (word)(base & 0xFFFF);
    desc->baseM  = (byte)((base >> 16) & 0xFF);
    desc->baseH  = (byte)((base >> 24) & 0xFF);
    desc->limitL = (word)(limit & 0xFFFF);
    desc->limitH = (byte) (((limit >> 16) & 0x0F) | 0xC0); /* 4KB unit & 32bit segment */
    desc->type   = type;
    return;
}

/*!
    \brief call lgdt

    \author HigePon
    \date   create:2003/06/07 update:
*/
void GDTUtil::lgdt() {

    GDTR gdtr;
    gdtr.base  = (dword)g_gdt;
    gdtr.limit = sizeof(SegDesc) * GDT_ENTRY_NUM - 1;
    asm volatile("lgdt %0\n" : /* no output */ : "m" (gdtr));
    return;
}

/*!
    \brief set up GDT

    \author HigePon
    \date   create:2003/06/07 update:
*/
void GDTUtil::setUpGDT() {

    g_gdt = (SegDesc*)malloc(sizeof(SegDesc) * GDT_ENTRY_NUM);
    if (g_gdt == NULL) panic("GDT memory allcate error");

    /* NULL */
    setSegDesc(&g_gdt[0], 0, 0, 0);
    g_gdt[0].limitH = 0;

    /* SYS CS 0-4GB */
    setSegDesc(&g_gdt[1], 0, 0xFFFFF, SEGMENT_PRESENT | SEGMENT_DPL0 | 0x10 | 0x0A);

    /* SYS DS 0-4GB */
    setSegDesc(&g_gdt[2], 0, 0xFFFFF, SEGMENT_PRESENT | SEGMENT_DPL0 | 0x10 | 0x02);

    /* SYS SS 0-4GB */
    setSegDesc(&g_gdt[3], 0, 0xFFFFF, SEGMENT_PRESENT | SEGMENT_DPL0 | 0x10 | 0x02);

    lgdt();
    return;
}

