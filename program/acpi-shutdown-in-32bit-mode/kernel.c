//####################################################################################################
// BIOS-PROGRAM - ACPI-Shutdown-in-32bit-Mode (Kernel)
//####################################################################################################
#include "hardware.h"
#include "font8x8.h"
#include "timer.h"
#include "graphic.h"
#include "keycode.h"

//====================================================================================================
// Declare Function
//====================================================================================================
void system_shutdown(void);
void delay_ms(unsigned int ms);
void draw_char(int x, int y, char c, unsigned char color, unsigned char *vram);
void draw_string(int x, int y, const char *str, unsigned char color, unsigned char *vram);

//====================================================================================================
// Function
//====================================================================================================
//****************************************************************************************************
// Keyboard Driver
//****************************************************************************************************
//----------------------------------------------------------------------------------------------------
// Sub Routine (process_keyboard)
//----------------------------------------------------------------------------------------------------
void process_keyboard(int *key_state)
{
	while (inb(0x64) & 0x01)
	{
		unsigned char scan	=	inb(0x60);
		if (scan == 0xE0)
		{
			continue;
		}
		
		if (scan & 0x80)
		{
			key_state[scan & 0x7F]	=	0;	// Release
		}
		else
		{
			key_state[scan]		=	1;	// Press
		}
	}
}

//****************************************************************************************************
// Main
//****************************************************************************************************
//----------------------------------------------------------------------------------------------------
// Main Routine (main)
//----------------------------------------------------------------------------------------------------
void main(void)
{
	unsigned char *vram	=	(unsigned char *)VRAM;
	
	// Clear Display
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
	{
		vram[i]	=	0;
	}
	
	// Create Key State
	int key_state[128];
	for (int i = 0; i < 128; i++)
	{
		key_state[i]	=	0;
	}
	
	// Game Loop
	while (1)
	{
		process_keyboard(key_state);
		
		if (key_state[KEY_ESC])
		{
			system_shutdown();
		}
		
		draw_string(4, 4, "ESC: Shutdown", 15, &font8x8, vram);
		
		delay_ms(16);
	}
}

//****************************************************************************************************
// Hardware Driver
//****************************************************************************************************
//----------------------------------------------------------------------------------------------------
// Sub Routine (acpi_strncmp)
// Helper to compare strings without standard library
//----------------------------------------------------------------------------------------------------
static int acpi_strncmp(const char *str1, const char *str2, int n)
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
//----------------------------------------------------------------------------------------------------
// Sub Routine (acpi_verify_checksum)
// Calculate Checksum and Verify whether it equals Zero
//----------------------------------------------------------------------------------------------------
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


//----------------------------------------------------------------------------------------------------
// Sub Routine (system_shutdown)
// Shutdown PC using ACPI (Advanced Configuration and Power Interface)
//----------------------------------------------------------------------------------------------------
void system_shutdown(void)
{
	unsigned int *pointer;
	unsigned int rsdp_address = 0;
	
	// Search Root System Description Pointer
	// Scan from 0x000E0000 to 0x000FFFFF per 16Byte
	for (pointer = (unsigned int *)0x000E0000; (unsigned int)pointer < 0x00100000; pointer = pointer + 4)
	{
		// Search RSD PTR
		if (pointer[0] == 0x20445352 && pointer[1] == 0x20525450)
		{
			if (acpi_verify_checksum((unsigned char *)pointer, 20))
			{
				rsdp_address	=	(unsigned int)pointer;
				break;
			}
		}
	}
	
	if (rsdp_address == 0)
	{
		return;
	}
	
	// Get RSDT (Root System Description Table)
	unsigned int rsdt_address	=	*((unsigned int *)(rsdp_address + 16));
	
	// Search FACP (Pixed ACPI Description Table) in RSDT
	unsigned int facp_address	=	0;
	unsigned int rsdt_length	=	*((unsigned int *)(rsdt_address + 4));
	int entries			=	(rsdt_length - 36) / 4;
	unsigned int *entry_pointers	=	(unsigned int *)(rsdt_address + 36);
	
	for (int i = 0; i < entries; i++)
	{
		unsigned int table_address	=	entry_pointers[i];
		if (*((unsigned int *)table_address) == 0x50434146)
		{
			facp_address	=	table_address;
			break;
		}
	}
	
	if (facp_address == 0)
	{
		return;
	}
	
	// Get Address and Port from FACP
	unsigned int dsdt_address	=	*((unsigned int *)(facp_address + 40));
	unsigned int smi_cmd_port	=	*((unsigned int *)(facp_address + 48));
	unsigned char acpi_enable	=	*((unsigned char *)(facp_address + 52));
	unsigned int pm1a_cnt_block	=	*((unsigned int *)(facp_address + 64));
	
	// Enable ACPI Mode
	// Send Enable Command to SMI Port
	if (smi_cmd_port != 0 && acpi_enable != 0)
	{
		outb(smi_cmd_port, acpi_enable);
		delay_ms(300);
	}
	
	// Search _S5_ Shutdown Object from DSDT (Differentiated System Description Table)
	unsigned short slp_typa	=	0;
	unsigned int dsdt_length	=	*((unsigned int *)(dsdt_address + 4));
	char *dsdt_bytes	=	(char *)dsdt_address;
	
	for (unsigned int i = 36; i < dsdt_length - 4; i++)
	{
		if (acpi_strncmp(&dsdt_bytes[i], "_S5_", 4) == 0)
		{
			char *p	=	&dsdt_bytes[i] + 4;
			if (*p == 0x12)	// Package Start
			{
				p++;
				
				// Calculate Package Byte Length & Skip
				int package_length_bytes	=	(*p >> 6) + 1;
				p	=	p + package_length_bytes;
				p++;
				
				// Get SLP_TYPa
				if (*p == 0x0A)
				{
					p++;
				}
				else if (*p == 0x0B)
				{
					p++;
				}
				
				slp_typa	=	(*p) << 10;
				break;
			}
		}
	}
	
	// Execute Shutdown
	// Set SLP_TYPa to SLP_EN (0x2000 bit 13) & Send to PM1a
	outw(pm1a_cnt_block, slp_typa | 0x2000);
	
	// Fail Safe
	while (1)
	{
		__asm__ volatile ("cli; hlt");
	}
}
