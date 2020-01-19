; rom_test_pia.s - Johan Smet - BSD-3-Clause (see LICENSE)

	.setcpu "6502"

	.segment "OS"

PORTA = $8000
CRA = $8001
PORTB = $8002
CRB = $8003

; program entry
vec_reset:
		; initialize stack pointer
		ldx #$ff
		txs

		; set DDRA / DDRB to all output
		lda #$ff
		sta PORTA
		sta PORTB

		; set CRA / CRB to enable write to the ports instead of ddr (no interrupts)
		lda #%00000100
		sta CRA
		sta CRB

		; enable lcd output
		lda #%00001100	; display on/off control: enable display - disable cursor + blinking
		jsr lcd_write_cmd

		; output message
		ldx #$00

@loop:	lda msg,x
		beq @end
		jsr lcd_write_data
		inx 
		jmp @loop

@end:	jmp @end

; LCD routines
lcd_write_data:
		sta PORTA		; write data to databus
		lda #%10100000	; rs = 1, rw = 0, enable = 1
		sta PORTB
		lda #%10000000	; rs = 1, rw = 0, enable = 0
		sta PORTB
		rts

lcd_write_cmd:
		sta PORTA		; write instruction to databus
		lda #%00100000	; rs = 0, rw = 0, enable = 1
		sta PORTB
		lda #%00000000	; rs = 0, rw = 0, enable = 0
		sta PORTB
		rts

; non-maskable interrupt handler
vec_nmi:
		jmp vec_nmi

; irq-handler
vec_irq:
		jmp vec_irq

; data
msg:	.asciiz "I CAN HAZ OUTPUT?"

; set vectors at the end of the ROM
	.segment "VECTORS"
		.word vec_nmi
		.word vec_reset
		.word vec_irq

