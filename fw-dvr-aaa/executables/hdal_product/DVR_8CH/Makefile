CC = $(CROSS_COMPILE)cc
CPP = $(CROSS_COMPILE)cpp
AR = $(CROSS_COMPILE)ar
STRIP=$(CROSS_COMPILE)strip
LD=$(CROSS_COMPILE)ld
AS=$(CROSS_COMPILE)as

NVT_MODULE_INIT_DTS = cfg_DVR_8CH
UDRV_LIB=$(NVT_HDAL_DIR)/drivers/u_driver/source/2d_lib
ISP_LIB=$(NVT_HDAL_DIR)/vendor/isp/source
LIB=$(NVT_HDAL_DIR)/source
LDFLAGS += -L$(LIB) -L$(UDRV_LIB) -L$(ISP_LIB) -lpthread -lhdal -lgm2d -lvendor_isp
CFLAGS += $(ASAN_CFLAGS) -Wall -I$(NVT_HDAL_DIR)/include -I$(NVT_HDAL_DIR)/vendor/isp/include

define gen_dtb_from_dts
	$(CPP) $(CFLAGS) -nostdinc  -undef -x assembler-with-cpp $(NVT_MODULE_INIT_DTS).dts > $(NVT_MODULE_INIT_DTS).tmp.dts; \
	dtc -O dtb -b 0 -o $(NVT_MODULE_INIT_DTS).dtb $(NVT_MODULE_INIT_DTS).tmp.dts; 
	 @echo "gen $(NVT_MODULE_INIT_DTS).dtb"
endef

define clean_dtb		
	rm -rf $(NVT_MODULE_INIT_DTS).tmp.dts; \
	rm -rf $(NVT_MODULE_INIT_DTS).dtb; 
endef
TARGETS := dvr_perf
	
all: $(TARGETS)
	@$(call gen_dtb_from_dts)

$(TARGETS): %: %.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	#$(CC) $(CFLAGS) --static -o $@ $< $(LIB)/libhdal.a $(NVT_HDAL_DIR)/vendor/isp/source/libvendor_isp.a $(LDFLAGS)
	$(STRIP) $@

install:
	@echo install to rootfs and create product partition
	sh install.sh
clean:
	@$(call clean_dtb)
	@rm -f $(TARGETS)
	@rm -rf image
	@rm -f DVR*.bin
	@rm -f DVR*.raw
	