/*
 * Copyright (c) 2016 Gurjant Kalsi <me@gurjantkalsi.com>
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

#include <dev/usbhost/interface.h>
#include <dev/usbhost/usbhost.h>

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <debug.h>


typedef struct __PACKED usbhost_device_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} usbhost_device_descriptor_t;

typedef struct usbhost_device {
    struct usbhost_device *parent;
    usbhost_device_descriptor_t descriptor;
    struct usb_setup setup;
} usbhost_device_t;

static usbhost_device_t root_hub;

void usbhost_init_device(usbhost_device_t *device)
{
    // Zero out the device descriptor.
    memset((uint8_t *)&(device->descriptor), 0, sizeof(device->descriptor));

    // Set the parent device to NULL for now.
    device->parent = NULL;
}

status_t usbhost_transfer(usbhost_device_t * dev, uint ep, uint8_t *buf, size_t n)
{
    // Submit a transfer to the USB host controller.
    status_t rc = usbhost_queue_transfer(dev, ep, buf, n);

    return NO_ERROR;
}

status_t usbhost_device_connected(usbhost_device_t * dev)
{
    DEBUG_ASSERT(dev);

    // Read the device descriptor 8 bytes at a time since we don't know what the
    // max packet size is yet. Once the descriptor has been read, we can start
    // using the real packet size.
    usbhost_device_descriptor_t temp_descriptor;
    dev->descriptor.bMaxPacketSize = 8;
    status_t rc = usbhost_transfer(
        dev,
        0,      // Control endpoint.
        (uint8_t *)&temp_descriptor,
        sizeof(temp_descriptor)
    );

    if (rc != NO_ERROR) {
        printf("Could not read device descriptor for USB device.\n");
        return rc;
    }

    // These should really be the same struct, so it stands to reason that
    // they're the same size.
    STATIC_ASSERT(sizeof(temp_descriptor) == sizeof(dev->descriptor));
    memcpy(
        (uint8_t *)&(dev->descriptor),
        (uint8_t *)&(temp_descriptor),
        sizeof(temp_descriptor)
    );

    // Set the USB address on the device.

    // At this point we know how many configurations the device has. For the
    // time being, we're going to set the default (0th) configuration on the
    // device.

    // TODO(gkalsi): At some point we need to read the endpoint descriptors? 

    // The device was enumerated successfully! We can begin talking to it.
    return NO_ERROR;
}

status_t usbhost_init(void)
{
    status_t rc;

    // Start the USB host controller.
    rc = usbhost_start();
    if (rc != NO_ERROR) {
        printf("Error starting USB host controller. Retcode = %d\n", rc);
        usbhost_stop();
        return rc;
    }

    // Read the device descriptor from the controller's root hub.
    usbhost_init_device(&root_hub);
    usbhost_device_connected(&root_hub);

    return NO_ERROR;
}