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

WARNFLAGS =			\
	-Werror			\
	-Wall			\
	-Wmissing-prototypes	\
	-Wredundant-decls	\
	-Wnested-externs	\
	-Wstrict-prototypes	\
	-Wmissing-prototypes	\
	-Wpointer-arith		\
	-Winline		\
	-Wcast-qual		\
	-Wundef			\
	-Wno-pointer-sign	\
	-Wmissing-include-dirs

ifdef DM_IOMMU
DM_FLAGS = -DALTERA_MSGDMA_DESC_PF_EXT -DMDX_MIPS_TLB -DCONFIG_IOMMU
else
DM_FLAGS = -DALTERA_MSGDMA_DESC_PF_STD
endif

export CFLAGS = -march=mips64 -mcpu=mips64 -G0 -O0 -g -nostdinc	\
	--target=mips64-unknown-freebsd				\
	-mno-abicalls -msoft-float -fwrapv -fno-builtin-printf	\
	${WARNFLAGS} ${DM_FLAGS} -DWITHOUT_CAPSICUM=1		\
	-DDM_BASE=${DM_BASE}

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
