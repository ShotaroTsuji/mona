/*!
  \file   ihandlers.cpp
  \brief  interrupt handlers

  interrupt handlers

  Copyright (c) 2002, 2003 and 2004 Higepon
  All rights reserved.
  License=MIT/X License

  \author  Higepon
  \version $Revision$
  \date   create:2002/07/25 update:$Date$
*/

#include "global.h"
#include "io.h"
#include "ihandlers.h"
#include "Process.h"
#include "Scheduler.h"
#include "Uart.h"
#include "RTC.h"

#define IRQHANDLERMaster(x) void irqHandler_##x()                             \
{                                                                             \
    outp8(0x20, 0x20);                                                        \
    if (g_irqInfo[x].maskInterrupt) outp8(0x21, (inp8(0x21) | (1 << x)));     \
    SendInterrupt(x);                                                         \
}

#define IRQHANDLERSlave(x) void irqHandler_##x()                              \
{                                                                             \
    outp8(0xA0, 0x20);                                                        \
    outp8(0x20, 0x20);                                                        \
    if (g_irqInfo[x].maskInterrupt) outp8(0xa1, inp8(0xa1) | (1 << (x - 8))); \
    SendInterrupt(x);                                                         \
}

void irqHandler_9()
{
    outp8(0xA0, 0x20);
    outp8(0x20, 0x20);
    if (g_irqInfo[9].maskInterrupt) outp8(0xa1, inp8(0xa1) | (1 << (9 - 8)));
    SendInterrupt(9);
}

void irqHandler_11()
{
    outp8(0xA0, 0x20);
    outp8(0x20, 0x20);

    if (g_irqInfo[11].maskInterrupt) outp8(0xa1, inp8(0xa1) | (1 << (11 - 8)));
    SendInterrupt(11);
}


// IRQHANDLERMaster(0)
IRQHANDLERMaster(1)
IRQHANDLERMaster(2)
IRQHANDLERMaster(3)
IRQHANDLERMaster(4)
IRQHANDLERMaster(5)
// IRQHANDLERMaster(6)
IRQHANDLERMaster(7)
IRQHANDLERSlave(8)
//IRQHANDLERSlave(9)
IRQHANDLERSlave(10)
//IRQHANDLERSlave(11)
IRQHANDLERSlave(12)
IRQHANDLERSlave(13)
IRQHANDLERSlave(14)
IRQHANDLERSlave(15)

/*!
  \brief send irq event

  \param irq irq number

  \author Higepon
  \date   create:2004/08/26 update:
*/
void SendInterrupt(int irq)
{
    MessageInfo msg;

    if (!g_irqInfo[irq].hasReceiver) return;

    /* set message */
    memset(&msg, 0, sizeof(MessageInfo));
    msg.header = MSG_INTERRUPTED;
    msg.arg1   = irq;

    intptr_t ret = g_messenger->send(g_irqInfo[irq].thread->thread, &msg);
    if (ret != M_OK) {
        g_console->printf("Send failed reason %d %s:%d\n", ret, __FILE__, __LINE__);
        g_irqInfo[irq].hasReceiver = false;
    }
    g_scheduler->SwitchToNext();
    /* not reached */
}

/*!
  \brief timer handler

  \author Higepon
  \date   create:2004/08/10 update:
*/
void irqHandler_0()
{

    static uint32_t i = 0;
    bool isProcessChange;

    RTC::increaseEpochNanoSec(KERNEL_TIMER_INTERVAL_MSEC * 1000 * 1000);

    /* EOI */
    outp8(0x20, 0x20);

    g_scheduler->Tick();
    g_currentThread->thread->Tick();

    if (i % 5)
    {
        isProcessChange = g_scheduler->Schedule1();
    }
    else
    {
        isProcessChange = g_scheduler->Schedule2();
    }

    i++;

    ThreadOperation::switchThread(isProcessChange, 1);

    /* not reached */
}

/*!
  \brief irq6 handler

  \author Higepon
  \date   create:2004/08/10 update:
*/
void irqHandler_6()
{
    /* thx! K-tan */
    outp8(0x20, 0x66);

    SendInterrupt(6);
    /* not reached */
}

