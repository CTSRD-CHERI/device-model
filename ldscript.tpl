/* TARGET(elf64-tradbigmips) */
ENTRY(_start)

SECTIONS
{
	. = __DM_BASE__;
	.start . : {
		*start.o(.text)
	}

	. = (__DM_BASE__ + 0x1000);
	.exception . : {
		*exception.o(.text)
	}

	.text : {
		*(.exception)
		*(.text)
	}

	.rodata : {
		*(.rodata)
	}

	.data : {
		*(.data)
	}

	.sdata : {
		_gp = .;
		*(.sdata)
	}

	.bss : {
		_sbss = ABSOLUTE(.);
		*(.bss COMMON)
		_ebss = ABSOLUTE(.);
	}
}
