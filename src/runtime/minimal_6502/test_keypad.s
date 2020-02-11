; rom_test_keypadf.s - Johan Smet - BSD-3-Clause (see LICENSE)
;
	.include "kernel.inc"
	.include "constants.inc"

	.setcpu "6502"
	.export main

; =============================================================================
; zero page variables
; =============================================================================
.zeropage

key_matrix: .res 4, $00
key_down:	.res 3, $00
key_count:	.byte $00
key_dprev:	.res 3, $00

; =============================================================================
; code
; =============================================================================

.segment "OS"

; program entry
main:	
			jsr keyb_init

@loop:		jsr keyb_scan
			cmp #$ff
			beq @loop

			jsr k_lcd_write_data
			jmp @loop

@end:		jmp @end


keyb_init:
			; initialize zero-page variables 
			ldx #$ff
			stx key_dprev+0
			stx key_dprev+1
			stx key_dprev+2

			rts

keyb_scan:
			; NOTE: ports of PIA have been configured correctly by the kernel
		
			; init variables 
			lda #$00
			sta key_count

			ldx #$ff
			stx key_down+0
			stx key_down+1
			stx key_down+2

			; read keyboard matrix in one pass to keep it as consistent as possible
			lda #%00010000		; start at first row

			sta PIA_PORTB		; select row
			ldy PIA_PORTB		; read columns
			sty key_matrix		; store in temp buffer
			.repeat 3, i
				rol				; select next row 
				sta	PIA_PORTB	
				ldy PIA_PORTB
				sty key_matrix+i+1
			.endrepeat

			; iterate over the matrix and check which keys are pressed
			.repeat 4, i 
				lda key_matrix+i 
				ldx #(i * 4)
				jsr _scan_keyrow
			.endrepeat

			; check for newly pressed keys
			ldx key_count 
@check:		lda #$ff
			dex
			bmi @cleanup		; exit routine when counter goes negative
			lda key_down,x		; load keycode
			cmp key_dprev+0		; check if equal to previously pressed key 
			beq	@check			; found -> skip to next key
			cmp key_dprev+1		; check if equal to previously pressed key 
			beq	@check			; found -> skip to next key
			cmp key_dprev+2		; check if equal to previously pressed key 
			beq	@check			; found -> skip to next key

			; move keys from this execution to _prev buffer
@cleanup:	ldx key_down+0
			stx key_dprev+0
			ldx key_down+1
			stx key_dprev+1
			ldx key_down+2
			stx key_dprev+2

			; end of routine
			rts

; _scan_keyrow: internal routine - scan one row of the key-matrix for keys that are pressed down
_scan_keyrow:
			lsr				; shift-right moves bottom bit into the carry flag
			bcc :+			; carry not set == key not pressed 
			jsr _key_press
		:	inx				; point to the next entry in the keymap 
			lsr				; shift next bit into carry flag
			bcc :+
			jsr _key_press
		:	inx				; point to the next entry in the keymap 
			lsr				; shift next bit into carry flag
			bcc :+
			jsr _key_press
		:	inx				; point to the next entry in the keymap 
			lsr				; shift next bit into carry flag
			bcc :+
			jsr _key_press
		:	rts

; key_press: internal routine
_key_press:
			pha				; save accumulator
			ldy key_count	; use Y-register as index into key-buffer
			inc key_count	; a key will be added
			lda key_map,x	; load key-code
			sta key_down,y	; store in keybuffer
			pla				; restore accumulator
			rts

; data
.rodata

key_map:	.byte '1', '2', '3', 'A'
			.byte '4', '5', '6', 'B'
			.byte '7', '8', '9', 'C'
			.byte '*', '0', '#', 'D'

