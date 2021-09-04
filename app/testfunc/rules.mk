LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/testfunc.c \
    $(LOCAL_DIR)/foobar.cpp \

GLOBAL_INCLUDES += $(LOCAL_DIR)

include make/module.mk
