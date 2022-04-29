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
		dev/pinctrl \
		dev/adc/ak55xx \
		dev/pca6416 \

GLOBAL_OPTFLAGS += -Og
GLOBAL_COMPILEFLAGS += -g3
DEBUG = 8

GLOBAL_DEFINES += \
	IMX_SAI_WARMUP_NR_PERIODS=16 \
	IMX_SAI_WARMUP_NR_PERIODS_FRAC=4 \
	AF_LK_LOGLEVEL=5 \

# IMX_SAI_PERMISSIVE=1 \
# global defines for debugging:
AF_LK_LOGLEVEL=7

#EXTRA_OBJS := libapp.a

include project/virtual/test.mk

