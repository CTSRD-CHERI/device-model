APP =		device-model
ARCH =		mips

CC =		clang-cheri
LD =		ld.lld-cheri
OBJCOPY =	llvm-objcopy-cheri

LDSCRIPT =	${.CURDIR}/ldscript

OBJECTS =	alloc.o						\
		bhyve/mem.o					\
		bhyve/block_if.o				\
		bhyve/pci_emul.o				\
		bhyve/pci_ahci.o				\
		bhyve/pci_e82545.o				\
		bhyve_support.o					\
		device-model.o					\
		emul_msgdma.o					\
		emul_pci.o					\
		fwd_device.o					\
		main.o						\
		osfive/lib/md5/md5.o				\
		osfive/sys/dev/altera/fifo/fifo.o		\
		osfive/sys/dev/altera/jtag_uart/jtag_uart.o	\
		osfive/sys/mips/beri/beripic.o			\
		osfive/sys/mips/beri/beri_epw.o			\
		osfive/sys/mips/mips/machdep.o			\
		osfive/sys/mips/mips/cache_mipsNN.o		\
		osfive/sys/mips/mips/timer.o			\
		osfive/sys/mips/mips/trap.o			\
		osfive/sys/mips/mips/exception.o		\
		osfive/sys/kern/kern_malloc_fl.o		\
		osfive/sys/kern/kern_panic.o			\
		osfive/sys/kern/subr_prf.o			\
		osfive/sys/kern/subr_console.o			\
		start.o

.include "${.CURDIR}/osfive/lib/libc/Makefile.inc"

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

CFLAGS = -march=mips64 -mcpu=mips4	\
	-G0 -O -g -nostdinc -mno-abicalls -msoft-float	\
	-fwrapv -fno-builtin-printf ${WARNFLAGS} -DWITHOUT_CAPSICUM=1

all:	compile link binary

llvm-objdump:
	llvm-objdump-cheri -d ${APP}.elf | less

.include "${.CURDIR}/osfive/mk/user.mk"
.include "${.CURDIR}/osfive/mk/compile.mk"
.include "${.CURDIR}/osfive/mk/link.mk"
.include "${.CURDIR}/osfive/mk/binutils.mk"
.include "${.CURDIR}/osfive/mk/clean.mk"
.include "${.CURDIR}/osfive/mk/info.mk"
