LOCAL_DIR := $(GET_LOCAL_DIR)


MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/app_external.c \

#EXTRA_OBJS :=

include make/module.mk
