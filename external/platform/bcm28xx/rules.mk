LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
    $(LOCAL_DIR)/source/configuration.c \
    $(LOCAL_DIR)/source/device/hub.c \
    $(LOCAL_DIR)/source/platform/platform.c \
    $(LOCAL_DIR)/source/platform/arm/broadcom2835.c \
    $(LOCAL_DIR)/source/platform/arm/armv6.c \
    $(LOCAL_DIR)/source/usbd/usbd.c \
    $(LOCAL_DIR)/source/hcd/dwc/roothub.c \
    $(LOCAL_DIR)/source/hcd/dwc/designware20.c \

    # $(LOCAL_DIR)/source/device/hid/mouse.c \
    # $(LOCAL_DIR)/source/device/hid/keyboard.c \
    # $(LOCAL_DIR)/source/device/hid/hid.c \

include $(LOCAL_DIR)/CMSIS/rules.mk

include make/module.mk

