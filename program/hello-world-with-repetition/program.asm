;####################################################################################################
; BIOS-PROGRAM - Hello-World-with-Repetition
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
;----------------------------------------------------------------------------------------------------
; Main Routine (_MBR_RM_Main)
;----------------------------------------------------------------------------------------------------
_MBR_RM_Main:
	CLI
	
	XOR	AX, AX
	
	MOV	DS, AX
	MOV	ES, AX
	MOV	SP, 0x7C00
	
	MOV	[_MBR_Data.driveNumber], DL
	
	STI
	
	MOV	AH, 0x03
	MOV	BH, 0x00
	INT	0x10
	
	MOV	AH, 0x00
	MOV	AL, 0x12
	INT	0x10
	
	MOV	CX, 0
	
.loop:
	CMP	CX, 0x0F
	JG	.end
	
	PUSH	0x000F
	PUSH	_MBR_Data.message
	CALL	_MBR_RM_PrintString
	ADD	SP, 4
	
	INC	CX
	JMP	.loop
	
.end:
	HLT
	JMP	.end

;----------------------------------------------------------------------------------------------------
; Sub Routine (_MBR_RM_PrintString)
;----------------------------------------------------------------------------------------------------
_MBR_RM_PrintString:
	PUSH	BP
	PUSH	BX
	PUSH	SI
	MOV	BP, SP
	
	MOV	SI, [BP + 8]
	MOV	BL, [BP + 10]
	
.loop:
	MOV	AL, [SI]
	
	CMP	AL, 0x00
	JE	.end
	
	MOV	AH, 0x0E
	MOV	BH, 0x00
	INT	0x10
	
	INC	SI
	JMP	.loop
	
.end:
	MOV	SP, BP
	POP	SI
	POP	BX
	POP	BP
	RET

;****************************************************************************************************
; Data
;****************************************************************************************************
_MBR_Data:
.driveNumber:
	DB	0x00
.message:
	DB	"Hello, World!", 0x0D, 0x0A
	DB	0x00

	TIMES	0x01B8 - ($ - $$)	DB	0x00				; Padding

; Disk Serial Number
	DD	0x00000000
; Reserved
	DW	0x0000
; Partition 1
	DB	0x80								; Active Partition Flag
	DB	0x20								; Start Head
	DB	0x21								; Start Sector Cylinder
	DB	0x00								; Start Cylinder
	DB	0x0E								; File System ID
	DB	0x15								; End Head
	DB	0x50								; End Sector Cylinder
	DB	0x05								; End Cylinder
	DD	0x00000800							; First Sector
	DD	0x003FF800							; Total Sectors
; Partition 2
	DB	0x00								; Active Partition Flag
	DB	0x00								; Start Head
	DB	0x00								; Start Sector Cylinder
	DB	0x00								; Start Cylinder
	DB	0x00								; File System ID
	DB	0x00								; End Head
	DB	0x00								; End Sector Cylinder
	DB	0x00								; End Cylinder
	DD	0x00000000							; First Sector
	DD	0x00000000							; Total Sectors
; Partition 3
	DB	0x00								; Active Partition Flag
	DB	0x00								; Start Head
	DB	0x00								; Start Sector Cylinder
	DB	0x00								; Start Cylinder
	DB	0x00								; File System ID
	DB	0x00								; End Head
	DB	0x00								; End Sector Cylinder
	DB	0x00								; End Cylinder
	DD	0x00000000							; First Sector
	DD	0x00000000							; Total Sectors
; Partition 4
	DB	0x00								; Active Partition Flag
	DB	0x00								; Start Head
	DB	0x00								; Start Sector Cylinder
	DB	0x00								; Start Cylinder
	DB	0x00								; File System ID
	DB	0x00								; End Head
	DB	0x00								; End Sector Cylinder
	DB	0x00								; End Cylinder
	DD	0x00000000							; First Sector
	DD	0x00000000							; Total Sectors
; Signature
	DB	0x55, 0xAA
