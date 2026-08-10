;####################################################################################################
; BIOS-PROGRAM - Bouncing-Block-in-32bit-Mode
;####################################################################################################
	ORG	0x7C00
	BITS	16

;====================================================================================================
; Master Boot Record
;====================================================================================================
_MBR:
;****************************************************************************************************
; Real Mode Program
;****************************************************************************************************
_MBR_RM:
;----------------------------------------------------------------------------------------------------
; Main Routine (_MBR_RM_Main)
;----------------------------------------------------------------------------------------------------
_MBR_RM_Main:
	; Initialize Register and Stack
	CLI			; Disable Interrupts
	XOR	AX, AX		; AX = 0
	MOV	DS, AX		; Data Segment = 0
	MOV	ES, AX		; Extra Segment = 0
	MOV	SS, AX		; Stack Segment = 0
	MOV	SP, 0x7C00	; Set Stack Pointer just below MBR Start Address (0x7C00)
	
	; Save Boot Drive Number
	MOV	[_MBR_Data.bootDrive], DL
	STI			; Enable Interrupts
	
	; Set Video Mode (VGA Graphic Mode: 320x200, 256 colors)
	MOV	AH, 0x00	; Video Mode Setting
	MOV	AL, 0x13	; Mode Number
	INT	0x10		; BIOS Video Mode Interrupt
	
	; Load Kernel from Disk
	MOV	AH, 0x42			; Disk Extended Read
	MOV	DL, [_MBR_Data.bootDrive]	; Load Boot Drive Number
	MOV	SI, DAP				; Load Disk Address Packet
	INT	0x13				; BIOS Disk Service
	JC	.diskError			; If Carry Flag = 1, Jump to Error Process
	
	; Enable A20 Line
	IN	AL, 0x92	; Read System Control Port A
	OR	AL, 0x02	; Set A20 Enable Bit (bit 1)
	OUT	0x92, AL	; Write back to Port
	
	; Prepare Protect Mode
	CLI							; Disable Interrupts
	LGDT	[GDT.descriptor]	; Register GDT to CPU
	
	; Enable Protect Mode
	MOV	EAX, CR0	; Copy Control Register 0 to EAX
	OR	EAX, 0x00000001	; Set Protection Enable Bit (bit 0)
	MOV	CR0, EAX	; Write back to Control Register 0 (Transition to Protect Mode)
	
	; Jump to 32 bit Code Segment
	JMP	CODE_SEGMENT:_MBR_PM_Initialize	; Jump by Specifying Segment Selector and Offset
	
	; Fail Safe for Disk Error
.diskError:
	PUSH	0x000F
	PUSH	_MBR_Data.errorMessage
	CALL	_MBR_RM_PrintString
	ADD	SP, 4
.diskErrorLoop:
	HLT
	JMP	.diskErrorLoop
;----------------------------------------------------------------------------------------------------
; Sub Routine (_MBR_RM_PrintString)
; void PrintString(const char* string, uint16_t color)
;----------------------------------------------------------------------------------------------------
_MBR_RM_PrintString:
	; Save Registers and Set up Stack Frame
	PUSH	BP
	PUSH	BX
	PUSH	SI
	MOV	BP, SP	; Set Base Pointer to Current Stack Pointer
	
	; Retrieve Arguments from Stack
	; Stack Layout: [BP]=SI, [BP+2]=BX, [BP+4]=BP, [BP+6]=Return IP, [BP+8]=Argument 1 (String), [BP+10]=Argument 2 (Color)
	MOV	SI, [BP + 8]	; SI = Pointer to String
	MOV	BL, [BP + 10]	; BL = Text Color (Used by BIOS INT 0x10 in Graphic Mode)
	
.loop:
	MOV	AL, [SI]	; Load next Character into AL
	
	CMP	AL, 0x00	; Check if Character is NULL Terminator ('\0')
	JE	.end		; If NULL, Jump to .end
	
	; Print Character using BIOS Teletype Output
	MOV	AH, 0x0E	; BIOS Teletype Output Function
	MOV	BH, 0x00	; Page Number
	INT	0x10		; BIOS Video Interrupt (Prints AL with color BL)
	
	INC	SI		; Move to next Character
	JMP	.loop		; Repeat until NULL Terminator

.end:
	; Restore Registers and Return
	MOV	SP, BP		; Restore Stack Pointer
	POP	SI
	POP	BX
	POP	BP
	RET			; Return to Caller

	BITS	32
