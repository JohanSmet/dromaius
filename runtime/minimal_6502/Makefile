all: rom_test.bin rom_dayofweek.bin

rom_test.bin: rom_test.s
	cl65 rom_test.s -o rom_test.bin -C simple.cfg 

rom_dayofweek.bin: rom_dayofweek.s
	cl65 rom_dayofweek.s -o rom_dayofweek.bin -C simple.cfg 

clean:
	$(RM) *.o rom_test.bin rom_dayofweek.bin

.PHONY: all clean
