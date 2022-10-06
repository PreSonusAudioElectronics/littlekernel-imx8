LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/ivshmem-serial.c
MODULE_SRCS += $(LOCAL_DIR)/ivshmem-msg.c

MODULE_DEPS += dev/ivshmem

include make/module.mk
