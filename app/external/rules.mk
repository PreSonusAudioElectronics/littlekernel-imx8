LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/app_external.c \
    $(LOCAL_DIR)/cxa.cpp \

# Guide to external libraries
EXTRA_LINK_LIBS += app pep
GLOBAL_LDFLAGS += -L../build/ \
                  -L../build/lib/pep \

# Set dependencies on the library outputs
EXTRA_LINKER_DEPS += ../build/libapp.a \
                     ../build/lib/pep/libpep.a \

$(info )
$(info PAE PROJECT LOCAL_DIR: $(LOCAL_DIR))
GOTHAM_DIR := $(LOCAL_DIR)../../../../../../../..
PEP_PORT_DIR := $(abspath $(GOTHAM_DIR)/lib/pep/port/lk)
$(info PEP_PORT_DIR: $(PEP_PORT_DIR))
$(info )

GLOBAL_INCLUDES += $(PEP_PORT_DIR)

include make/module.mk
