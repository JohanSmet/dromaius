// signal_history_profiles.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_SIGNAL_HISTORY_PROFILES_H
#define DROMAIUS_SIGNAL_HISTORY_PROFILES_H

#ifdef __cplusplus
extern "C" {
#endif

// forward declaration of private types
struct SignalHistory;

struct DevCommodorePet;
struct Cpu6502;
struct Chip6520;
struct Chip6522;
struct Chip63xxRom;
struct PerifDatassette;
struct PerifDisk2031;

void dev_commodore_pet_history_profiles(struct DevCommodorePet *pet, const char *chip_name, struct SignalHistory *history);
void cpu_6502_signal_history_profiles(struct Cpu6502 *cpu, const char *chip_name, struct SignalHistory *history);
void chip_6520_signal_history_profiles(struct Chip6520 *pia, const char *chip_name, struct SignalHistory *history);
void chip_6522_signal_history_profiles(struct Chip6522 *via, const char *chip_name, struct SignalHistory *history);
void chip_63xx_signal_history_profiles(struct Chip63xxRom *rom, const char *chip_name, struct SignalHistory *history);
void perif_datassette_signal_history_profiles(struct PerifDatassette *datassette, const char *chip_name, struct SignalHistory *history);
void perif_disk2031_signal_history_profiles(struct PerifDisk2031 *fd2031, const char *chip_name, struct SignalHistory *history);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIGNAL_HISTORY_PROFILES_H

