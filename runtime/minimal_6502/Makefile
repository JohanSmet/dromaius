rom_test.bin: rom_test.s
	cl65 rom_test.s -o rom_test.bin -C simple.cfg -m rom_test.map

clean:
	$(RM) *.o *.bin

.PHONY: clean
