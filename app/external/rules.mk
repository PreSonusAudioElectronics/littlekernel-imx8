LOCAL_DIR := $(GET_LOCAL_DIR)


MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/app_external.c \
    $(LOCAL_DIR)/cxa.cpp \

# Guide to external libraries
EXTRA_LINK_LIBS += app pep
GLOBAL_LDFLAGS += -L../../ \
                  -L../../lib/pep \

include make/module.mk
