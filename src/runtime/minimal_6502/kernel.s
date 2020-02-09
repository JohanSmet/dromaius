; kernel.s - Johan Smet - BSD-3-Clause (see LICENSE)
;
; basic OS functionality for the minimal_6502 device

.setcpu "6502"

; =============================================================================
; exported symbols
; =============================================================================

.export k_lcd_write_cmd
.export k_lcd_write_data
.export k_lcd_clear
.export k_lcd_show_string

; =============================================================================
; imported symbols
; =============================================================================

.import main

; =============================================================================
; constants
; =============================================================================

PIA_PORTA  = $8000
PIA_CRA    = $8001

LCD_ENABLE = %00100000
LCD_RW     = %01000000
LCD_RS     = %10000000

; =============================================================================
; zero page variables
; =============================================================================
.zeropage

k_strptr:	.byte $0000

; =============================================================================
; code
; =============================================================================

.segment "OS"

_k_init:
		; initialize stack pointer
		ldx #$ff 
		txs

		; initialize port-A of the pia (connected to the LCD)
		; >> set DDRA to all output
		lda #$ff
		sta PIA_PORTA

		; >> set CRA to enable write to the port instead of ddr (no interrupts)
		lda #%00000100
		sta PIA_CRA

		; initialize LCD. We don't know what state the LCD is in. It could be in 8-bit mode or
		; in 4-bit mode (either expecting the upper or lower nibble).
		;  -> do a sequence that is guaranteed to switch it to 8-bit mode (see datasheet)
		;  -> then switch to 4-bit mode

		; >> switch to 8-bit mode (lower nibble of lda == sent to the lcd)
		lda #%00000011
		jsr _k_lcd_nibble
		; TODO: wait for more than 4ms

		lda #%00000011
		jsr _k_lcd_nibble
		; TODO: wait for more than 100 µs

		lda #%00000011
		jsr _k_lcd_nibble
		; TODO: wait for more than 100 µs

		; >> lcd: switch to 4 bit interface mode
		lda #%00000010
		jsr _k_lcd_nibble

		; >> lcd: clear display
		lda #%00000001
		jsr k_lcd_write_cmd

		; >> lcd: turn on
		lda #%00001110	; display on/off control: enable display - enable cursor - disable blinking
		jsr k_lcd_write_cmd

		jmp main

k_lcd_write_cmd:
		pha				; save accumulator

		; write upper nible
		lsr				; shift upper nibble to lower nibble
		lsr
		lsr
		lsr
		jsr _k_lcd_nibble

		; write lower nible
		pla				; restore accumulator
		and #$0f		; blank out top nible
		jsr _k_lcd_nibble

		rts

k_lcd_write_data:
		pha				; save accumulator

		; write upper nible
		lsr				; shift upper nibble to lower nibble
		lsr
		lsr
		lsr
		ora #LCD_RS		; set LCD data-register
		jsr _k_lcd_nibble

		; write lower nibble
		pla				; restore accumulator
		and #$0f		; blank out top nible
		ora #LCD_RS		; set LCD data-register
		jsr _k_lcd_nibble

		rts

k_lcd_clear:
		lda #%00000001
		jsr k_lcd_write_cmd
		rts

k_lcd_show_string:
		stx k_strptr
		sty k_strptr+1
		ldy #$00

		; send position command
		ora #%10000000
		jsr k_lcd_write_cmd

@loop:	lda (k_strptr),y
		beq @end
		jsr k_lcd_write_data
		iny
		jmp @loop

@end:	rts


_k_lcd_nibble:
		ora #LCD_ENABLE
		sta PIA_PORTA
		eor #LCD_ENABLE
		sta PIA_PORTA

		rts

; non-maskable interrupt handler
_k_vec_nmi:
		jmp _k_vec_nmi

; irq-handler 
_k_vec_irq:
		jmp _k_vec_irq


; set _k_vectors at the end of the ROM
	.segment "VECTORS"
		.word _k_vec_nmi
		.word _k_init
		.word _k_vec_irq
