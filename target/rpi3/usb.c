/*
 * Copyright (c) 2016 Gurjant Kalsi
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
 */

#include <debug.h>
#include <err.h>
#include <trace.h>
#include <reg.h>

#include <platform/bcm28xx.h>
#include <platform/usb_dwc_regs.h>  // TODO(gkalsi): This file needs to be moved into third_party
#include <platform/interrupts.h>

#include <dev/usbhost/interface.h>
#include <hw/usb.h>

#define LOCAL_TRACE 1

////////////////////////////////////////////////////////////////////////////////
/// TODO(gkalsi): Everything within this section is particular to the BCM28XX
/// It should be moved to the platform specific code.
////////////////////////////////////////////////////////////////////////////////

#define USB_POWER (1 << 3)

#define MAILBOX_READ   0
#define MAILBOX_STATUS 6
#define MAILBOX_WRITE  8

#define MAILBOX_FULL   0x80000000
#define MAILBOX_EMPTY  0x40000000

#define MAILBOX_REGS_BASE (ARMCTRL_0_SBM_BASE + 0x80)

#define MAILBOX_CHANNEL_MASK 0xf

static volatile uint *const mailbox_regs = (volatile uint*)MAILBOX_REGS_BASE;

static void mailbox_write(uint32_t channel, uint32_t value)
{
    while (mailbox_regs[MAILBOX_STATUS] & MAILBOX_FULL);
    mailbox_regs[MAILBOX_WRITE] = (value & ~MAILBOX_CHANNEL_MASK) | channel;
}

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

////////////////////////////////////////////////////////////////////////////////
/// END OF PLATFORM SPECIFIC CODE.
////////////////////////////////////////////////////////////////////////////////

static volatile struct dwc_regs * const regs = (void*)USB_BASE;


static enum handler_return usb_irq(void *arg)
{
    printf("USB IRQ\n");
    return INT_RESCHEDULE;
}

status_t usbhost_start(void)
{
    LTRACE_ENTRY;

    // Power on the USB host device.
    LTRACEF("Powering on USB Device.\n");
    uint32_t power_register = USB_POWER;
    mailbox_write(0, power_register << 4);

    // Ensure that the USB host device was powered on.
    power_register = (mailbox_read(0) >> 4);
    if ((power_register & USB_POWER) == 0) {
        printf("Failed to power on USB host controller.\n");
        return ERR_GENERIC;
    }

    // Perform a soft reset on the device.
    writel(DWC_SOFT_RESET, &regs->core_reset);
    while (readl(&regs->core_reset) & DWC_SOFT_RESET);

    // Setup the device's DMA mode.
    const uint32_t rx_words = 1024;
    const uint32_t tx_words = 1024;
    const uint32_t ptx_words = 1024;
    writel(rx_words, &regs->rx_fifo_size);
    writel((tx_words << 16) | rx_words, &regs->nonperiodic_tx_fifo_size);
    writel((ptx_words << 16) | (rx_words + tx_words), &regs->host_periodic_tx_fifo_size);
    uint32_t ahb_configuration = readl(&regs->ahb_configuration);
    ahb_configuration |= DWC_AHB_DMA_ENABLE | BCM_DWC_AHB_AXI_WAIT;
    writel(ahb_configuration, &regs->ahb_configuration);

    // Disable all interrupts coming in from the USB device.
    writel(0, &regs->core_interrupt_mask.val);

    // Clear any pending interrupts on the device.
    writel(0xffffffff, &regs->core_interrupts.val);

    // Unmask interrupts from the core that we're interested in.
    union dwc_core_interrupts core_interrupt_mask;
    core_interrupt_mask.val = 0x0;
    core_interrupt_mask.host_channel_intr = 1;
    core_interrupt_mask.port_intr = 1;
    regs->core_interrupt_mask = core_interrupt_mask;

    // Unmask the interrupt line from the USB Controller to the CPU and register
    // an interrupt handler for this interrupt.
    register_int_handler(INTERRUPT_VC_USB, &usb_irq, NULL);
    unmask_interrupt(INTERRUPT_VC_USB);

    // Enable interrupts from the USB controller.
    ahb_configuration = readl(&regs->ahb_configuration);
    ahb_configuration |= DWC_AHB_INTERRUPT_ENABLE;
    writel(ahb_configuration, &regs->ahb_configuration);

    return NO_ERROR;
}

status_t usbhost_stop(void)
{
    return NO_ERROR;
}

status_t usbhost_queue_transfer(usthost_device_t *dev, uint ep, uint8_t *buf, size_t n);
{
    if (dev->parent == NULL) {
        // The Root Hub is technically part of the controller so we don't 
        // actually queue any transfers to it. Instead we fill in the transfer
        // details in software and pretend that a transfer went through.
        
    } else {
        dwc_
    }

    return NO_ERROR;
}
