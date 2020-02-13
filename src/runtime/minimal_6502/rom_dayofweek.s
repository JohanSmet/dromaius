; Originally from copied from http://www.6502.org/source/misc/dow.htm
; (modified to compile with ca65)
;
; How to compute the day of the week in 6502 assembly.
; By Paul Guertin (pg@sff.net), 18 August 2000.

; This routine works for any date from 1900-03-01 to 2155-12-31.
; No range checking is done, so validate input before calling.
;
; I use the formula
;     Weekday = (day + offset[month] + year + year/4 + fudge) mod 7
; where the value of fudge depends on the century.
;
; Input: Y = year (0=1900, 1=1901, ..., 255=2155)
;        X = month (1=Jan, 2=Feb, ..., 12=Dec)
;        A = day (1 to 31)
;
; Output: Weekday in A (0=Sunday, 1=Monday, ..., 6=Saturday)

	.include "kernel.inc"
	.setcpu "6502"
	.export main

.zeropage

TMP:	.byte $00
year:	.byte $00
month:	.byte $00
day:	.byte $00
result: .byte $00

.segment "OS"

; program entry
main:
		lda #2
		jsr k_lcd_set_lines

@b:		jsr k_lcd_clear

		; input year
		ldx #<YEAR_MSG
		ldy #>YEAR_MSG
		lda #$00
		jsr k_lcd_show_string

		lda #40
		jsr k_lcd_position

		jsr input_digit
		jsr mult10
		sta year
		jsr input_digit
		clc
		adc #100
		clc
		adc year
		sta year

		jsr delay

		; input month
		jsr k_lcd_clear
		ldx #<MONTH_MSG
		ldy #>MONTH_MSG
		lda #$00
		jsr k_lcd_show_string

		lda #40
		jsr k_lcd_position

		jsr input_digit
		jsr mult10
		sta month
		jsr input_digit
		clc
		adc month
		sta month

		jsr delay

		; input day
		jsr k_lcd_clear
		ldx #<DAY_MSG
		ldy #>DAY_MSG
		lda #$00
		jsr k_lcd_show_string

		lda #40
		jsr k_lcd_position

		jsr input_digit
		jsr mult10
		sta day
		jsr input_digit
		clc
		adc day
		sta day

		jsr delay

		; load params
		ldy year
		ldx month
		lda day

		; call day of week routine - output in register-A
		jsr	dow
		sta result

		; display result
		jsr k_lcd_clear

		lda result
		asl				; * 2
		asl				; * 4
		asl				; * 8
		asl				; * 16
		clc
		adc #<DAYS
		tax
		lda #$00
		adc #>DAYS
		tay
		lda #$00
		jsr k_lcd_show_string

		ldx #<END_MSG
		ldy #>END_MSG
		lda #$40
		jsr k_lcd_show_string

		jsr wait_for_key

		jmp @b

; day of week routine
dow:
         CPX #3          ; Year starts in March to bypass
         BCS @MARCH      ; leap year problem
         DEY             ; If Jan or Feb, decrement year
@MARCH:  EOR #$7F        ; Invert A so carry works right
         CPY #200        ; Carry will be 1 if 22nd century
         ADC MTAB-1,X    ; A is now day+month offset
         STA TMP
         TYA             ; Get the year
         JSR @MOD7       ; Do a modulo to prevent overflow
         SBC TMP         ; Combine with day+month
         STA TMP
         TYA             ; Get the year again
         LSR             ; Divide it by 4
         LSR
         CLC             ; Add it to y+m+d and fall through
         ADC TMP
@MOD7:   ADC #7          ; Returns (A+3) modulo 7
         BCC @MOD7       ; for A in 0..255
         RTS

; input numeric character
input_digit:
		; scan keyboard until a key is pressed
		jsr k_keyb_scan
		cmp #$ff
		beq input_digit

		; check for invalid keys
		cmp #'0'			; check with lower bound
		bcc input_digit		; branch if less

		cmp #':'			; check with upper bound
		bcs input_digit		; branch if greater than or equal

		; display digit
		pha
		jsr k_lcd_write_data
		pla

		; convert to decimal number
		sec
		sbc #'0'

		rts

; wait for a keypress
wait_for_key:
		jsr k_keyb_scan
		cmp #$ff
		beq wait_for_key
		rts

; multiply accumulator with 10
mult10:
		asl					; accumulator = org * 2
		sta TMP
		asl					; accumulator = org * 4
		asl					; accumulator = org * 8
		adc TMP				; accumulator = (org * 8) + (org * 2)
		rts

; delay - poor man's sleep routine
delay:
		ldy #$02
@l2:	ldx #$ff
@l1:	nop
		dex
		bne @l1
		dey
		bne @l2
		rts

.rodata
MTAB:		 .byte 1,5,6,3,1,5,3,0,4,2,6,4   	; Month offsets

YEAR_MSG:	.asciiz "Type year (20xx)"
MONTH_MSG:	.asciiz "Type month (xx) "
DAY_MSG:	.asciiz "Type day (xx)   "

DAYS:		.asciiz "Sunday         "
			.asciiz "Monday         "
			.asciiz "Tuesday        "
			.asciiz "Wednesday      "
			.asciiz "Thursday       "
			.asciiz "Friday         "
			.asciiz "Saturday       "

END_MSG:	.asciiz "(press key)     "
