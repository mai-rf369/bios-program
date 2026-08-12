#include "acpi.h"
#include "hardware.h"
#include "timer.h"

static int acpi_strncmp(const char *str, const char *str2, int n)
{
	for (int i = 0; i < n; i++)
	{
		if (str1[i] != str2[i])
		{
			return 1;
		}
	}
	
	return 0;
}

static int acpi_verify_checksum(unsigned char *data, int length)
{
	unsigned char sum	=	0;
	
	for (int i = 0; i < length; i++)
	{
		sum	=	sum + data[i];
	}
	
	if (sum == 0)
	{
		return 1;
	}
	
	return 0;
}

void system_shutdown(void)
{
	ACPI_RSDP *rsdp	=	0;
	
	for (unsigned char *pointer = (unsigned char *)0x000E0000; pointer < (unsigned char *)0x00100000; pointer = pointer + 16)
	{
		if (acpi_strncmp((const char *)pointer, "RSD PTR ", 8) == 0)
		{
			if (acpi_verify_checksum(pointer, 20))
			{
				rsdp	=	(ACPI_RSDP *)pointer;
				break;
			}
		}
	}
	if (rsdp == 0)
	{
		return;
	}
	
	ACPI_RSDT *rsdt	=	(ACPI_RSDT *)rsdp->rsdt_address;
	int entries	=	(rsdt->header.length - sizeof(ACPI_HEADER)) / 4;
	
	ACPI_FADT *fadt	=	0;
	for (int i = 0; i < entries; i++)
	{
		ACPI_HEADER *header	=	(ACPI_HEADER *)rsdt->table_pointers[i];
		if (acpi_strncmp(header->signature, "FACP", 4) == 0)
		{
			fadt	=	(ACPI_FADT *)header;
			break;
		}
	}
	if (fadt == 0)
	{
		return;
	}
	
	if (fadt->smi_command != 0 && fadt->acpi_enable != 0)
	{
		outb(fadt->smi_command, fadt->acpi_enable);
		delay_ms(300);
	}
	
	unsigned short sleep_typa	=	0;
	ACPI_HEADER *dsdt	=	(ACPI_HEADER *)fadt->dsdt;
	char *dsdt_bytes	=	(char *)fadt->dsdt;
	
	for (unsigned int i = sizeof(ACPI_HEADER); i < dsdt->length - 4; i++)
	{
		if (acpi_strncmp(&dsdt_bytes[i], "_S5_", 4) == 0)
		{
			char *p	=	&dsdt_bytes[i] + 4;
			
			if (*p == AML_PACKAGE_OP)
			{
				p	=	p + 1;
				
				int package_length_bytes	=	(*p >> 6) + 1;
				p	=	p + package_length_bytes;
				
				p	=	p + 1;
				
				if (*p == AML_BYTE_PREFIX || *p == AML_WORD_PREFIX)
				{
					p	=	p + 1;
				}
				
				sleep_typa	=	(*p) << 10;
				break;
			}
		}
	}
	
	outw(fadt->pm1a_control_block, sleep_typa | ACPI_SLEEP_ENABLE);
	
	while (1)
	{
		__asm__ volatile ("cli; hlt");
	}
}
