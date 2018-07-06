APP =		device-model
ARCH =		mips

CC =		${CROSS_COMPILE}gcc
LD =		${CROSS_COMPILE}ld
OBJCOPY =	${CROSS_COMPILE}objcopy

# Not ready
# CC =         clang60
# LD =         ld.lld60
# OBJCOPY =    llvm-objcopy60

LDSCRIPT =	${.CURDIR}/ldscript

OBJECTS =	device-model.o					\
		osfive/sys/dev/altera/jtag_uart/jtag_uart.o	\
		osfive/sys/mips/beri/beripic.o			\
		osfive/sys/mips/mips/timer.o			\
		osfive/sys/mips/mips/trap.o			\
		osfive/sys/mips/mips/exception.o		\
		osfive/sys/kern/subr_prf.o			\
		osfive/sys/kern/subr_console.o			\
		start.o

.include "osfive/lib/libc/Makefile.inc"

# Not ready
# CFLAGS = -target mips64 -integrated-as

CFLAGS =	-O -pipe -g -nostdinc -fno-pic -mno-abicalls -G0	\
	-march=mips64 -mabi=64 -msoft-float -ffreestanding -fwrapv	\
	-gdwarf-2 -fno-common -fms-extensions -finline-limit=8000	\
	-std=iso9899:1999 -fno-builtin-printf				\
	-Wall -Wredundant-decls -Wnested-externs			\
	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith	\
	-Winline -Wcast-qual -Wundef -Wno-pointer-sign			\
	-fformat-extensions -Wmissing-include-dirs			\
	-fdiagnostics-show-option -Wno-unknown-pragmas			\
	-Wno-uninitialized -Werror

all:	compile link binary

.include "${.CURDIR}/osfive/mk/user.mk"
.include "${.CURDIR}/osfive/mk/compile.mk"
.include "${.CURDIR}/osfive/mk/link.mk"
.include "${.CURDIR}/osfive/mk/binutils.mk"
.include "${.CURDIR}/osfive/mk/clean.mk"
