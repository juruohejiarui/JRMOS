ENTRY(_start)
SECTIONS
{

	. = 0xffff800000000000 + 0x100000;
	.mm_symbol_text :
	{
		mm_symbol_text = .;
		KEEP(*hal/aarch64/init/head.o(.text))
		*(.text)
		*(.text)
		mm_symbol_etext = .;
	}
	. = ALIGN(8);
	.data :
	{
		mm_symbol_data = .;
		*(.data)
		
		mm_symbol_edata = .;
	}
	.rodata : {
		mm_symbol_rodata = .;
		*(.rodata)
		mm_symbol_erodata = .;
	}
	. = ALIGN(32768);
	.data.hal_init_stk : { *(.data.hal_init_stk) }
	.bss :
	{
		mm_symbol_bss = .;
		*(.bss)
		mm_symbol_ebss = .;
	}

	mm_symbol_end = .;
}