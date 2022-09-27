# Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

include $(NV_TOPDIR)/drive-linux/make/nvdefs.mk
TARGETS = nvsipl_multicast_mmt

CPPFLAGS := $(NV_PLATFORM_CPPFLAGS) $(NV_PLATFORM_SDK_INC) $(NV_PLATFORM_CXXFLAGS)
CPPFLAGS += -I./platform
CPPFLAGS += -std=c++14 -fexceptions -frtti -fPIC
LDFLAGS := $(NV_PLATFORM_SDK_LIB) $(NV_PLATFORM_TARGET_LIB) $(NV_PLATFORM_LDFLAGS)

OBJS := CPoolManager.o
OBJS += CProducer.o
OBJS += CSIPLProducer.o
OBJS += CConsumer.o
OBJS += CCudaConsumer.o
OBJS += CClientCommon.o
OBJS += CEncConsumer.o
OBJS += CUtils.o
OBJS += main.o

LDLIBS += -lnvsipl
LDLIBS += -lnvsipl_query
LDLIBS += -lnvmedia
LDLIBS += -lnvscistream
LDLIBS += -lnvscibuf
LDLIBS += -lnvscisync
LDLIBS += -lnvscievent
LDLIBS += -lnvsciipc
LDLIBS += -lnvscicommon
LDLIBS += -lcuda
ifeq ($(NV_PLATFORM_OS),QNX)
  LDLIBS += $(NV_PLATFORM_CUDA_LIB)/libcudart_static.a
else
  LDLIBS += -L$(NV_PLATFORM_CUDA_TOOLKIT)/targets/aarch64-linux/lib/ -lcudart
  LDLIBS += -lpthread
endif


.PHONY: default
default: $(TARGETS)
$(TARGETS): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
clean clobber:
	rm -rf $(OBJS) $(TARGETS)
