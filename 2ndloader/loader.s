@@@ GBA multiboot cable second-level bootloader
@@@ (c) Matt Evans 15th May 2002

@@@ This code implements a very minimal boot 'monitor'.  Once downloaded
@@@ via the (slow) multiboot protocol, it configures the GBA serial port into
@@@ UART mode at 115.2Kbit/s, 8N1, and listens for commands from the front-end code
@@@ to download the REAL code quicker.
	
@@@ This code is part of the Matt'sSerialMultibootCable tarball, and must be
@@@ distributed alongside the rest of it.  This code is released under the GPL.
	

IWRAM		= 0x03000000
EWRAM		= 0x02000000
IO_base         = 0x04000000
VRAM_base       = 0x06000000
PAL_base        = 0x05000000

XFERBLOCK	= 512

SER_DATA	= 0x12A
SIOCNT		= 0x128
	
	.global _start
.text
	
_start:
start:	
        .ALIGN
    @ Start Vector

        b       rom_header_end

@@@ Header definition pilfered from Jeff Frohwein's crt0

    @ Nintendo Logo Character Data (8000004h)
        
	.byte 0x24, 0x0FF, 0x0AE, 0x51, 0x69, 0x9A
	.byte 0x0A2, 0x21, 0x3D, 0x84, 0x82, 0x0A, 0x84, 0x0E4, 0x9, 0x0AD, 0x11
	.byte 0x24, 0x8B, 0x98, 0x0C0, 0x81, 0x7F, 0x21, 0x0A3, 0x52, 0x0BE
	.byte 0x19, 0x93, 0x9, 0x0CE, 0x20, 0x10, 0x46, 0x4A,0x4A, 0x0F8
	.byte 0x27, 0x31, 0x0EC, 0x58, 0x0C7, 0x0E8, 0x33, 0x82, 0x0E3, 0x0CE
	.byte 0x0BF, 0x85, 0x0F4, 0x0DF, 0x94, 0x0CE, 0x4B, 0x9, 0x0C1, 0x94
	.byte 0x56, 0x8A, 0x0C0, 0x13, 0x72, 0x0A7, 0x0FC, 0x9F, 0x84, 0x4D
	.byte 0x73, 0x0A3, 0x0CA, 0x9A, 0x61, 0x58, 0x97, 0x0A3, 0x27, 0x0FC
	.byte 0x3, 0x98, 0x76, 0x23, 0x1D, 0x0C7, 0x61, 0x3, 0x4, 0x0AE, 0x56, 0x0BF
	.byte 0x38, 0x84, 0, 0x40, 0x0A7, 0x0E, 0x0FD, 0x0FF, 0x52, 0x0FE
	.byte 0x3, 0x6F, 0x95, 0x30, 0x0F1, 0x97, 0x0FB, 0x0C0, 0x85, 0x60, 0x0D6
	.byte 0x80, 0x25, 0x0A9, 0x63, 0x0BE, 0x3, 0x1, 0x4E, 0x38, 0x0E2, 0x0F9
	.byte 0x0A2, 0x34, 0x0FF, 0x0BB, 0x3E, 0x3, 0x44, 0x78, 0x0, 0x90, 0x0CB
	.byte 0x88, 0x11, 0x3A, 0x94, 0x65, 0x0C0, 0x7C, 0x63, 0x87, 0x0F0
	.byte 0x3C, 0x0AF, 0x0D6, 0x25, 0x0E4, 0x8B, 0x38, 0x0A, 0x0AC, 0x72
	.byte 0x21, 0x0D4, 0x0F8, 0x7

    @ Game Title (80000A0h)
        .byte   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .byte    0x00, 0x00, 0x00, 0x00

    @ Game Code (80000ACh)
        .byte   0x00,0x00,0x00,0x00

    @ Maker Code (80000B0h)
        .byte   0x30,0x31

    @ Fixed Value (80000B2h)
        .byte   0x96

    @ Main Unit Code (80000B3h)
        .byte   0x00

    @ Device Type (80000B4h)
        .byte   0x00

    @ Unused Data (7Byte) (80000B5h)
        .byte   0x00,0x00,0x00,0x00,0x00,0x00,0x00

    @ Software Version No (80000BCh)
        .byte   0x00

    @ Complement Check (80000BDh)
        .byte   0xf0

    @ Checksum (80000BEh)
        .byte   0x00,0x00

    .align
    