static void terminateCurrentThread(const char* reason)
{
    Process* current = g_currentThread->process;

    g_console->printf("%s : Process killed %s thread-index=%d eip=%x\n", reason, current->getName(),
                      g_currentThread->process->getThreadIndex(g_currentThread->thread), g_currentThread->archinfo->eip);
    logprintf("%s: Process killed %s eip=%x\n", reason, current->getName(),  g_currentThread->archinfo->eip);
    ThreadOperation::kill();
}

/*----------------------------------------------------------------------
    Exception handlers
----------------------------------------------------------------------*/
extern "C" void divideErrorHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_DIVIDE_ERROR_EXCEPTION);
    }
    terminateCurrentThread("divide by zero");
}

extern "C" void debugHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_DEBUG_EXCEPTION);
    }
    terminateCurrentThread("debug exception");
}

extern "C" void nmiInterruptHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_NMI_INTTERUPT);
    }
    terminateCurrentThread(__func__);
}

extern "C" void set_debug_traps();
extern "C" void breakpointHandler()
{
    // Enable remote debug handlers, on first breakpoint exception.
    if (!g_isRemoteDebug) {
        g_console->printf("INFO: GDB remote debug is enabled\n");
        set_debug_traps();
        g_isRemoteDebug = true;
    }
    gdbCatchException(VECTOR_BREAKPOINT_EXCEPTION);
}

extern "C" void overflowHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_OVERFLOW_EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

extern "C" void boundRangeExceededHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_BOUND_RANGE_EXCEEDED_EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

extern "C" void invalidOpCodeHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_INVALID_OP_CODE_EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

extern "C" void deviceNotAvailableHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_DEVICE_NOT_AVAILABLE__EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

extern "C" void doubleFaultHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_DOUBLE_FAULT_EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

extern "C" void coprocessorSegmentOverrunHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_COPROCESSOR_SEGMENT_OVERRUN);
    }
    terminateCurrentThread(__func__);
}

extern "C" void invalidTssHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_INVALID_TSS_EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

extern "C" void segmentNotProcessHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_SEGMENT_NOT_PRESENT);
    }
    terminateCurrentThread(__func__);
}

extern "C" void stackFaultHandler(uintptr_t error)
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_STACK_FAULT_EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

extern "C" void generalProtectionHandler(uintptr_t error)
{
    const int IDT_ERROR = 2;
    g_console->printf(__func__);
    const char* processName = g_currentThread->process->getName();
    g_console->printf("\n%s thread-index=%d:", processName, g_currentThread->process->getThreadIndex(g_currentThread->thread));

    // See Intel's manual "Interruption and exception".
    if (error & IDT_ERROR) {
        // Indicates that the index portion of the error code refers to a gate descriptor in the IDT
        g_console->printf(" error on IDT. index %d of IDT table.\n", (error >> 3) & 0xff);
    } else {
        g_console->printf(" error code =%x\n", error);
    }

    uint32_t realcr3;
    ArchThreadInfo* i = g_currentThread->archinfo;
    g_console->printf("eax=%x ebx=%x ecx=%x edx=%x\n", i->eax, i->ebx, i->ecx, i->edx);
    g_console->printf("esp=%x ebp=%x esi=%x edi=%x\n", i->esp, i->ebp, i->esi, i->edi);
    g_console->printf("cs =%x ds =%x ss =%x cr3=%x, %x\n", i->cs , i->ds , i->ss , i->cr3, realcr3);
    g_console->printf("eflags=%x eip=%x\n", i->eflags, i->eip);
#if 1
    logprintf("name=%s\n", g_currentThread->process->getName());
    logprintf("eax=%x ebx=%x ecx=%x edx=%x\n", i->eax, i->ebx, i->ecx, i->edx);
    logprintf("esp=%x ebp=%x esi=%x edi=%x\n", i->esp, i->ebp, i->esi, i->edi);
    logprintf("cs =%x ds =%x ss =%x cr3=%x, %x\n", i->cs , i->ds , i->ss , i->cr3, realcr3);
    logprintf("eflags=%x eip=%x\n", i->eflags, i->eip);
#endif
    panic(__func__);
}

