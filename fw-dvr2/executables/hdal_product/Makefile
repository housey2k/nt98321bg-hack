MODULE_NAME = dvr_fpga
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
HDAL_PRODUCT_DIR = $(NVT_HDAL_DIR)/samples/hdal_product
RTOS_OUTPUT_DIR = $(HDAL_PRODUCT_DIR)/DVR_FPGA/output
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
KDRV_INC_PATH = $(NVT_HDAL_DIR)/drivers/k_driver/include
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
HDAL_LIB_PATH = $(NVT_HDAL_DIR)/output
# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(VOS_INC_PATH) -I$(KDRV_INC_PATH) -I$(NVT_HDAL_DIR)/vendor/isp/include -I$(HDAL_PRODUCT_DIR)/libfdt -I$(NVT_HDAL_DIR)/vendor/media/include
.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
LD_LIB = 	-L$(NVT_HDAL_DIR)/source \
			-L$(NVT_HDAL_DIR)/vendor/isp/source \
			-L$(NVT_HDAL_DIR)/samples/hdal_product/libfdt \
			-L$(NVT_HDAL_DIR)/drivers/u_driver/source/2d_lib \
			-L$(NVT_HDAL_DIR)/vendor/media/source \

WARNING	= -Wall -Wundef -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX_USER__

C_CFLAGS	= $(PLATFORM_CFLAGS) $(ASAN_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)

LDFLAGS += $(LD_LIB) -lpthread -lhdal -lvendor_isp -lfdt -lgm2d -lvendor_media

SRC = module_init.c show_logo.c

OBJS = $(SRC:.c=.o)

TARGETS = module_init

%.o:%.c
	$(CC) $(C_CFLAGS) $< -c -o $@

all: $(TARGETS)
	make -C DVR_FPGA
	make -C DVR_4CH
	make -C DVR_4CH_mini
	make -C DVR_8CH
	make -C DVR_8CH_max
	make -C NVR_4CH
	make -C NVR_8CH

$(TARGETS):  $(OBJS) Makefile
	make -C libfdt
	$(CC) $(C_CFLAGS) $(LDFLAGS) -o $@ $(OBJS)
	rm -f *.o
	$(STRIP) $@


install:
	make -C libfdt install
	make -C DVR_4CH install
	make -C DVR_4CH_mini install
	make -C DVR_8CH install
	make -C DVR_8CH_max install
	make -C NVR_4CH install
	make -C NVR_8CH install

clean:
	rm -f $(TARGETS)
	rm -rf output
	make -C libfdt clean
	make -C DVR_FPGA  clean
	make -C DVR_4CH  clean
	make -C DVR_4CH_mini  clean
	make -C DVR_8CH  clean
	make -C DVR_8CH_max  clean
	make -C NVR_4CH  clean
	make -C NVR_8CH  clean


###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = lib$(MODULE_NAME).a
SRC = 

OBJ = $(SRC:.c=.o)

all: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	@echo Creating $@...
	@$(AR) rcsD $@ $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so*

install:
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf $(LIB_NAME) $(RTOS_OUTPUT_DIR)
endif
