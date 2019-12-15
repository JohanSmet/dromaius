# Dromaius

Welcome to the Dromaius repository. This is a hobby project where I implement emulation of older ("retro") hardware.

Dromaius is the genus of [ratite](https://en.wikipedia.org/wiki/Dromaius) present in Australia, of which the one extant species is the Dromaius novaehollandiae or **emu**. Yeah, naming things is hard.

## Features
- a 6502 CPU emulator.
- a simple computer (a cpu, a rom, and some ram) to be able to run a 6502 binary.
- a, basic, cross-platform UI
- written in C11

## Status
The 6502 emulator implements all official 6502 opcodes and strives to be cycle-accurate. 

Cycle-accurate in this case meaning that every opcode takes the same amount of clock cycles to complete as specified in the MOS Microcomputer Family Programming Manual and all operations happen on the appropriate clock edge. No attempt is made to exactly match the timing of each clock cycle, although the emulator does it's best to run at the requested frequency.

The 6502 passes the functional tests for 6502/65C02 type processors by Klaus Dorman ([GitHub repo](https://github.com/Klaus2m5/6502_65C02_functional_tests)). With exception of the BRK-instruction because the current test setup doesn't allow the interrupt vectors to be changed.

## License
Code and documentation in this repository is provided under the 3-clause BSD license unless stated otherwise.

