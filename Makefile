APP =		device-model
MACHINE =	mips

TRIPLE ?=	mips64-unknown-freebsd-

export CC =	${TRIPLE}clang
export LD =	${TRIPLE}ld.lld
OBJCOPY =	${TRIPLE}objcopy
OBJDUMP =	${TRIPLE}objdump
SIZE =		llvm-size-cheri

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

ifdef DM_PCI
DM_FLAGS += -DCONFIG_EMUL_PCI
endif

ifdef DM_CAP
DM_FLAGS += -DE1000_DESC_CAP
endif

export CFLAGS = --target=mips64c128-unknown-freebsd	\
	-march=beri -mabi=64 -mcpu=beri -cheri=128	\
	-ftls-model=local-exec -nostdinc -G0		\
	-O0 -g						\
	-fno-builtin-printf -ffreestanding		\
	-msoft-float -fwrapv				\
	-fomit-frame-pointer -D__mips_n64 -nostdlib	\
	-DBASE_ADDR=0xffffffff80100000			\
	-mno-abicalls -fno-pic				\
	${DM_FLAGS} -DWITHOUT_CAPSICUM=1		\
	-DDM_BASE=${DM_BASE}

export AFLAGS = ${CFLAGS}

all:	${LDSCRIPT}
	@python3 -B ${OSDIR}/tools/emitter.py mdepx.conf
	@${OBJCOPY} -O binary obj/${APP}.elf obj/${APP}.bin
	@${SIZE} obj/${APP}.elf

${LDSCRIPT}:
	@sed s#__DM_BASE__#${DM_BASE}#g ${LDSCRIPT_TPL} > ${LDSCRIPT}

llvm-objdump:
	@${OBJDUMP} -d ${OBJDIR}/${APP}.elf | less

clean:
	@rm -rf obj/*

include ${OSDIR}/mk/user.mk
