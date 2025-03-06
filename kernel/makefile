HOSTC=gcc
HOSTCPP=g++
C_STD=c11
CPP_STD=c++20

ARCH_HOST=$(shell uname -m)

C_INCLUDE_PATH=-I./includes

ARCH_DIR=hal/$(ARCH)/

# choose tools for arch
ifneq ($(ARCH_HOST),$(ARCH))
	CC = /usr/bin/$(ARCH)-linux-gnu-gcc
	ASM = /usr/bin/$(ARCH)-linux-gnu-as
	LD = /usr/bin/$(ARCH)-linux-gnu-ld
	NM = /usr/bin/$(ARCH)-linux-gnu-nm
	OBJCOPY = /usr/bin/$(ARCH)-linux-gnu-objcopy
else
	CC = gcc
	ASM = as
	LD = ld
	NM = nm
	OBJCOPY = objcopy
endif

ifeq ($(ARCH_HOST),x86_64)
	C_FLAGS_ARCH=-m64
	ASM_FLAGS_ARCH=--64
	LD_FLAGS_ARCH=-b elf64-x86-64 -b elf64-x86-64
	OBJCOPY_FLAGS_ARCH=-I elf64-x86-64 -S -R ".eh_frame" -R ".comment"
endif

C_FLAGS= \
	$(C_FLAGS_ARCH) \
	$(C_INCLUDE_PATH) \
	--std=$(C_STD) \
	-mcmodel=large \
	-fno-builtin -fno-stack-protector \
	-Wno-packed-bitfield-compat \
	-Wno-address-of-packed-member \
	-mno-red-zone \
	-fmerge-constants \

GEN_C_FLAGS= \
	--std=$(C_STD) \
	$(C_INCLUDE_PATH) \
	-fno-builtin -fno-stack-protector \
	-Wno-packed-bitfield-compat \
	-Wno-address-of-packed-member \
	-mno-red-zone \
	-fmerge-constants \

ASM_FLAGS=$(ASM_FLAGS_ARCH)

LD_FLAGS=$(LG_FLAGS_ARCH) -z muldefs --no-warn-rwx-segments -z noexecstack

OBJCOPY_FLAGS=$(OBJCOPY_FLAGS_ARCH) -O binary

ALLGENOBJS=
ALLGENSCRIPTS=
include ./.gendepend

ALLOBJS=
ALLTMPS=
include ./.depend

rely: ./Tools/rely
	@./Tools/rely $(ARCH) CC C_FLAGS o .depend . ALLOBJS ALLTMPS Tools 
	@echo [SCRIPT] build rely
genrely: ./Tools/rely
	@./Tools/rely $(ARCH) HOSTC GEN_C_FLAGS script .gendepend ./Tools/hal/gen ALLGENOBJS ALLGENSCRIPTS
	@echo [SCRIPT] build generation rely

./Tools/gen: ./Tools/gen.cpp
	@$(HOSTCPP) ./Tools/gen.cpp -o ./Tools/gen --std=$(CPP_STD)
	@echo "[HOSTCPP] ./Tools/gen.cpp"

gen: $(ALLGENSCRIPTS)
	@echo "[GEN] generate"

./Tools/rely: ./Tools/rely.cpp
	@$(HOSTCPP) ./Tools/rely.cpp -o ./Tools/rely --std=$(CPP_STD)
	@echo "[HOSTCPP] ./Tools/rely.cpp"


all: system_tmp ./Tools/hal/kallsyms.o
	@$(LD) $(LD_FLAGS) -o system $(ALLOBJS) ./Tools/hal/kallsyms.o -T $(ARCH_DIR)/kernel.lds 
	@echo "[LD] system"
	@$(OBJCOPY) $(OBJCOPY_FLAGS) system kernel.bin
	@echo "[OBJDUMP] kernel.bin"

system_tmp: $(ALLOBJS)
	@$(LD) $(LD_FLAGS) $(LDFLAGS_ARCH) -o system_tmp $(ALLOBJS) -T $(ARCH_DIR)/kernel.lds 
	@echo "[LD] system_tmp"

./Tools/hal/kallsyms: ./Tools/hal/kallsyms.c
	@$(HOSTC) -o ./Tools/hal/kallsyms ./Tools/hal/kallsyms.c
	@echo "[HOSTC] ./Tools/hal/kallsyms.c"

./Tools/hal/kallsyms.o: ./Tools/hal/kallsyms system_tmp
	@$(NM) -n system_tmp | ./Tools/hal/kallsyms > ./Tools/hal/kallsyms.S
	@$(CC) -c ./Tools/hal/kallsyms.S -o ./Tools/hal/kallsyms.o -Wl,--no-warn-rwx-segments
	@echo "[GEN] kallsyms.o"

clean:
	rm -rf $(ALLOBJS) $(ALLTMPS) $(ALLGENOBJS) $(ALLGENTMPS)
	rm system system_tmp kernel.bin ./Tools/hal/kallsyms.o ./Tools/hal/kallsyms.S ./Tools/hal/kallsyms ./Tools/rely ./Tools/gen
	rm .depend .gendepend .gentask
