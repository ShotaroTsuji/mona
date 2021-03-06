/*! \file
    \brief monapi io functions

    \author Higepon
    \date   create:2004/10/30 update:$Date$
    $Revision$
*/
#include <monapi.h>
#include <monapi/io.h>
#include <monapi/syscall.h>
#include <monapi/Message.h>
#include <sys/error.h>

/*!
    control irq interrupt.

    \param irq      [in] irq number 0-15
    \param enabled  [in] enable interrupt?
    \param auto_ir2 [in] if 8 <= irq <= 15 and enables = true, auto_ir2 indicate whether enable ir2.
    \return none
*/
void monapi_set_irq(int irq, MONAPI_BOOL enabled, MONAPI_BOOL auto_ir2)
{
    if (irq >= 0 && irq <= 7)
    {
        if (enabled == MONAPI_TRUE) outp8(0x21, (inp8(0x21) & ~(1 << irq)));
        else outp8(0x21, (inp8(0x21) | (1 << irq)));
    }
    else if (irq >= 8 && irq <= 15)
    {
        if (enabled == MONAPI_TRUE) outp8(0xa1, inp8(0xa1) & ~(1 << (irq - 8)));
        else outp8(0xa1, inp8(0xa1) | (1 << (irq - 8)));

        if (enabled == MONAPI_TRUE && auto_ir2 == MONAPI_TRUE)
        {
            outp8(0x21, (inp8(0x21) & ~(1 << 2)));
        }
    }
}

// no more used!!
// /*!
//     wait interrupt.

//     \param ms     [in] timeout ms
//     \param irq    [in] irq number
//     \param file   [in] file name for time out info.
//     \param line   [in] line number for time out info.
//     \return MONAPI_TRUE/MONAPI_FALSE OK/NG
// */
// MONAPI_BOOL monapi_wait_interrupt(uint32_t ms, uint8_t irq, const char* file, int line)
// {
//     MessageInfo msg;

//     uint32_t timerId = set_timer(ms);

//     for (int i = 0; ; i++)
//     {
//         int result = MonAPI::Message::peek(&msg, i);

//         if (result != M_OK)
//         {
//             i--;
//             syscall_mthread_yield_message();
//         }
//         else if (msg.header == MSG_TIMER)
//         {
//             if (msg.arg1 != timerId) continue;
//             kill_timer(timerId);

//             if (MonAPI::Message::peek(&msg, i, PEEK_REMOVE) != M_OK) {
//                 monapi_fatal("peek error %s:%d\n", __FILE__, __LINE__);
//             }

//             monapi_warn("interrupt timeout %s:%d\n", file, line);
//             return MONAPI_FALSE;
//         }
//         else if (msg.header == MSG_INTERRUPTED)
//         {
//             if (msg.arg1 != irq) continue;
//             kill_timer(timerId);

//             if (MonAPI::Message::peek(&msg, i, PEEK_REMOVE) != M_OK) {
//                 monapi_fatal("peek error %s:%d\n", __FILE__, __LINE__);
//             }

//             return MONAPI_TRUE;
//         }
//     }
//     return MONAPI_FALSE;
// }

void delayMicrosec()
{
    // inp from any port 0-0x3ff takes almost exactly 1 micro second.
    inp8(0x80);
}

uint8_t inp8(uint32_t port) {

    uint8_t ret;
    asm volatile ("inb %%dx, %%al": "=a"(ret): "d"(port));
    return ret;
}

void outp8(uint32_t port, uint8_t value) {
   asm volatile ("outb %%al, %%dx": :"d" (port), "a" (value));
}

uint16_t inp16(uint32_t port) {

    uint16_t ret;
    asm volatile ("inw %%dx, %%ax": "=a"(ret): "d"(port));
    return ret;
}

void outp16(uint32_t port, uint16_t value) {
   asm volatile ("outw %%ax, %%dx": :"d" (port), "a" (value));
}

uint32_t inp32(uint32_t port) {

    uint32_t ret;
    asm volatile ("inl %%dx, %%eax": "=a"(ret): "d"(port));
    return ret;
}

void outp32(uint32_t port, uint32_t value) {
   asm volatile ("outl %%eax, %%dx": :"d" (port), "a" (value));
}

void rdtsc(uint32_t* timeL, uint32_t* timeH)
{
    uint32_t l,h;
    asm volatile("rdtsc           \n"
                 "mov   %%eax, %0 \n"
                 "mov   %%edx, %1 \n"
                 : "=m"(l), "=m"(h)
                 : /* no */
                 : "eax", "edx");
    *timeL = l;
    *timeH = h;
}

void rdtsc_trace(const char* file, int lineno, const char* func)
{
    uint32_t l, h;
    rdtsc(&l, &h);
    _logprintf("%x:%x %s:%d (%s)\n", h, l, file, lineno, func);
}
