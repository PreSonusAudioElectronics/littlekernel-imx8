LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/app_external.c \
    $(LOCAL_DIR)/cxa.cpp \

# Make Pep LK port dir visible to LK build
GOTHAM_DIR := $(LOCAL_DIR)../../../../..
PEP_PORT_DIR := $(abspath $(GOTHAM_DIR)/lib/pep/port/lk)

APP_BUILD_DIR := $(abspath $(GOTHAM_DIR)/../build/)
PEP_BUILD_DIR := $(abspath $(APP_BUILD_DIR)/lib/pep/)

# Guide to external libraries
EXTRA_LINK_LIBS += app pep
GLOBAL_LDFLAGS += -L$(APP_BUILD_DIR) \
                  -L$(PEP_BUILD_DIR) \

# Set dependencies on the library outputs
EXTRA_LINKER_DEPS += libapp.a \
                     libpep.a \


# Make LK use the pep print mutex once the application has started
GLOBAL_DEFINES += LK_USE_PRINT_LOCK

GLOBAL_INCLUDES += $(PEP_PORT_DIR)

include make/module.mk
