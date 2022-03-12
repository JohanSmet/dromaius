# Dromaius: Todo

## General
- [X] Show what is happening at each pin. ATM only the state of the node is shown and it's unclear which chip is responsible
      for the value on the line and which are reading the value or in high-Z.
- [ ] Add more devices :-)

## Library: simulator
- [-] Simulate the chips concurrently on multiple threads
		=> Attempts were made but the small amount of work done in each simulation step means the overhead from threading
		   completely eliminates any benefits of concurrent execution.
- [X] Try to eliminate signal writes in the chip simulation threads to decrease the load for the consolidation step at the end.
		=> First round of optimizations have been done.

## Library: 6502
- [ ] Implement the unofficial/illegal 6502 opcodes
- [ ] Add the variants of the 6502 (e.g. 65c02 and others)
- [ ] Refactor/optimize the implementation
		- right now it's basically a bunch of nested switch statements, switch to a coroutine-like method might require less branches
		- another option is to structure the emulator more like a real 6502 (see https://www.pagetable.com/?p=39)
		- low priority: this isn't the slowest part of the emulator

## Library: general features
- [ ] Implement save states.
- [ ] Investigate if it's possible to allow writes to signal queues of future timesteps
		-> e.g. to simulate read delays of ROM/RAM without wake-up events
- [ ] Save the signal writes in a transaction log that allows us to rollback to a previous timestep

## UI
- [ ] Display the SVG schematic (and keyboard?) in the desktop application
- [ ] Display the history of signals in a logic analyzer graphical visualization.
