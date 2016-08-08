/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <app.h>
#include <debug.h>
#include <err.h>
#include <reg.h>

#include <platform/bcm28xx.h>
#include <platform/usb_dwc_regs.h>
#include <platform/interrupts.h>

#define USB_POWER (1 << 3)

#define MAILBOX_READ   0
#define MAILBOX_STATUS 6
#define MAILBOX_WRITE  8

#define MAILBOX_FULL   0x80000000
#define MAILBOX_EMPTY  0x40000000

#define MAILBOX_REGS_BASE (ARMCTRL_0_SBM_BASE + 0x80)

#define MAILBOX_CHANNEL_MASK 0xf

static volatile uint *const mailbox_regs = (volatile uint*)MAILBOX_REGS_BASE;
static volatile struct dwc_regs * const regs = (void*)USB_BASE;

static uint32_t mailbox_read(uint32_t channel)
{
    while (mailbox_regs[MAILBOX_STATUS] & MAILBOX_EMPTY);

    uint32_t val;
    while (true) {
        val = mailbox_regs[MAILBOX_READ];
        if ((val & MAILBOX_CHANNEL_MASK) == channel) break;
    }
    return val & ~MAILBOX_CHANNEL_MASK; 
}

static void mailbox_write(uint32_t channel, uint32_t value)
{
    while (mailbox_regs[MAILBOX_STATUS] & MAILBOX_FULL);
    mailbox_regs[MAILBOX_WRITE] = (value & ~MAILBOX_CHANNEL_MASK) | channel;

}

static enum handler_return usb_irq(void *arg)
{
    union dwc_core_interrupts interrupts;
    interrupts.val = readl(&regs->core_interrupts.val);

    if (interrupts.port_intr) {
        printf("[USB] Port Interrupt. IRQs = 0x%x\n", interrupts.val);
        union dwc_host_port_ctrlstatus hw_status = regs->host_port_ctrlstatus;
        printf("[USB] Host Port CTRL status = 0x%x\n", hw_status.val);
        hw_status.enabled = 0;
        regs->host_port_ctrlstatus = hw_status;

    }

    return INT_RESCHEDULE;
}

static int usb_worker(void *arg)
{
    return 0;
}

static void rpi_usb_entry(const struct app_descriptor *app, void *args)
{
    // Power on the device.
    printf("[USB] Powering on the USB device.\n");
    uint32_t pwr_reg = 0;

    pwr_reg |= USB_POWER;
    mailbox_write(0, pwr_reg << 4);

    printf("[USB] Wrote 0x%x to the power register.\n", USB_POWER);

    pwr_reg = (mailbox_read(0) >> 4);
    if (pwr_reg & USB_POWER) {
        printf("[USB] Power on.\n");
    } else {
        printf("[USB] Power on failed.\n");
        return;
    }

    // Soft Reset the device.
    printf("[USB] Soft Reset.\n");
    writel(DWC_SOFT_RESET, &regs->core_reset);
    while (readl(&regs->core_reset) & DWC_SOFT_RESET);
    printf("[USB] Soft Reset Success.\n");

    // Setup DMA mode.
    printf("[USB] DMA Setup.\n");
    const uint32_t rx_words = 1024;
    const uint32_t tx_words = 1024;
    const uint32_t ptx_words = 1024;
    writel(rx_words, &regs->rx_fifo_size);
    writel((tx_words << 16) | rx_words, &regs->nonperiodic_tx_fifo_size);
    writel((ptx_words << 16) | (rx_words + tx_words), &regs->host_periodic_tx_fifo_size);
    uint32_t ahb_configuration = readl(&regs->ahb_configuration);
    ahb_configuration |= DWC_AHB_DMA_ENABLE | BCM_DWC_AHB_AXI_WAIT;
    writel(ahb_configuration, &regs->ahb_configuration);
    printf("[USB] DMA Setup Success.\n");

    // Set up device interrupts
    printf("[USB] Interrupt setup.\n");
    union dwc_core_interrupts core_interrupt_mask;

    /* Clear all pending core interrupts.  */
    writel(0, &regs->core_interrupt_mask.val);
    writel(0xffffffff, &regs->core_interrupts.val);

    /* Enable core host channel and port interrupts.  */
    core_interrupt_mask.val = 0x0;
    core_interrupt_mask.host_channel_intr = 1;
    core_interrupt_mask.port_intr = 1;
    regs->core_interrupt_mask = core_interrupt_mask;

    /* Enable the interrupt line that goes to the USB controller and register
     * the interrupt handler.  */
    // Setup an interrupt handler for USB.
    register_int_handler(INTERRUPT_VC_USB, &usb_irq, NULL);
    register_int_handler(INTERRUPT_USB, &usb_irq, NULL);

    // Unmask the interrupt.
    unmask_interrupt(INTERRUPT_VC_USB);
    unmask_interrupt(INTERRUPT_USB);

    /* Enable interrupts for entire USB host controller.  (Yes that's what we
     * just did, but this one is controlled by the host controller itself.)  */
    
    ahb_configuration = readl(&regs->ahb_configuration);
    ahb_configuration |= DWC_AHB_INTERRUPT_ENABLE;
    writel(ahb_configuration, &regs->ahb_configuration);
    printf("[USB] Interrupt setup success. AHB Config = 0x%x\n",
           regs->ahb_configuration);

    // Start accepting xfers.

    // Spin on the interrupt register and see if any interrupts come in.
    // volatile union dwc_core_interrupts interrupts = regs->core_interrupts;
    // while (true) {
    //     hexdump(ARMCTRL_INTC_BASE, 12);
    // }

    // Attach the Root Hub
    // Read the Root Hub's device descriptor.
    union dwc_host_port_ctrlstatus hw_status;
    hw_status.enabled = 0;
    hw_status.connected_changed = 0;
    hw_status.enabled_changed = 0;
    hw_status.overcurrent_changed = 0;
    hw_status.powered = 1;
    regs->host_port_ctrlstatus = hw_status;

}


// Need to implement thread to Submit USB Xfers


APP_START(ndebugtest)
  .entry = rpi_usb_entry,
APP_END