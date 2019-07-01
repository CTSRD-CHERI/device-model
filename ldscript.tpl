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

	. = (__DM_BASE__ + 0x2000);
	.tlbmiss . : {
		*tlbmiss.o(.text)
	}

	.text : {
		*(.exception)
		*(.text)
	}

	.rodata : {
		*(.rodata)
	}

	/* Ensure _smem is associated with the next section */
	. = .;
	_smem = ABSOLUTE(.);
	.data : {
		_sdata = ABSOLUTE(.);
		*(.data)
		_edata = ABSOLUTE(.);
	}

	.bss : {
		_sbss = ABSOLUTE(.);
		*(.bss COMMON)
		_ebss = ABSOLUTE(.);
	}
}
