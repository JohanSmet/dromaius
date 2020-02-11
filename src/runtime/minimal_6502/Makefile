BINARIES = rom_test.bin rom_dayofweek.bin rom_test_pia.bin rom_test_lcd.bin rom_test_keypad.bin
ROM_TEST_OBJS = rom_test.o
ROM_DAYOFWEEK_OBJS = rom_dayofweek.o kernel.o
ROM_TEST_PIA_OBJS = rom_test_pia.o
ROM_TEST_LCD_OBJS = rom_test_lcd.o
ROM_TEST_KEYPAD_OBJS = test_keypad.o kernel.o

all: $(BINARIES)

%.o: %.s
	ca65 $< -o $@

rom_test.bin: $(ROM_TEST_OBJS) simple.cfg
	cl65 -o $@ -C simple.cfg $(ROM_TEST_OBJS)

rom_dayofweek.bin: $(ROM_DAYOFWEEK_OBJS) simple.cfg
	cl65 -o $@ -C simple.cfg $(ROM_DAYOFWEEK_OBJS)

rom_test_pia.bin: $(ROM_TEST_PIA_OBJS) simple.cfg
	cl65 -o $@ -C simple.cfg $(ROM_TEST_PIA_OBJS)

rom_test_lcd.bin: $(ROM_TEST_LCD_OBJS) simple.cfg
	cl65 -o $@ -C simple.cfg $(ROM_TEST_LCD_OBJS)

rom_test_keypad.bin: $(ROM_TEST_KEYPAD_OBJS) simple.cfg
	cl65 -o $@ -C simple.cfg $(ROM_TEST_KEYPAD_OBJS)

clean:
	$(RM) *.o *.bin

install: $(BINARIES)
	cp $(BINARIES) ../../../runtime/minimal_6502

.PHONY: all clean install
