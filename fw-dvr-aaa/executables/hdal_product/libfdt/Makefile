#--------- ENVIRONMENT SETTING --------------------
CC = $(CROSS_COMPILE)cc
AR = $(CROSS_COMPILE)ar
INCLUDE_DIR = ./
INCLUDES	= -I$(INCLUDE_DIR)
WARNING	= -Wall -Wundef  -Wno-missing-braces -Wstrict-prototypes
COMPILE_OPTS	= $(INCLUDES) -I. -O2 -fPIC -ffunction-sections -fdata-sections
CPPFLAGS	=
CFLAGS		= $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS)
C_FLAGS		= $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING)
OUTPUT_DIR = $(NVT_HDAL_DIR)/output
#LD_FLAGS	= -shared -L$(LIBRARY_DIR)
#--------- END OF ENVIRONMENT SETTING -------------

#--------- Compiling --------------------
SRC = fdt.c fdt_ro.c fdt_wip.c fdt_sw.c fdt_rw.c fdt_strerror.c fdt_empty_tree.c fdt_addresses.c fdt_overlay.c

copy = if [ ! -z "$(1)" -a "$(1)" != " " ]; then cp -avf $(1) $(2); fi
STATIC_LIB = libfdt.a
DYNAMIC_LIB = libfdt.so

OBJ = $(SRC:.c=.o)
TARGETS := $(STATIC_LIB) $(DYNAMIC_LIB)
.PHONY: $(TARGETS)

all: $(TARGETS)

$(STATIC_LIB): $(OBJ)
	$(AR) rcs $@ $(OBJ)

$(DYNAMIC_LIB) : $(OBJ)
	$(CC) -shared -s -o $@ $(OBJ)


%.o:%.c
	$(CC) $(C_FLAGS) $< -c -o $@

clean:
	rm -vf $(OBJ) *.o *.a *.so*

install_header:
	@$(call copy, $(INC), $(INCLUDE_DIR))
	@$(call copy, $(INC_PROTECT), $(INCLUDE_DIR)/protected)

install: install_header
	@cp -avf *.so* *.a $(LIBRARY_DIR)
	@cp  -avf *.so *.a $(OUTPUT_DIR)

