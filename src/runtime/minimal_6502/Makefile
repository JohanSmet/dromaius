BINARIES = rom_test.bin rom_dayofweek.bin rom_test_pia.bin rom_test_lcd.bin

all: $(BINARIES)

rom_test.bin: rom_test.s simple.cfg
	cl65 rom_test.s -o rom_test.bin -C simple.cfg 

rom_dayofweek.bin: rom_dayofweek.s simple.cfg
	cl65 rom_dayofweek.s -o rom_dayofweek.bin -C simple.cfg 

rom_test_pia.bin: rom_test_pia.s simple.cfg
	cl65 rom_test_pia.s -o rom_test_pia.bin -C simple.cfg 

rom_test_lcd.bin: rom_test_lcd.s simple.cfg
	cl65 -t none rom_test_lcd.s -o rom_test_lcd.bin -C simple.cfg 

clean:
	$(RM) *.o *.bin 

install: $(BINARIES)
	cp $(BINARIES) ../../../runtime/minimal_6502

.PHONY: all clean install
