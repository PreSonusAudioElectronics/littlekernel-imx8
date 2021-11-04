LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := imx8mn/dts

MODULES += \
	app/external \

MODULES += \
        dev/ivshmem/services/console \
        dev/ivshmem/services/binary \
        dev/ivshmem/services/rpc \

#GLOBAL_DEFINES += \

#EXTRA_OBJS := libapp.a

include project/virtual/test.mk

