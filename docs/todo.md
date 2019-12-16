# Dromaius: Todo

## General
- [ ] Figure out a consistent, but flexible, way to pass signals between components
	- ATM dromaius uses a combination of integers (for multi-bit values) and bools (for single-bit)
	- the 6502 expects pointer to shared data
	- the ROM/RAM modules expect data to be copied in & out
	- whatever the solution it should be feasable to group multiple bits at one point and split them out later (e.g. for address lines)
	- it would be nice if connections react the way they would in a real electrical circuit
		- e.g. support wire-OR for multiple open-drain output (to combine irq-lines with pull ups)
- [ ] Add more devices :-)

## Library: 6502
- [ ] Implement the unofficial/illegal 6502 opcodes
- [ ] Add the variants of the 6502 (e.g. 65c02 and others)
- [ ] Refactor/optimize the implementation
		- right now it's basically a bunch of nested switch statements, switch to a coroutine-like method might require less branches
		- another option is to structure the emulator more like a real 6502 (see https://www.pagetable.com/?p=39)