rom_header_end:
        b       me_start_vector    
	
boot_method:
        .byte   0       @ boot method (0=ROM boot, 3=Multiplay boot)
slave_number:
        .byte   0       @ slave # (1=slave#1, 2=slave#2, 3=slave#3)

        .byte   0       @ reserved
        .byte   0       @ reserved
        .word   0       @ reserved
        .word   0       @ reserved
        .word   0       @ reserved
        .word   0       @ reserved
        .word   0       @ reserved
        .word   0       @ reserved

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


@@@@@@@@@@@@@@@@@@@@@@
@        Reset       @
@@@@@@@@@@@@@@@@@@@@@@

	.align
me_start_vector:

	@ Set up stack, and try to show I'm alive:

	ldr	r13,=0x03007e00		@ ish.. maybe.

	@ Move 'actual_code' to 'end_of'... to IWRAM:

	adr	r0,actual_code
	adr	r1,end_of_code
	sub	r1,r1,r0	@ r1 = length

	mov	r2,#IWRAM

copy_loop:
	ldr	r3,[r0],#4
	str	r3,[r2],#4
	subs	r1,r1,#4
	bne	copy_loop

	mov	pc,#IWRAM
	


	@@@@@@@@@@@@@@@@@ This runs in IWRAM @@@@@@@@@@@@@@@@@@@@@@@@@@
actual_code:	
	
	mov     r10,#IO_base

