; kernel.s - Johan Smet - BSD-3-Clause (see LICENSE)
;
; basic OS functionality for the minimal_6502 device
;	-> the kernel isn't a separate rom at the moment
;	-> this module is linked with the program rom, so no fixed jump-table is required
;	-> sets the reset, irq and nmi vectors for the 6502
;		- reset initializes the machine and passes control to the "main"-symbol exported
;		  by another module
;		- irq / nmi aren't passed to the user program at the moment because these aren't
;		  connected in the minimal_6502

; **** FUNCTION OVERVIEW ****
; *) k_lcd_write_cmd : send a command to the LCD module
;		in-data: acc = command
; *) k_lcd_write_data : send 8-bits of data to be displayed on the LCD module
;		in-data: acc = data
; *) k_lcd_clear : clear the LCD screen 
; *) k_lcd_show_string : displays a string at the specified position of the LCD
;		in-data: acc = position 
;		         x   = low byte of address of string 
;		         y   = high byte of address of string

.ifndef KERNEL_GUARD
.define KERNEL_GUARD 1

.import k_lcd_write_cmd			; send command to the LCD
.import k_lcd_write_data		; send data to the LCD
.import k_lcd_clear				; clear the LCD
.import k_lcd_show_string		; show string on the lcd

.endif ; KERNEL_GUARD
