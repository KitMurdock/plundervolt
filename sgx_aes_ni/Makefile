URTS_LIB_PATH        = ~/sgx-step/linux-sgx/psw/urts/linux

ifeq ($(SGX_SDK),)
    SGX_SDK          = /opt/intel/sgxsdk
endif
export SGX_SDK
ifneq ($(SGX_SDK), /opt/intel/sgxsdk)
    URTS_LD_LIBRARY_PATH = LD_LIBRARY_PATH=$(LIBSGXSTEP_DIR)/linux-sgx/psw/urts/linux
endif

ENCLAVE              = Enclave
SUBDIRS              = $(ENCLAVE) $(LIBSGXSTEP)

CC                   = gcc
AS                   = gcc
LD                   = gcc

ifeq ($(M32), 1)
	CFLAGS   = -m32 -DM32=$(M32)
	LDFLAGS  = -m32 
else
	LIB_SUFX = 64
endif

LDFLAGS		    += -lncurses
CFLAGS              += -fPIC -fno-stack-protector -fno-builtin -fno-jump-tables \
                       -fno-common -Wno-attributes -g -D_GNU_SOURCE -O0 -mavx2 -march=haswell 
INCLUDE             += -I$(SGX_SDK)/include/ -I$(LIBSGXSTEP_DIR) 
LDFLAGS             +=  -lencl_proxy -lsgx_urts \
                       -lsgx_uae_service -pthread $(SUBDIRS:%=-L %) -L$(SGX_SDK)/lib$(LIB_SUFX)/ \
                       -L$(URTS_LIB_PATH)

SOURCES              = $(shell ls *.c)
CXXSOURCES              = $(shell ls *.cpp)
OBJECTS              = $(SOURCES:.c=.o)
CXXOBJECTS              = $(CXXSOURCES:.cpp=.o)
OUTPUT               = app

BUILDDIRS            = $(SUBDIRS:%=build-%)
CLEANDIRS            = $(SUBDIRS:%=clean-%)

.SILENT:

all: $(OUTPUT)
	
run: clean all
	sudo $(URTS_LD_LIBRARY_PATH) ./$(OUTPUT) 500 -255

debug: clean all
	sudo `which sgx-gdb` ./$(OUTPUT)

$(OUTPUT): $(BUILDDIRS) $(OBJECTS)
	echo "$(INDENT)[LD]" $(OBJECTS) $(LIBS) -o $(OUTPUT) 
	$(LD) $(OBJECTS) $(LDFLAGS) -o $(OUTPUT) 

%.o : %.c 
	echo "$(INDENT)[CC] " $<
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

%.o : %.cpp 
	echo "$(INDENT)[CC] " $<
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

%.o : %.S
	echo "$(INDENT)[AS] " $<
	$(AS) $(INCLUDE) -c $< -o $@

clean: $(CLEANDIRS)
	echo "$(INDENT)[RM]" $(OBJECTS) $(OUTPUT)
	rm -f $(OBJECTS) $(OUTPUT)

$(BUILDDIRS):
	echo "$(INDENT)[===] $(@:build-%=%) [===]"
	$(MAKE) -C $(@:build-%=%) INDENT+="$(INDENT_STEP)" M32=$(M32) curr-dir=$(curr-dir)/$(@:build-%=%)

$(CLEANDIRS):
	echo "$(INDENT)[===] $(@:clean-%=%) [===]"
	
