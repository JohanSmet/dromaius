; kernel.s - Johan Smet - BSD-3-Clause (see LICENSE)
;
; basic OS functionality for the minimal_6502 device

.setcpu "6502"

KERNEL_IMPLEMENTATION = 1
.include "kernel.inc"
.include "constants.inc"

; =============================================================================
; imported symbols
; =============================================================================

.import main

; =============================================================================
; constants
; =============================================================================

LCD_ENABLE = %00100000
LCD_RW     = %01000000
LCD_RS     = %10000000

; =============================================================================
; zero page variables
; =============================================================================
.zeropage

k_strptr:		.byte $0000			; pointer to a string

k_key_matrix:	.res 4, $00			; copy of keyboard state
k_key_down:		.res 3, $00			; keys that are currently down
k_key_count:	.byte $00			; number of keys that are currently down
k_key_dprev:	.res 3, $00			; keys that were pressed in previous scan

; =============================================================================
; code
; =============================================================================

.segment "OS"

_k_init:
		; initialize stack pointer
		ldx #$ff
		txs

		; initialize zero-page variables (that need to be)
		ldx #$ff
		stx k_key_dprev+0
		stx k_key_dprev+1
		stx k_key_dprev+2

		; initialize port-A of the pia (connected to the LCD)
		; >> set DDRA to all output
		lda #$ff
		sta PIA_PORTA

		; >> set CRA to enable write to the port instead of ddr (no interrupts)
		lda #%00000100
		sta PIA_CRA

		; initialize port-B of the PIA (connected to the keypad)
		; >> set upper nibble of DDRB to output
		lda #%11110000
		sta PIA_PORTB
		;
		; >> set CRB to enable write to the port instead of ddr (no interrupts)
		lda #%00000100
		sta PIA_CRB

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

		; jump to program entry
		jmp main

; =============================================================================
; LDC functions
; =============================================================================

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

; =============================================================================
; keypad functions
; =============================================================================

k_keyb_scan:
			; init variables
			lda #$00
			sta k_key_count

			ldx #$ff
			stx k_key_down+0
			stx k_key_down+1
			stx k_key_down+2

			; read keyboard matrix in one pass to keep it as consistent as possible
			lda #%00010000		; start at first row

			sta PIA_PORTB		; select row
			ldy PIA_PORTB		; read columns
			sty k_key_matrix	; store in temp buffer
			.repeat 3, i
				rol				; select next row
				sta	PIA_PORTB
				ldy PIA_PORTB
				sty k_key_matrix+i+1
			.endrepeat

			; iterate over the matrix and check which keys are pressed
			.repeat 4, i
				lda k_key_matrix+i
				ldx #(i * 4)
				jsr _k_scan_keyrow
			.endrepeat

			; check for newly pressed keys
			ldx k_key_count
@check:		lda #$ff
			dex
			bmi @cleanup		; exit routine when counter goes negative
			lda k_key_down,x	; load keycode
			cmp k_key_dprev+0	; check if equal to previously pressed key
			beq	@check			; found -> skip to next key
			cmp k_key_dprev+1	; check if equal to previously pressed key
			beq	@check			; found -> skip to next key
			cmp k_key_dprev+2	; check if equal to previously pressed key
			beq	@check			; found -> skip to next key

			; move keys from this execution to _prev buffer
@cleanup:	ldx k_key_down+0
			stx k_key_dprev+0
			ldx k_key_down+1
			stx k_key_dprev+1
			ldx k_key_down+2
			stx k_key_dprev+2

			; end of routine
			rts

; _scan_keyrow: internal routine - scan one row of the key-matrix for keys that are pressed down
_k_scan_keyrow:
			lsr					; shift-right moves bottom bit into the carry flag
			bcc :+				; carry not set == key not pressed
			jsr _k_key_press
		:	inx					; point to the next entry in the keymap
			lsr					; shift next bit into carry flag
			bcc :+
			jsr _k_key_press
		:	inx					; point to the next entry in the keymap
			lsr					; shift next bit into carry flag
			bcc :+
			jsr _k_key_press
		:	inx					; point to the next entry in the keymap
			lsr					; shift next bit into carry flag
			bcc :+
			jsr _k_key_press
		:	rts

; key_press: internal routine
_k_key_press:
			pha					; save accumulator
			ldy k_key_count		; use Y-register as index into key-buffer
			inc k_key_count		; a key will be added
			lda k_key_map,x		; load key-code
			sta k_key_down,y	; store in keybuffer
			pla					; restore accumulator
			rts

; =============================================================================
; interrupts
; =============================================================================

; non-maskable interrupt handler
_k_vec_nmi:
		jmp _k_vec_nmi

; irq-handler
_k_vec_irq:
		jmp _k_vec_irq

; =============================================================================
; data
; =============================================================================
.rodata

k_key_map:	.byte '1', '2', '3', 'A'
			.byte '4', '5', '6', 'B'
			.byte '7', '8', '9', 'C'
			.byte '*', '0', '#', 'D'


; set vectors at the end of the ROM
	.segment "VECTORS"
		.word _k_vec_nmi
		.word _k_init
		.word _k_vec_irq
