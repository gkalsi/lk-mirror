LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/usbcore.c \
	$(LOCAL_DIR)/usbdebug.c \
	$(LOCAL_DIR)/bcm2835_power.c \
	$(LOCAL_DIR)/usb_dwc_hcd.c \
	$(LOCAL_DIR)/usbhub.c

include make/module.mk