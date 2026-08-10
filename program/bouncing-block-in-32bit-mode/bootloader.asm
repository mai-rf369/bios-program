;####################################################################################################
; BIOS-PROGRAM - Bouncing-Block-in-32bit-Mode
;####################################################################################################
	ORG	0x7C00
	BITS	16

;====================================================================================================
; Master Boot Record
;====================================================================================================
_MBR:
; Bootstrap Code
;****************************************************************************************************
; Real Mode Program
;****************************************************************************************************
_MBR_RM:
;----------------------------------------------------------------------------------------------------
; Main Routine (_MBR_RM_Main)
;----------------------------------------------------------------------------------------------------
_MBR_RM_Main:
	; Initialize Register
	CLI
	XOR	AX, AX
	MOV	DS, AX
	MOV	ES, AX
	MOV	SS, AX
	MOV	SP, 0x7C00
	
	; Save Boot Drive
	MOV	[_MBR_Data.bootDrive], DL
	STI
	
	; Set Video Mode (VGA Graphic Mode: 320x200, 256 colors)
	MOV	AH, 0x00
	MOV	AL, 0x13
	INT	0x10
	
	; Load Kernel
	MOV	AH, 0x42
	MOV	DL, [_MBR_Data.bootDrive]
	MOV	SI, DAP
	INT	0x13
	JC	_MBR_RM_Main.diskError
	
	; Enable A20 Line
	IN	AL, 0x92
	OR	AL, 2
	OUT	0x92, AL
	
	; Load GDT
	CLI
	LGDT	[_MBR_Data.globalDescriptorTable_descriptor]
	
	MOV	EAX, CR0
	OR	EAX, 0x0001	; Enable Protect Mode Flag
	MOV	CR0, EAX
	
	; Flush Pipeline & Jump to 32bit Code
	JMP	CODE_SEGMENT:_MBR_PM_Initialize
	
.diskError:
	HLT
	JMP	_MBR_RM_Main.diskError
;****************************************************************************************************
; Real Mode Program
;****************************************************************************************************
_MBR_PM:
;----------------------------------------------------------------------------------------------------
; Main Routine (_MBR_PM_Initialize)
;----------------------------------------------------------------------------------------------------
_MBR_PM_Initialize:
	; Initialize 32bit Segment Register
	MOV	AX, DATA_SEGMENT
	MOV	DS, AX
	MOV	SS, AX
	MOV	ES, AX
	MOV	FS, AX
	MOV	GS, AX
	
	; Set Stack
	MOV	EBP, 0x90000
	MOV	ESP, EBP
	
	; Jump to C Kernel
	; Call Loaded Memory (0x10000)
	MOV	EAX, 0x10000
	CALL	EAX
	
.loop:
	HLT
	JMP	.loop
	
;----------------------------------------------------------------------------------------------------
; Error Routine (_MBR_RM_DiskError)
;----------------------------------------------------------------------------------------------------
_MBR_RM_DiskError:
	
	
	
;****************************************************************************************************
; Data
;****************************************************************************************************
_MBR_Data:
.bootDrive:
	DB	0x00
.diskAddressPacket:
	DB	0x10	; Packet Size (16 Bytes)
	DB	0
	DW	16	; Sectors to Load (16 Sectors: 8KB)
	DW	0x0000	; Destination Buffer Offset
	DW	0x1000	; Destination Buffer Segment (0x1000:0000 = 0x10000)
	DQ	1	; Read Start LBA (LBA 1 = Next to MBR)
.globalDescriptorTable:
.globalDescriptorTable_start:
	DD	0x0, 0x0	; Null Descriptor
.globalDescriptorTable_code:
	DW	0xFFFF		; Code Segment (0 - 4GB)
	DW	0x0000
	DB	0x00
	DB	0b10011010
	DB	0b11001111
	DB	0x00
.globalDescriptorTable_data:
	DW	0xFFFF		; Data Segment (0 - 4GB)
	DW	0x0000
	DB	0x00
	DB	0b10011010
	DB	0b11001111
	DB	0x00
.globalDescriptorTable_end:

.globalDescriptorTable_descriptor:
	DW	.globalDescriptorTable_end - .globalDescriptorTable_start - 1	; Global Descriptor Table Size
	DD	.globalDescriptorTable_start					; Global Descriptor Table Start Address

CODE_SEGMENT	EQU	.globalDescriptorTable_code - .globalDescriptorTable_start
DATA_SEGMENT	EQU	.globalDescriptorTable_data - .globalDescriptorTable_start

	TIMES	0x01B8 - ($ - $$)	DB	0x00	; Padding
; Disk Serial Number
	DD	0x00000000
; Reserved
	DW	0x0000
; Partition 1
	DB	0x80		; Active Partition Flag
	DB	0x20		; Start Head
	DB	0x21		; Start Sector Cylinder
	DB	0x00		; Start Cylinder
	DB	0x0E		; File System ID
	DB	0x15		; End Head
	DB	0x50		; End Sector Cylinder
	DB	0x05		; End Cylinder
	DD	0x00000800	; First Sector
	DD	0x003FF800	; Total Sectors
; Partition 2
	DB	0x00		; Active Partition Flag
	DB	0x00		; Start Head
	DB	0x00		; Start Sector Cylinder
	DB	0x00		; Start Cylinder
	DB	0x00		; File System ID
	DB	0x00		; End Head
	DB	0x00		; End Sector Cylinder
	DB	0x00		; End Cylinder
	DD	0x00000000	; First Sector
	DD	0x00000000	; Total Sectors
; Partition 3
	DB	0x00		; Active Partition Flag
	DB	0x00		; Start Head
	DB	0x00		; Start Sector Cylinder
	DB	0x00		; Start Cylinder
	DB	0x00		; File System ID
	DB	0x00		; End Head
	DB	0x00		; End Sector Cylinder
	DB	0x00		; End Cylinder
	DD	0x00000000	; First Sector
	DD	0x00000000	; Total Sectors
; Partition 4
	DB	0x00		; Active Partition Flag
	DB	0x00		; Start Head
	DB	0x00		; Start Sector Cylinder
	DB	0x00		; Start Cylinder
	DB	0x00		; File System ID
	DB	0x00		; End Head
	DB	0x00		; End Sector Cylinder
	DB	0x00		; End Cylinder
	DD	0x00000000	; First Sector
	DD	0x00000000	; Total Sectors
; Signature
	DB	0x55, 0xAA
	