;****************************************************************************************************
; Protect Mode Program (32-bit)
;****************************************************************************************************
_MBR_PM:
;----------------------------------------------------------------------------------------------------
; Main Routine (_MBR_PM_Initialize)
;----------------------------------------------------------------------------------------------------
_MBR_PM_Initialize:
	; Set 32bit Segment Registers to Data Segment defined by GDT
	MOV	AX, DATA_SEGMENT	; Data Segment defined by GDT
	MOV	DS, AX			; Data Segment
	MOV	SS, AX			; Stack Segment
	MOV	ES, AX			; Extra Segment
	MOV	FS, AX
	MOV	GS, AX
	
	; Set up a large Stack for 32bit mode
	MOV	EBP, 0x90000	; 0x90000 is a Safe Free Memory Area sufficiently far from Kernel Area (0x10000)
	MOV	ESP, EBP
	
	; Jump to C Kernel
	MOV	EAX, 0x10000	; Loaded Memory Address (0x10000)
	CALL	EAX		; Call Kernel main() Function
	
	; Fail Safe (If Kernel returns)
.loop:
	HLT
	JMP	.loop

;****************************************************************************************************
; Data
;****************************************************************************************************
_MBR_Data:
.bootDrive:
	DB	0x00	; Boot Drive Number
.errorMessage:
	DB	"Disk Error", 0x0D, 0x0A
	DB	0x00
;----------------------------------------------------------------------------------------------------
; Disk Address Packet (DAP)
;----------------------------------------------------------------------------------------------------
DAP:
	DB	0x10	; Packet Size (16 Bytes = 0x10)
	DB	0	; Reserved
	DW	16	; Sectors to Load (16 Sectors = 8KB) *Increase this if Kernel Size grows
	DW	0x0000	; Destination Buffer Offset
	DW	0x1000	; Destination Buffer Segment (0x1000:0000 = Physical Address 0x10000)
	DQ	1	; Read Start LBA (LBA 1 = Sector next to MBR)
;----------------------------------------------------------------------------------------------------
; Global Descriptor Table (GDT)
;----------------------------------------------------------------------------------------------------
GDT:
.start:
	; Null Descriptor
	DD	0x00000000
	DD	0x00000000
.code:
	; Code Segment Descriptor (Base Address: 0x00000000, Limit: 4GB)
	DW	0xFFFF		; Limit [0:15]
	DW	0x0000		; Base Address [0:15]
	DB	0x00		; Base Address [16:23]
	DB	0b10011010	; Access Privilege (Present=1, Ring=00, System=1, Code/Data=1, Conforming=0, Readable=1, Accessed=0)
	DB	0b11001111	; Flag (Granularity=1, 32bit=1) & Limit [16:19]
	DB	0x00		; Base Address [24:31]
.data:
	; Data Segment Descriptor (Base Address: 0x00000000, Limit: 4GB)
	DW	0xFFFF		; Limit [0:15]
	DW	0x0000		; Base Address [0:15]
	DB	0x00		; Base Address [16:23]
	DB	0b10010010	; Access Privilege (Present=1, Ring=00, System=1, Code/Data=0, Expand-down=0, Writable=1, Accessed=0)
	DB	0b11001111	; Flag (Granularity=1, 32bit=1) & Limit [16:19]
	DB	0x00		; Base Address [24:31]
.end:
.descriptor:
	; GDT Pointer Information (Size & Start Address)
	DW	.end - .start - 1	; GDT Size - 1
	DD	.start			; GDT Start Address

; Segment Selector Constant (Byte Offset from GDT Head)
CODE_SEGMENT	EQU	GDT.code - GDT.start
DATA_SEGMENT	EQU	GDT.data - GDT.start
;****************************************************************************************************
; Boot Sector Padding
;****************************************************************************************************
	TIMES	0x01B8 - ($ - $$)	DB	0x00	; Padding with 0 up to 440 Bytes (0x01B8)
;****************************************************************************************************
; Partition Table (PT)
;****************************************************************************************************
_MBR_PT:
; Disk Serial Number
	DD	0x00000000
; Reserved
	DW	0x0000
; Partition 1
	DB	0x80		; Active Partition Flag (0x80 = Active)
	DB	0x20		; Start Head
	DB	0x21		; Start Sector Cylinder
	DB	0x00		; Start Cylinder
	DB	0x0E		; File System ID (0x0E = FAT16 LBA)
	DB	0x15		; End Head
	DB	0x50		; End Sector Cylinder
	DB	0x05		; End Cylinder
	DD	0x00000800	; First Sector (2048th Sector)
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
; MBR Boot Signature
	DB	0x55, 0xAA
	