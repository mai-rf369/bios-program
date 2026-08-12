ENTRY(_START)
SECTIONS
{
	.	=	0x10000;
	
	.TEXT : ALIGN(4096)
	{
		*(.TEXT)
	}
	
	.RODATA : ALIGN(4096)
	{
		*(.RODATA)
	}
	
	.DATA : ALIGN(4096)
	{
		*(.DATA)
	}
	
	.BSS : ALIGN(4096)
	{
		*(COMMON)
		*(.BSS)
	}
}