uart_setup:	
	@@@@ Setup UART now: @@@@@

	mov	r0,#0

	add	r9,r10,#0x100
	strh	r0, [r9, #0x134 - 0x100]	@ rcnt:	serial comm. selected for port

	mov	r0,#0x3c00
	orr	r0,r0,#0x0083		@ 3c83 = UARTmode/noIRQ/rxtxEn/no fifo/no parity
					@ 8bits/no errors flags zero/no RTSCTS/115k2bps
	strh	r0, [r9, #SIOCNT - 0x100]

	mov	r0,#0x3d00
	orr	r0,r0,#0x0083		@ 3c83 = UARTmode/noIRQ/rxtxEn/no parity
					@ 8bits/no errors flags zero/no RTSCTS/115k2bps
					@ WITH FIFO
	strh	r0, [r9, #SIOCNT - 0x100]
	
		
	@ Well, now let's send some stuff around the place :)

	mov	r10,#EWRAM
	mov	r11,#0			@ Offset into ram (current 'address')
	
loader:
	mov	r0,#'>'
	bl	tx_byte
	
	bl	rx_byte

	@ Commands are as such:
	@ aDDDDDDDD - set current address 0000000 (def. 0) - returns > for ok
	@ s - send block, followed by N bytes. follwed by DDDDDDDD sum of bytes sent
	@	- also incs current address by 256
	@ r - resend last block. does address-256 and then s as above
	@ j - jump to EWRAM
	@ anything else = do >
	
	cmp	r0,#'a'
	beq	setaddress

	cmp	r0,#'s'
	beq	rxblock

	cmp	r0,#'r'
	beq	redoblock

	cmp	r0,#'j'
	beq	jumpstart
	
	b	loader

setaddress:	
	@ Get 4 bytes of current address
        bl      rx_byte
        mov     r11,r0
        bl      rx_byte
        orr     r11,r11,r0,lsl#8
        bl      rx_byte
        orr     r11,r11,r0,lsl#16
        bl      rx_byte
        orr     r11,r11,r0,lsl#24       @ r11 = address

	b	loader

rxblock:	
	mov	r8,#0			@ r8 = sum
dataload:
	mov	r9,#XFERBLOCK
	sub	r9,r9,#1
	@mov	r9,#255
blockload:
	bl	rx_byte
	strb	r0,[r10,r11]
	add	r8,r8,r0
	
	add	r11,r11,#1
	
	subs	r9,r9,#1
	bge	blockload

	@ Print sum of bytes then return to loader:

	mov	r0,r8
	bl	phex32
	
	b	loader


redoblock:
	sub	r11,r11,#XFERBLOCK
	b	rxblock

	
jumpstart:
	adr	r0,totalrx
	bl	pstring
	mov	r0,r11
	bl	phex32
	mov	r0,#'\n'
	bl	tx_byte
	adr	r0,sum
	bl	pstring
	mov	r0,r8
	bl	phex32
	mov	r0,#'\n'
	bl	tx_byte
	
	mov	PC,#EWRAM		@ branch to new code... wheee!
	
	
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@ Print 32bit hex number in r0
phex32:
	stmfd	r13!,{r1-r2,r14}
	
	mov	r0,r0,ror#24
	bl	phex8	@only prints bottom byte!

	mov	r0,r0,ror#24	@ same as ROL#8
	bl	phex8
	
	mov	r0,r0,ror#24
	bl	phex8

	mov	r0,r0,ror#24
	bl	phex8
	
	ldmfd	r13!,{r1-r2,pc}

	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@ Print bottom byte in r0 (saves r0)
phex8:
	stmfd	r13!,{r0-r2,r14}
	mov	r1,r0,ror#4
	and	r0,r1,#0x0f
	cmp	r0,#10
	addlt	r0,r0,#'0'
	addge	r0,r0,#'a'-10
	bl	tx_byte
	
	mov	r1,r1,ror#28	@ bottom nibble = orig now
	and	r0,r1,#0x0f
	cmp	r0,#10
	addlt	r0,r0,#'0'
	addge	r0,r0,#'a'-10
	bl	tx_byte
	ldmfd	r13!,{r0-r2,pc}
	
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@ Print string pointed to by r0:
pstring:
	stmfd	r13!,{r1-r2,r14}
	mov	r1,r0
pstringl:	
	ldrb	r0,[r1],#1
	cmp	r0,#0
	ldmeqfd	r13!,{r1-r2,pc}
	bl	tx_byte
	b	pstringl
	

	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@ Sends byte in r0 to serial, blocking till sent.
tx_byte:
	stmfd	r13!,{r1-r2}
	ldr	r2, =(IO_base + 0x100)
tx_wait:		
	ldrh	r1, [r2, #SIOCNT - 0x100]
	tst	r1, #0x0010		@ 1 = full (can't send)
	bne	tx_wait

	@ send data:
	strb	r0, [r2, #SER_DATA - 0x100]
	ldmfd	r13!,{r1-r2}
	mov	pc,r14

	
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@ Block till byte rx'd
rx_byte:
	stmfd	r13!,{r1}
	ldr	r1, =(IO_base + 0x100)
rx_wait:		
	ldrh	r0, [r1, #SIOCNT - 0x100]
	tst	r0, #0x0020		@ 0 = full (data there)
	bne	rx_wait

	@ send data:
	ldrb	r0, [r1, #SER_DATA - 0x100]
	ldmfd	r13!,{r1}
	mov	pc,r14

	
	
.pool
	.word 0xf337face
banner:	
	.ascii "\n[GBA 2nd stage loader (16/05/02 me)]*\0"
	.align
okload:
	.ascii "\nSend length\0"
	.align
totalrx:
	.ascii "Total bytes received: \0"
	.align
sum:	.ascii "Sum: \0"
	.align
	
	@ 'code' includes our literals here!
end_of_code:
	.word 0
.end
	