APP =		device-model
MACHINE =	mips

CC =		clang-cheri
LD =		ld.lld-cheri
OBJCOPY =	llvm-objcopy-cheri
SIZE =		llvm-size60

OBJDIR =	obj
OSDIR =		mdepx

LDSCRIPT_TPL =	${CURDIR}/ldscript.tpl
LDSCRIPT =	${OBJDIR}/ldscript

DM_BASE_UNCACHED =	0xffffffffb0000000
DM_BASE_CACHED =	0xffffffff90000000
DM_BASE ?=		${DM_BASE_CACHED}

OBJECTS =							\
	bhyve/bhyve_support.o					\
	bhyve/block_if.o					\
	bhyve/mem.o						\
	bhyve/pci_ahci.o					\
	bhyve/pci_emul.o					\
	bhyve/pci_e82545.o					\
	bhyve/pthread.o						\
	device-model.o						\
	emul_iommu.o						\
	emul_msgdma.o						\
	emul_pci.o						\
	fwd_device.o						\
	main.o							\
	start.o							\
	test.o							\
	tlb.o							\
	tlbmiss.o						\
	${OSDIR}/lib/libc/gen/assert.o				\
	${OSDIR}/kernel/dev/altera/fifo/fifo.o			\
	${OSDIR}/kernel/dev/altera/jtag_uart/jtag_uart.o	\
	${OSDIR}/kernel/mips/beri/beripic.o			\
	${OSDIR}/kernel/mips/beri/beri_epw.o			\

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

CFLAGS = -march=mips64 -mcpu=mips64 -G0 -O0 -g -nostdinc	\
	--target=mips64-unknown-freebsd				\
	-mno-abicalls -msoft-float -fwrapv -fno-builtin-printf	\
	${WARNFLAGS} -DWITHOUT_CAPSICUM=1 -DDM_BASE=${DM_BASE}

ifdef DM_IOMMU
CFLAGS += -DALTERA_MSGDMA_DESC_PF_EXT -DMDX_MIPS_TLB -DCONFIG_IOMMU
else
CFLAGS += -DALTERA_MSGDMA_DESC_PF_STD
endif

all:	${OBJDIR}/${APP}.bin

${LDSCRIPT}:
	@sed s#__DM_BASE__#${DM_BASE}#g ${LDSCRIPT_TPL} > ${LDSCRIPT}

llvm-objdump:
	llvm-objdump-cheri -d ${APP}.elf | less

clean:
	@rm -f ${LDSCRIPT} ${OBJECTS}

include ${OSDIR}/mk/default.mk