extern "C" void pageFaultHandler(uintptr_t address, uintptr_t error)
{
    if (!g_page_manager->pageFaultHandler(address, error, g_currentThread->archinfo->eip)) {
        bool isProcessChange = g_scheduler->Schedule2();
        ThreadOperation::switchThread(isProcessChange, 1);
    }
}

extern "C" void x87FloatingPointErrorHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_X87_FLOATING_POINT_ERROR);
    }
    terminateCurrentThread(__func__);
}

extern "C" void alignmentCheckHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_ALIGNMENT_CHECK_EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

extern "C" void machineCheckHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_MACHINE_CHECK_EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

extern "C" void simdFloatingPointHandler()
{
    if (g_isRemoteDebug) {
        gdbCatchException(VECTOR_SIMD_FLOATING_POINT_EXCEPTION);
    }
    terminateCurrentThread(__func__);
}

void dummyHandler()
{
    g_console->printf("dummy Handler\n");
    panic("dummy handler");
}

void dokodemoView() {
    g_console->printf("dokodemo");
    DokodemoView* i = &g_dokodemo_view;
    StackView*    j = &g_stack_view;

    g_console->printf("\n");
    g_console->printf("eax=%x ebx=%x ecx=%x edx=%x\n", i->eax, i->ebx, i->ecx, i->edx);
    g_console->printf("esp=%x ebp=%x esi=%x edi=%x\n", i->esp, i->ebp, i->esi, i->edi);
    g_console->printf("cs =%x ds =%x ss =%x cr3=%x\n", i->cs , i->ds , i->ss , i->cr3);
    g_console->printf("eflags=%x\n", i->eflags);
    g_console->printf("stack 0(%x) 1(%x) 2(%x) 3(%x)\n", j->stack0, j->stack1, j->stack2, j->stack3);
    g_console->printf("stack 4(%x) 6(%x) 5(%x) 7(%x)\n", j->stack4, j->stack5, j->stack6, j->stack7);

#if 1
    logprintf("\n");
    logprintf("eax=%x ebx=%x ecx=%x edx=%x\n", i->eax, i->ebx, i->ecx, i->edx);
    logprintf("esp=%x ebp=%x esi=%x edi=%x\n", i->esp, i->ebp, i->esi, i->edi);
    logprintf("cs =%x ds =%x ss =%x cr3=%x\n", i->cs , i->ds , i->ss , i->cr3);
    logprintf("eflags=%x\n", i->eflags);
    logprintf("stack 0(%x) 1(%x) 2(%x) 3(%x)\n", j->stack0, j->stack1, j->stack2, j->stack3);
    logprintf("stack 4(%x) 6(%x) 5(%x) 7(%x)\n", j->stack4, j->stack5, j->stack6, j->stack7);
#endif
}

extern "C" void arch_divideErrorHandler();
extern "C" void arch_debugHandler();
extern "C" void arch_nmiInterruptHandler();
extern "C" void arch_breakpointHandler();
extern "C" void arch_overflowHandler();
extern "C" void arch_boundRangeExceededHandler();
extern "C" void arch_invalidOpCodeHandler();
extern "C" void arch_deviceNotAvailableHandler();
extern "C" void arch_doubleFaultHandler();
extern "C" void arch_coprocessorSegmentOverrunHandler();
extern "C" void arch_invalidTssHandler();
extern "C" void arch_segmentNotProcessHandler();
extern "C" void arch_stackFaultHandler();
extern "C" void arch_generalProtectionHandler();
extern "C" void arch_pageFaultHandler();
extern "C" void arch_x87FloatingPointErrorHandler();
extern "C" void arch_alignmentCheckHandler();
extern "C" void arch_machineCheckHandler();
extern "C" void arch_simdFloatingPointHandler();

