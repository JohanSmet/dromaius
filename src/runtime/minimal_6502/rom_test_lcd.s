; rom_test_lcd.s - Johan Smet - BSD-3-Clause (see LICENSE)

	.setcpu "6502"

	.segment "OS"

PORTA = $8000
CRA = $8001

LMSG = $20
HMSG = $21

LCD_ENABLE = %00100000
LCD_RW     = %01000000
LCD_RS     = %10000000

; program entry
vec_reset:
		; initialize stack pointer
		ldx #$ff
		txs

		; set DDRA to all output
		lda #$ff
		sta PORTA

		; set CRA to enable write to the port instead of ddr (no interrupts)
		lda #%00000100
		sta CRA

		; lcd: switch to 4 bit interface mode
		lda #%00000010 | LCD_ENABLE
		sta PORTA
		eor #LCD_ENABLE
		sta PORTA

		; lcd: set two line mode
		lda #%00101000
		jsr lcd_write_cmd

		; enable lcd output
		lda #%00001111	; display on/off control: enable display - enable cursor - enable blinking
		jsr lcd_write_cmd

		; shift cursor right
		lda #%00010100
		jsr lcd_write_cmd
		;
		; shift cursor left
		lda #%00010000
		jsr lcd_write_cmd

		; output message
		ldx #<msg1
		ldy #>msg1
		lda #$00
		jsr lcd_write_string

		ldx #<msg2
		ldy #>msg2
		lda #$40
		jsr lcd_write_string

		; enable display shift
		lda #%00000111
		jsr lcd_write_cmd

		; shift display left
		ldx #10
@sl:
		lda #%00011000
		jsr lcd_write_cmd
		dex
		bne @sl

		; shift display right
		ldx #20
@sr:
		lda #%00011100
		jsr lcd_write_cmd
		dex
		bne @sr

		; return home
		lda #%00000010
		jsr lcd_write_cmd

@end:	jmp @end

; LCD routines
lcd_write_data:
		pha				; save accumulator
		; write upper nible
		lsr				; shift upper nibble to lower nibble
		lsr
		lsr
		lsr
		ora #(LCD_ENABLE | LCD_RS)
		sta PORTA
		eor #LCD_ENABLE
		sta PORTA
		; write lower nibble
		pla				; restore accumulator
		and #$0f		; blank out top nible
		ora #(LCD_ENABLE | LCD_RS)
		sta PORTA
		eor #LCD_ENABLE
		sta PORTA

		rts

lcd_write_cmd:
		pha				; save accumulator
		; write upper nible
		lsr				; shift upper nibble to lower nibble
		lsr
		lsr
		lsr
		ora #LCD_ENABLE
		sta PORTA
		eor #LCD_ENABLE
		sta PORTA
		; write lower nible
		pla				; restore accumulator
		and #$0f		; blank out top nible
		ora #LCD_ENABLE
		sta PORTA
		eor #LCD_ENABLE
		sta PORTA

		rts

lcd_write_string:
		stx LMSG
		sty HMSG
		ldy #$00

		ora #%10000000
		jsr lcd_write_cmd

@loop:	lda (LMSG),y
		beq @end
		jsr lcd_write_data
		iny
		jmp @loop

@end:	rts

; non-maskable interrupt handler
vec_nmi:
		jmp vec_nmi

; irq-handler
vec_irq:
		jmp vec_irq

; data
msg1:	.asciiz "I CAN HAZ OUTPUT <-->"
msg2:	.asciiz "YEZ YOU CAN!    "

; set vectors at the end of the ROM
	.segment "VECTORS"
		.word vec_nmi
		.word vec_reset
		.word vec_irq

