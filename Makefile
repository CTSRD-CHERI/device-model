APP =		device-model
MACHINE =	mips

export CC =	clang-cheri
export LD =	ld.lld-cheri
OBJCOPY =	llvm-objcopy-cheri
SIZE =		llvm-size60

OBJDIR =	obj
OSDIR =		mdepx

LDSCRIPT_TPL =	${CURDIR}/ldscript.tpl
LDSCRIPT =	${OBJDIR}/ldscript

DM_BASE_UNCACHED =	0xffffffffb0000000
DM_BASE_CACHED =	0xffffffff90000000
DM_BASE ?=		${DM_BASE_CACHED}

ifdef DM_IOMMU
DM_FLAGS = -DALTERA_MSGDMA_DESC_PF_EXT -DMDX_MIPS_TLB -DCONFIG_IOMMU
else
DM_FLAGS = -DALTERA_MSGDMA_DESC_PF_STD
endif

export CFLAGS = --target=cheri-unknown-freebsd		\
	-march=beri -mabi=64 -mcpu=beri -cheri=128	\
	-cheri-cap-table-abi=pcrel			\
	-ftls-model=local-exec -nostdinc -G0		\
	-O0 -g						\
	-fno-builtin-printf -ffreestanding		\
	-msoft-float -fwrapv				\
	-fomit-frame-pointer -D__mips_n64 -nostdlib	\
	-DBASE_ADDR=0xffffffff80100000			\
	-mno-abicalls -fno-pic				\
	${DM_FLAGS} -DWITHOUT_CAPSICUM=1		\
	-DDM_BASE=${DM_BASE}

export ASFLAGS = ${CFLAGS}

all:	${LDSCRIPT}
	@python3 -B ${OSDIR}/tools/emitter.py mdepx.conf
	@${OBJCOPY} -O binary obj/${APP}.elf obj/${APP}.bin
	@${SIZE} obj/${APP}.elf

${LDSCRIPT}:
	@sed s#__DM_BASE__#${DM_BASE}#g ${LDSCRIPT_TPL} > ${LDSCRIPT}

llvm-objdump:
	llvm-objdump-cheri -d ${APP}.elf | less

clean:
	@rm -rf obj/*

include ${OSDIR}/mk/user.mk
