//####################################################################################################
// BIOS-PROGRAM - Sample Program
//####################################################################################################
#ifndef ACPI_H
#define ACPI_H

//====================================================================================================
// Definition
//====================================================================================================
//****************************************************************************************************
// ACPI / AML (ACPI Machine Language) Definition
//****************************************************************************************************
#define AML_PACKAGE_OP		0x12
#define AML_BYTE_PREFIX		0x0A
#define AML_WORD_PREFIX		0x0B
#define ACPI_SLEEP_ENABLE	(1 << 13)

//****************************************************************************************************
// ACPI Structure
//****************************************************************************************************
//----------------------------------------------------------------------------------------------------
// RSDP (Root System Description Pointer)
//----------------------------------------------------------------------------------------------------
typedef struct {
	char signature[8];
	unsigned char checksum;
	char oem_id[6];
	unsigned char revision;
	unsigned int rsdt_address;
} __attribute__((packed)) ACPI_RSDP;
//----------------------------------------------------------------------------------------------------
// ACPI Table Header
//----------------------------------------------------------------------------------------------------
typedef struct {
	char signature[4];
	unsigned int length;
	unsigned char revision;
	unsigned char checksum;
	char oem_id[6];
	char oem_table_id[8];
	unsigned int oem_revision;
	unsigned int creator_id;
	unsigned int creator_revision;
} __attribute__((packed)) ACPI_HEADER;
//----------------------------------------------------------------------------------------------------
// RSDT (Root System Description Table)
//----------------------------------------------------------------------------------------------------
typedef struct {
	ACPI_HEADER header;
	unsigned int table_pointers[];
} __attribute__((packed)) ACPI_RSDT;
//----------------------------------------------------------------------------------------------------
// FADT (Fixed ACPI Description Table, Signature: "FACP")
//----------------------------------------------------------------------------------------------------
typedef struct {
	ACPI_HEADER header;
	unsigned int firmware_control;
	unsigned int dsdt;
	unsigned char reserved;
	unsigned char preferred_pm_profile;
	unsigned short sci_int;
	unsigned int smi_command;
	unsigned char acpi_enable;
	unsigned char acpi_disable;
	unsigned char s4bios_request;
	unsigned char pstate_control;
	unsigned int pm1a_event_block;
	unsigned int pm1b_event_block;
	unsigned int pm1a_control_block;
} __attribute__((packed)) ACPI_FADT;
//====================================================================================================
// Function
//====================================================================================================
//****************************************************************************************************
// ACPI System
//****************************************************************************************************
//----------------------------------------------------------------------------------------------------
// Shutdown
//----------------------------------------------------------------------------------------------------
void system_shutdown(void);

#endif
