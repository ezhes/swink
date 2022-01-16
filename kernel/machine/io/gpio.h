/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef GPIO_H
#define GPIO_H

#if PLATFORM_RPI3
#define MMIO_BASE       0x3F000000
#define MMIO_END        0x40000000

#define GP_BASE         (MMIO_BASE + 0x00200000)
#define GPFSEL0         ((volatile unsigned int*)(GP_BASE + 0x00))
#define GPFSEL1         ((volatile unsigned int*)(GP_BASE + 0x04))
#define GPFSEL2         ((volatile unsigned int*)(GP_BASE + 0x08))
#define GPFSEL3         ((volatile unsigned int*)(GP_BASE + 0x0C))
#define GPFSEL4         ((volatile unsigned int*)(GP_BASE + 0x10))
#define GPFSEL5         ((volatile unsigned int*)(GP_BASE + 0x14))
#define GPSET0          ((volatile unsigned int*)(GP_BASE + 0x1C))
#define GPSET1          ((volatile unsigned int*)(GP_BASE + 0x20))
#define GPCLR0          ((volatile unsigned int*)(GP_BASE + 0x28))
#define GPLEV0          ((volatile unsigned int*)(GP_BASE + 0x34))
#define GPLEV1          ((volatile unsigned int*)(GP_BASE + 0x38))
#define GPEDS0          ((volatile unsigned int*)(GP_BASE + 0x40))
#define GPEDS1          ((volatile unsigned int*)(GP_BASE + 0x44))
#define GPHEN0          ((volatile unsigned int*)(GP_BASE + 0x64))
#define GPHEN1          ((volatile unsigned int*)(GP_BASE + 0x68))
#define GPPUD           ((volatile unsigned int*)(GP_BASE + 0x94))
#define GPPUDCLK0       ((volatile unsigned int*)(GP_BASE + 0x98))
#define GPPUDCLK1       ((volatile unsigned int*)(GP_BASE + 0x9C))

/* Auxilary mini UART registers */
#define AUX_MU_BASE     (MMIO_BASE + 0x00215000)
#define AUX_MU_ENABLE   ((volatile unsigned int*)(AUX_MU_BASE + 0x04))
#define AUX_MU_IO       ((volatile unsigned int*)(AUX_MU_BASE + 0x40))
#define AUX_MU_IER      ((volatile unsigned int*)(AUX_MU_BASE + 0x44))
#define AUX_MU_IIR      ((volatile unsigned int*)(AUX_MU_BASE + 0x48))
#define AUX_MU_LCR      ((volatile unsigned int*)(AUX_MU_BASE + 0x4C))
#define AUX_MU_MCR      ((volatile unsigned int*)(AUX_MU_BASE + 0x50))
#define AUX_MU_LSR      ((volatile unsigned int*)(AUX_MU_BASE + 0x54))
#define AUX_MU_MSR      ((volatile unsigned int*)(AUX_MU_BASE + 0x58))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(AUX_MU_BASE + 0x5C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(AUX_MU_BASE + 0x60))
#define AUX_MU_STAT     ((volatile unsigned int*)(AUX_MU_BASE + 0x64))
#define AUX_MU_BAUD     ((volatile unsigned int*)(AUX_MU_BASE + 0x68))

/* Power Management Controller */
#define PM_BASE         (MMIO_BASE + 0x00100000)
#define PM_RSTC         ((volatile unsigned int*)(PM_BASE + 0x1c))
#define PM_RSTS         ((volatile unsigned int*)(PM_BASE + 0x20))
#define PM_WDOG         ((volatile unsigned int*)(PM_BASE + 0x24))
#define PM_WDOG_MAGIC   0x5a000000
#define PM_RSTC_FULLRST 0x00000020

/* VideoCore Mailbox */
#define VC_MAILBOX_BASE         (MMIO_BASE+0x0000B880)
#define VC_MAILBOX_READ         ((volatile unsigned int*)(VC_MAILBOX_BASE+0x0))
#define VC_MAILBOX_POLL         ((volatile unsigned int*)(VC_MAILBOX_BASE+0x10))
#define VC_MAILBOX_SENDER       ((volatile unsigned int*)(VC_MAILBOX_BASE+0x14))
#define VC_MAILBOX_STATUS       ((volatile unsigned int*)(VC_MAILBOX_BASE+0x18))
#define VC_MAILBOX_CONFIG       ((volatile unsigned int*)(VC_MAILBOX_BASE+0x1C))
#define VC_MAILBOX_WRITE        ((volatile unsigned int*)(VC_MAILBOX_BASE+0x20))
#define VC_MAILBOX_RESPONSE     0x80000000
#define VC_MAILBOX_FULL         0x80000000
#define VC_MAILBOX_EMPTY        0x40000000


#else 
#error Unsupported platform
#endif /* PLATFORM_RPI3 */
#endif /* GPIO_H */