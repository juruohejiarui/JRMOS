/***************************************************
*		版权声明
*
*	本操作系统名为：MINE
*	该操作系统未经授权不得以盈利或非盈利为目的进行开发，
*	只允许个人学习以及公开交流使用
*
*	代码最终所有权及解释权归田宇所有；
*
*	本模块作者：	田宇
*	EMail:		345538255@qq.com
*
*
***************************************************/

OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{

	. = 0xffff800000000000 + 0x100000;
	.mm_symbol_text :
	{
		mm_symbol_text = .;
		KEEP(*hal/x86_64/init/head.o(.text))
		KEEP(*hal/x86_64/cpu/apboot.o(.text))
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