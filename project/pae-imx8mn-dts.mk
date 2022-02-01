LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := imx8mn/dts

MODULES += \
	app/external \
	lib/klog \
	lib/version \
	lib/pool \

MODULES += \
        dev/ivshmem/services/console \
        dev/ivshmem/services/binary \
        dev/ivshmem/services/rpc \
		dev/ivshmem/services/serial \
		dev/dac/wm8524 \
		dev/pinctrl

# GLOBAL_OPTFLAGS += -g3 -Og
# DEBUG := 2

#GLOBAL_DEFINES += \

#EXTRA_OBJS := libapp.a

include project/virtual/test.mk

