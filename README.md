# Dromaius

Welcome to the Dromaius repository. This is a hobby project where I implement emulation of older ("retro") hardware.

Dromaius is the genus of [ratite](https://en.wikipedia.org/wiki/Dromaius) present in Australia, of which the one extant species is the Dromaius novaehollandiae or **emu**. Yeah, naming things is hard.

## Features
- Emulation of several vintage MOS parts:
	- the 6502 CPU
	- the 6520 Peripheral Interface Adapter
	- the 6522 Versatile Interface Adapter
- Emulation of a selection of the 7400 series logic ICs
- Emulation of various ROM and RAM chips
- a simple computer (a cpu, a rom, and some ram) to be able to run a 6502 binary.
- an implementation of the Commodore PET 2001N 8-bit personal computer
- a, basic, cross-platform UI
- a browser-based (JavaScript and WebAssembly) schematic visualisation of the Commodore PET
- written in C11/C++ (and a smidgen of Python and JavaScript)

## Status

### MOS 6502
The 6502 emulator implements all official 6502 opcodes and strives to be cycle-accurate. 

Cycle-accurate in this case means that every opcode takes the same amount of clock cycles to complete as specified in the MOS Microcomputer Family Programming Manual and all operations happen on the appropriate clock edge. No attempt is made to exactly match the timing of each clock cycle, although the emulator does it's best to run at the requested frequency.

The 6502 passes the functional tests for 6502/65C02 type processors by Klaus Dorman ([GitHub repo](https://github.com/Klaus2m5/6502_65C02_functional_tests)). With exception of the BRK-instruction because the current test setup doesn't allow the interrupt vectors to be changed.

### Commodore PET 2001N
Dromaius emulates the digital logic of all the ICs in the PET. This makes for a signal accurate, barring bugs, albeit rather slow emulation of the pet.

The emulator boots up to the basic prompt, which should be fully functional. Programs can be loaded from an emulated datassette or emulated floppy disk. The fully emulated PET runs at about 1/10th of the speed of a real PET on my aging quad core computer. A lite version that drops emulation of the DRAM refresh circuitry and simplifies the display section is also implemented. This runs at about full speed (again on my ageing computer) allowing for better testability of the other components.

## More information
- [Building](docs/building.md) Dromaius from source.
- [Todo](docs/todo.md). Dromaius is very much a work-in-progress.

## License
Code and documentation in this repository is provided under the 3-clause BSD license unless stated otherwise.

