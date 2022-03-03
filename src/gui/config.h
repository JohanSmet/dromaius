// gui/config.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_GUI_CONFIG_H
#define DROMAIUS_GUI_CONFIG_H

#include "types.h"
#include <string_view>

enum class MachineType : int {
	CommodorePet = 0,
	CommodorePetLite = 1,
	Minimal6502 = 2
};

struct Config {
public:
	MachineType		machine_type = MachineType::CommodorePet;

public:
	bool set_machine_type(const std::string_view &text);

};

#endif // DROMAIUS_GUI_CONFIG_H
