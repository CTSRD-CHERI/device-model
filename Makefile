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

export CFLAGS = ${DM_FLAGS} -DDM_BASE=${DM_BASE}
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
