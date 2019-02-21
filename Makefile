.include "${.CURDIR}/osfive/mk/bsd.pre.mk"

APP =		device-model
ARCH =		mips

CC =		clang-cheri
LD =		ld.lld-cheri
OBJCOPY =	llvm-objcopy-cheri

LDSCRIPT_TPL =	${.CURDIR}/ldscript.tpl
LDSCRIPT =	${.OBJDIR}/ldscript

DM_BASE_UNCACHED =	0xffffffffb0000000
DM_BASE_CACHED =	0xffffffff90000000
DM_BASE ?=		${DM_BASE_UNCACHED}

OBJECTS =	alloc.o						\
		bhyve/bhyve_support.o				\
		bhyve/block_if.o				\
		bhyve/mem.o					\
		bhyve/pci_ahci.o				\
		bhyve/pci_emul.o				\
		bhyve/pci_e82545.o				\
		bhyve/pthread.o					\
		device-model.o					\
		emul_msgdma.o					\
		emul_pci.o					\
		fwd_device.o					\
		main.o						\
		${OSDIR}/lib/libc/gen/assert.o			\
		${OSDIR}/lib/md5/md5.o				\
		${OSDIR}/sys/dev/altera/fifo/fifo.o		\
		${OSDIR}/sys/dev/altera/jtag_uart/jtag_uart.o	\
		${OSDIR}/sys/kern/kern_malloc_fl.o		\
		${OSDIR}/sys/kern/kern_panic.o			\
		${OSDIR}/sys/kern/subr_console.o		\
		${OSDIR}/sys/kern/subr_prf.o			\
		${OSDIR}/sys/mips/beri/beripic.o		\
		${OSDIR}/sys/mips/beri/beri_epw.o		\
		${OSDIR}/sys/mips/mips/cache_mipsNN.o		\
		${OSDIR}/sys/mips/mips/exception.o		\
		${OSDIR}/sys/mips/mips/machdep.o		\
		${OSDIR}/sys/mips/mips/timer.o			\
		${OSDIR}/sys/mips/mips/trap.o			\
		start.o

LIBRARIES = LIBC

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

CFLAGS = -march=mips64 -mcpu=mips4 -G0 -O -g -nostdinc		\
	 -mno-abicalls -msoft-float -fwrapv -fno-builtin-printf	\
	${WARNFLAGS} -DWITHOUT_CAPSICUM=1
CFLAGS += -DDM_BASE=${DM_BASE}

all:	__compile __link __binary

${LDSCRIPT}:
	sed s#__DM_BASE__#${DM_BASE}#g ${LDSCRIPT_TPL} > ${LDSCRIPT}

llvm-objdump:
	llvm-objdump-cheri -d ${APP}.elf | less

clean: __clean
	rm -f ${LDSCRIPT}

.include "${.CURDIR}/${OSDIR}/lib/libc/Makefile.inc"
.include "${.CURDIR}/${OSDIR}/mk/bsd.post.mk"
