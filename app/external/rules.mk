LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/app_external.c \
    $(LOCAL_DIR)/cxa.cpp \

# Guide to external libraries
EXTRA_LINK_LIBS += app pep

GLOBAL_LDFLAGS += -L${LK_BUILD_DIR} \
                  -L${LK_BUILD_DIR}/lib/pep \

# Set dependencies on the library outputs
EXTRA_LINKER_DEPS += ${LK_BUILD_DIR}/libapp.a \
                     ${LK_BUILD_DIR}/lib/pep/libpep.a \

include make/module.mk
