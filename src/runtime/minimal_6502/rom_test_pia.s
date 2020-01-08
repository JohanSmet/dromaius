; rom_test_pia.s - Johan Smet - BSD-3-Clause (see LICENSE)

	.setcpu "6502"

	.segment "OS"

; program entry
vec_reset:
		; initialize stack pointer
		ldx #$ff
		txs

		; set DDRA to all output
		lda #$ff
		sta $8000

@end:	jmp @end

; non-maskable interrupt handler
vec_nmi:
		jmp vec_nmi

; irq-handler
vec_irq:
		jmp vec_irq


; set vectors at the end of the ROM
	.segment "VECTORS"
		.word vec_nmi
		.word vec_reset
		.word vec_irq

