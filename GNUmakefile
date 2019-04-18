APP =		device-model
MACHINE =	mips

CC =		clang-cheri
LD =		ld.lld-cheri
OBJCOPY =	llvm-objcopy-cheri

OBJDIR =	${CURDIR}/obj
LDSCRIPT_TPL =	${CURDIR}/ldscript.tpl
LDSCRIPT =	${OBJDIR}/ldscript

DM_BASE_UNCACHED =	0xffffffffb0000000
DM_BASE_CACHED =	0xffffffff90000000
DM_BASE ?=		${DM_BASE_CACHED}

OBJECTS =							\
		${OBJDIR}/bhyve/bhyve_support.o			\
		${OBJDIR}/bhyve/block_if.o			\
		${OBJDIR}/bhyve/mem.o				\
		${OBJDIR}/bhyve/pci_ahci.o			\
		${OBJDIR}/bhyve/pci_emul.o			\
		${OBJDIR}/bhyve/pci_e82545.o			\
		${OBJDIR}/bhyve/pthread.o			\
		${OBJDIR}/device-model.o			\
		${OBJDIR}/emul_msgdma.o				\
		${OBJDIR}/emul_pci.o				\
		${OBJDIR}/fwd_device.o				\
		${OBJDIR}/main.o				\
		${OBJDIR}/start.o				\
		${OBJDIR}/test.o				\
		${OSOBJDIR}/lib/libc/gen/assert.o		\
		${OSOBJDIR}/sys/dev/altera/fifo/fifo.o		\
		${OSOBJDIR}/sys/dev/altera/jtag_uart/jtag_uart.o\
		${OSOBJDIR}/sys/mips/beri/beripic.o		\
		${OSOBJDIR}/sys/mips/beri/beri_epw.o		\

KERNEL = malloc mips_cache
LIBRARIES = md5 libc

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
	${WARNFLAGS} -DWITHOUT_CAPSICUM=1 -DDM_BASE=${DM_BASE}

all:	_compile _link _binary

${LDSCRIPT}:
	sed s#__DM_BASE__#${DM_BASE}#g ${LDSCRIPT_TPL} > ${LDSCRIPT}

llvm-objdump:
	llvm-objdump-cheri -d ${APP}.elf | less

clean: _clean
	rm -f ${LDSCRIPT}

include ${CURDIR}/osfive/lib/md5/Makefile.inc
include ${CURDIR}/osfive/lib/libc/Makefile.inc
include ${CURDIR}/osfive/mk/gnu.mk
