APP =		device-model
ARCH =		mips

CC =		clang-cheri
LD =		ld.lld-cheri
OBJCOPY =	llvm-objcopy-cheri

LDSCRIPT =	${.CURDIR}/ldscript

OBJECTS =	device-model.o					\
		emul_msgdma.o					\
		osfive/sys/dev/altera/jtag_uart/jtag_uart.o	\
		osfive/sys/mips/beri/beripic.o			\
		osfive/sys/mips/beri/beri_epw.o			\
		osfive/sys/mips/mips/timer.o			\
		osfive/sys/mips/mips/trap.o			\
		osfive/sys/mips/mips/exception.o		\
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

CFLAGS = -target mips64 -integrated-as -march=mips64 		\
	-O -g -nostdinc -mno-abicalls -msoft-float		\
	-fwrapv -fno-builtin-printf ${WARNFLAGS}

all:	compile link binary

.include "${.CURDIR}/osfive/mk/user.mk"
.include "${.CURDIR}/osfive/mk/compile.mk"
.include "${.CURDIR}/osfive/mk/link.mk"
.include "${.CURDIR}/osfive/mk/binutils.mk"
.include "${.CURDIR}/osfive/mk/clean.mk"
