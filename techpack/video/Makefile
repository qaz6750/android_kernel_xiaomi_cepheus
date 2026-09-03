# SPDX-License-Identifier: GPL-2.0-only

# auto-detect subdirs
ifeq ($(CONFIG_ARCH_KONA), y)
include $(srctree)/techpack/video/config/konavid.conf
endif

ifeq ($(CONFIG_ARCH_KONA), y)
LINUXINCLUDE    += -include $(srctree)/techpack/video/config/konavidconf.h
endif

ifeq ($(CONFIG_ARCH_SM8150), y)
include $(srctree)/techpack/video/config/msmnilevid.conf
endif

ifeq ($(CONFIG_ARCH_SM8150), y)
LINUXINCLUDE    += -include $(srctree)/techpack/video/config/msmnilevidconf.h
endif

# auto-detect subdirs
ifeq ($(CONFIG_ARCH_BENGAL), y)
include $(srctree)/techpack/video/config/bengalvid.conf
endif

ifeq ($(CONFIG_ARCH_BENGAL), y)
LINUXINCLUDE    += -include $(srctree)/techpack/video/config/bengalvidconf.h
endif

LINUXINCLUDE    += -I$(srctree)/techpack/video/include \
                   -I$(srctree)/techpack/video/include/uapi

USERINCLUDE     += -I$(srctree)/techpack/video/include/uapi

obj-y +=msm/