InterruptHandlers handlers[IHANDLER_NUM] = {
    {0x00, &arch_divideErrorHandler},
    {0x01, &arch_debugHandler},
    {0x02, &arch_nmiInterruptHandler},
    {0x03, &arch_breakpointHandler},
    {0x04, &arch_overflowHandler},
    {0x05, &arch_boundRangeExceededHandler},
    {0x06, &arch_invalidOpCodeHandler},
    {0x07, &arch_deviceNotAvailableHandler},
    {0x08, &arch_doubleFaultHandler},
    {0x09, &arch_coprocessorSegmentOverrunHandler},
    {0x0A, &arch_invalidTssHandler},
    {0x0B, &arch_segmentNotProcessHandler},
    {0x0C, &arch_stackFaultHandler},
    {0x0D, &arch_generalProtectionHandler},
    {0x0E, &arch_pageFaultHandler},
    {0x0F, &arch_dummyhandler},
    {0x10, &arch_x87FloatingPointErrorHandler},
    {0x11, &arch_alignmentCheckHandler},
    {0x12, &arch_machineCheckHandler},
    {0x13, &arch_simdFloatingPointHandler},
    {0x14, &arch_dummyhandler},
    {0x15, &arch_dummyhandler},
    {0x16, &arch_dummyhandler},
    {0x17, &arch_dummyhandler},
    {0x18, &arch_dummyhandler},
    {0x19, &arch_dummyhandler},
    {0x1A, &arch_dummyhandler},
    {0x1B, &arch_dummyhandler},
    {0x1C, &arch_dummyhandler},
    {0x1D, &arch_dummyhandler},
    {0x1E, &arch_dummyhandler},
    {0x1F, &arch_dummyhandler},
    {0x20, &arch_dummyhandler},
    {0x21, &arch_dummyhandler},
    {0x22, &arch_dummyhandler},
    {0x23, &arch_dummyhandler},
    {0x24, &arch_dummyhandler},
    {0x25, &arch_dummyhandler},
    {0x26, &arch_dummyhandler},
    {0x27, &arch_dummyhandler},
    {0x28, &arch_irqhandler_8},
    {0x29, &arch_irqhandler_9},
    {0x2A, &arch_irqhandler_10},
    {0x2B, &arch_irqhandler_11},
    {0x2C, &arch_irqhandler_12},
    {0x2D, &arch_irqhandler_13},
    {0x2E, &arch_irqhandler_14},
    {0x2F, &arch_irqhandler_15},
    {0x30, &arch_dummyhandler},
    {0x31, &arch_dummyhandler},
    {0x32, &arch_dummyhandler},
    {0x33, &arch_dummyhandler},
    {0x34, &arch_dummyhandler},
    {0x35, &arch_dummyhandler},
    {0x36, &arch_dummyhandler},
    {0x37, &arch_dummyhandler},
    {0x38, &arch_dummyhandler},
    {0x39, &arch_dummyhandler},
    {0x3A, &arch_dummyhandler},
    {0x3B, &arch_dummyhandler},
    {0x3C, &arch_dummyhandler},
    {0x3D, &arch_dummyhandler},
    {0x3E, &arch_dummyhandler},
    {0x3F, &arch_dummyhandler},
    {0x40, &arch_dummyhandler},
    {0x41, &arch_dummyhandler},
    {0x42, &arch_dummyhandler},
    {0x43, &arch_dummyhandler},
    {0x44, &arch_dummyhandler},
    {0x45, &arch_dummyhandler},
    {0x46, &arch_dummyhandler},
    {0x47, &arch_dummyhandler},
    {0x48, &arch_dummyhandler},
    {0x49, &arch_dummyhandler},
    {0x4A, &arch_dummyhandler},
    {0x4B, &arch_dummyhandler},
    {0x4C, &arch_dummyhandler},
    {0x4D, &arch_dummyhandler},
    {0x4E, &arch_dummyhandler},
    {0x4F, &arch_dummyhandler},
    {0x50, &arch_dummyhandler},
    {0x51, &arch_dummyhandler},
    {0x52, &arch_dummyhandler},
    {0x53, &arch_dummyhandler},
    {0x54, &arch_dummyhandler},
    {0x55, &arch_dummyhandler},
    {0x56, &arch_dummyhandler},
    {0x57, &arch_dummyhandler},
    {0x58, &arch_dummyhandler},
    {0x59, &arch_dummyhandler},
    {0x5A, &arch_dummyhandler},
    {0x5B, &arch_dummyhandler},
    {0x5C, &arch_dummyhandler},
    {0x5D, &arch_dummyhandler},
    {0x5E, &arch_dummyhandler},
    {0x5F, &arch_dummyhandler},
    {0x60, &arch_irqhandler_0} /* IRQ 0 */,
    {0x61, &arch_irqhandler_1},
    {0x62, &arch_irqhandler_2},
    {0x63, &arch_irqhandler_3},
    {0x64, &arch_irqhandler_4},
    {0x65, &arch_irqhandler_5},
    {0x66, &arch_irqhandler_6},
    {0x67, &arch_irqhandler_7},
    {0x68, &arch_dummyhandler},
    {0x69, &arch_dummyhandler},
    {0x6A, &arch_dummyhandler},
    {0x6B, &arch_dummyhandler},
    {0x6C, &arch_dummyhandler},
    {0x6D, &arch_dummyhandler},
    {0x6E, &arch_dummyhandler},
    {0x6F, &arch_dummyhandler},
    {0x70, &arch_dummyhandler},
    {0x71, &arch_dummyhandler},
    {0x72, &arch_dummyhandler},
    {0x73, &arch_dummyhandler},
    {0x74, &arch_dummyhandler},
    {0x75, &arch_dummyhandler},
    {0x76, &arch_dummyhandler},
    {0x77, &arch_dummyhandler},
    {0x78, &arch_dummyhandler},
    {0x79, &arch_dummyhandler},
    {0x7A, &arch_dummyhandler},
    {0x7B, &arch_dummyhandler},
    {0x7C, &arch_dummyhandler},
    {0x7D, &arch_dummyhandler},
    {0x7E, &arch_dummyhandler},
    {0x7F, &arch_dummyhandler},
    {0x80, &arch_syscall_handler},
    {0x81, &arch_dummyhandler},
    {0x82, &arch_dummyhandler},
    {0x83, &arch_dummyhandler},
    {0x84, &arch_dummyhandler},
    {0x85, &arch_dummyhandler},
    {0x86, &arch_dummyhandler},
    {0x87, &arch_dummyhandler},
    {0x88, &arch_dummyhandler},
    {0x89, &arch_dummyhandler},
    {0x8A, &arch_dummyhandler},
    {0x8B, &arch_dummyhandler},
    {0x8C, &arch_dummyhandler},
    {0x8D, &arch_dummyhandler},
    {0x8E, &arch_dummyhandler},
    {0x8F, &arch_dummyhandler},
    {0x90, &arch_dummyhandler},
    {0x91, &arch_dummyhandler},
    {0x92, &arch_dummyhandler},
    {0x93, &arch_dummyhandler},
    {0x94, &arch_dummyhandler},
    {0x95, &arch_dummyhandler},
    {0x96, &arch_dummyhandler},
    {0x97, &arch_dummyhandler},
    {0x98, &arch_dummyhandler},
    {0x99, &arch_dummyhandler},
    {0x9A, &arch_dummyhandler},
    {0x9B, &arch_dummyhandler},
    {0x9C, &arch_dummyhandler},
    {0x9D, &arch_dummyhandler},
    {0x9E, &arch_dummyhandler},
    {0x9F, &arch_dummyhandler},
    {0xA0, &arch_dummyhandler},
    {0xA1, &arch_dummyhandler},
    {0xA2, &arch_dummyhandler},
    {0xA3, &arch_dummyhandler},
    {0xA4, &arch_dummyhandler},
    {0xA5, &arch_dummyhandler},
    {0xA6, &arch_dummyhandler},
    {0xA7, &arch_dummyhandler},
    {0xA8, &arch_dummyhandler},
    {0xA9, &arch_dummyhandler},
    {0xAA, &arch_dummyhandler},
    {0xAB, &arch_dummyhandler},
    {0xAC, &arch_dummyhandler},
    {0xAD, &arch_dummyhandler},
    {0xAE, &arch_dummyhandler},
    {0xAF, &arch_dummyhandler},
    {0xB0, &arch_dummyhandler},
    {0xB1, &arch_dummyhandler},
    {0xB2, &arch_dummyhandler},
    {0xB3, &arch_dummyhandler},
    {0xB4, &arch_dummyhandler},
    {0xB5, &arch_dummyhandler},
    {0xB6, &arch_dummyhandler},
    {0xB7, &arch_dummyhandler},
    {0xB8, &arch_dummyhandler},
    {0xB9, &arch_dummyhandler},
    {0xBA, &arch_dummyhandler},
    {0xBB, &arch_dummyhandler},
    {0xBC, &arch_dummyhandler},
    {0xBD, &arch_dummyhandler},
    {0xBE, &arch_dummyhandler},
    {0xBF, &arch_dummyhandler},
    {0xC0, &arch_dummyhandler},
    {0xC1, &arch_dummyhandler},
    {0xC2, &arch_dummyhandler},
    {0xC3, &arch_dummyhandler},
    {0xC4, &arch_dummyhandler},
    {0xC5, &arch_dummyhandler},
    {0xC6, &arch_dummyhandler},
    {0xC7, &arch_dummyhandler},
    {0xC8, &arch_dummyhandler},
    {0xC9, &arch_dummyhandler},
    {0xCA, &arch_dummyhandler},
    {0xCB, &arch_dummyhandler},
    {0xCC, &arch_dummyhandler},
    {0xCD, &arch_dummyhandler},
    {0xCE, &arch_dummyhandler},
    {0xCF, &arch_dummyhandler},
    {0xD0, &arch_dummyhandler},
    {0xD1, &arch_dummyhandler},
    {0xD2, &arch_dummyhandler},
    {0xD3, &arch_dummyhandler},
    {0xD4, &arch_dummyhandler},
    {0xD5, &arch_dummyhandler},
    {0xD6, &arch_dummyhandler},
    {0xD7, &arch_dummyhandler},
    {0xD8, &arch_dummyhandler},
    {0xD9, &arch_dummyhandler},
    {0xDA, &arch_dummyhandler},
    {0xDB, &arch_dummyhandler},
    {0xDC, &arch_dummyhandler},
    {0xDD, &arch_dummyhandler},
    {0xDE, &arch_dummyhandler},
    {0xDF, &arch_dummyhandler},
    {0xE0, &arch_dummyhandler},
    {0xE1, &arch_dummyhandler},
    {0xE2, &arch_dummyhandler},
    {0xE3, &arch_dummyhandler},
    {0xE4, &arch_dummyhandler},
    {0xE5, &arch_dummyhandler},
    {0xE6, &arch_dummyhandler},
    {0xE7, &arch_dummyhandler},
    {0xE8, &arch_dummyhandler},
    {0xE9, &arch_dummyhandler},
    {0xEA, &arch_dummyhandler},
    {0xEB, &arch_dummyhandler},
    {0xEC, &arch_dummyhandler},
    {0xED, &arch_dummyhandler},
    {0xEE, &arch_dummyhandler},
    {0xEF, &arch_dummyhandler},
    {0xF0, &arch_dummyhandler},
    {0xF1, &arch_dummyhandler},
    {0xF2, &arch_dummyhandler},
    {0xF3, &arch_dummyhandler},
    {0xF4, &arch_dummyhandler},
    {0xF5, &arch_dummyhandler},
    {0xF6, &arch_dummyhandler},
    {0xF7, &arch_dummyhandler},
    {0xF8, &arch_dummyhandler},
    {0xF9, &arch_dummyhandler},
    {0xFA, &arch_dummyhandler},
    {0xFB, &arch_dummyhandler},
    {0xFC, &arch_dummyhandler},
    {0xFD, &arch_dummyhandler},
    {0xFE, &arch_dummyhandler},
    {0xFF, &arch_dummyhandler}
};
