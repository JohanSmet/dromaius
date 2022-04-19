// signal_history_profiles.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "signal_history_profiles.h"
#include "signal_history.h"

#include "dev_commodore_pet.h"

#include "cpu_6502.h"
#include "chip_6520.h"
#include "chip_6522.h"
#include "chip_rom.h"

#include "perif_datassette_1530.h"
#include "perif_disk_2031.h"

void dev_commodore_pet_history_profiles(struct DevCommodorePet *pet, const char *chip_name, struct SignalHistory *history) {

	uint32_t prof_data = signal_history_profile_create(history, chip_name, "Data Bus");
	for (size_t i = 0; i < arrlenu(pet->sg_cpu_data); ++i) {
		signal_history_profile_add_signal(history, prof_data, *pet->sg_cpu_data[i], NULL);
	}

	uint32_t prof_address = signal_history_profile_create(history, chip_name, "Address Bus");
	for (size_t i = 0; i < arrlenu(pet->sg_cpu_address); ++i) {
		signal_history_profile_add_signal(history, prof_address, *pet->sg_cpu_address[i], NULL);
	}

	uint32_t prof_video = signal_history_profile_create(history, chip_name, "Video");
	signal_history_profile_add_signal(history, prof_video, pet->signals[SIG_P2001N_HORZ_DISP_ON], NULL);
	signal_history_profile_add_signal(history, prof_video, pet->signals[SIG_P2001N_HORZ_DISP_OFF], NULL);
	signal_history_profile_add_signal(history, prof_video, pet->signals[SIG_P2001N_HORZ_DRIVE], NULL);
	signal_history_profile_add_signal(history, prof_video, pet->signals[SIG_P2001N_VIDEO_LATCH], NULL);
	signal_history_profile_add_signal(history, prof_video, pet->signals[SIG_P2001N_VERT_DRIVE], NULL);
	signal_history_profile_add_signal(history, prof_video, pet->signals[SIG_P2001N_VIDEO_ON], NULL);
	signal_history_profile_add_signal(history, prof_video, pet->signals[SIG_P2001N_VIDEO], NULL);

	uint32_t prof_phases = signal_history_profile_create(history, chip_name, "Clk1 Phases");
	signal_history_profile_add_signal(history, prof_phases, pet->signals[SIG_P2001N_BPHI2A], NULL);
	signal_history_profile_add_signal(history, prof_phases, pet->signals[SIG_P2001N_BPHI2B], NULL);
	signal_history_profile_add_signal(history, prof_phases, pet->signals[SIG_P2001N_BPHI2C], NULL);
	signal_history_profile_add_signal(history, prof_phases, pet->signals[SIG_P2001N_BPHI2D], NULL);
	signal_history_profile_add_signal(history, prof_phases, pet->signals[SIG_P2001N_BPHI2E], NULL);
	signal_history_profile_add_signal(history, prof_phases, pet->signals[SIG_P2001N_BPHI2F], NULL);
	signal_history_profile_add_signal(history, prof_phases, pet->signals[SIG_P2001N_BPHI2G], NULL);
	signal_history_profile_add_signal(history, prof_phases, pet->signals[SIG_P2001N_BPHI2H], NULL);
}

void cpu_6502_signal_history_profiles(struct Cpu6502 *cpu, const char *chip_name, struct SignalHistory *history) {

	uint32_t prof_ctrl = signal_history_profile_create(history, chip_name, "Control Signals");
	signal_history_profile_add_signal(history, prof_ctrl, cpu->signals[PIN_6502_RES_B], "/RES");
	signal_history_profile_add_signal(history, prof_ctrl, cpu->signals[PIN_6502_CLK],	"PHI0");
	signal_history_profile_add_signal(history, prof_ctrl, cpu->signals[PIN_6502_RDY],	"RDY");
	signal_history_profile_add_signal(history, prof_ctrl, cpu->signals[PIN_6502_SYNC],	"SYNC");
	signal_history_profile_add_signal(history, prof_ctrl, cpu->signals[PIN_6502_RW],	"RW");
	signal_history_profile_add_signal(history, prof_ctrl, cpu->signals[PIN_6502_IRQ_B],	"/IRQ");
	signal_history_profile_add_signal(history, prof_ctrl, cpu->signals[PIN_6502_NMI_B], "/NMI");
}

void chip_6520_signal_history_profiles(struct Chip6520 *pia, const char *chip_name, struct SignalHistory *history) {

	uint32_t prof_ctrl = signal_history_profile_create(history, chip_name, "Control Signals");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_RESET_B],	"/RESET");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_PHI2],		"PHI2");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_CA1],		"CA1");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_CA2],		"CA2");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_CB1],		"CB1");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_CB2],		"CB2");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_IRQA_B],	"/IRQA");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_IRQB_B],	"/IRQB");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_RW],		"RW");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_RS0],		"RS0");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_RS1],		"RS1");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_CS0],		"CS0");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_CS1],		"CS1");
	signal_history_profile_add_signal(history, prof_ctrl, pia->signals[CHIP_6520_CS2_B],	"/CS2");

	uint32_t prof_porta = signal_history_profile_create(history, chip_name, "Port-A");
	signal_history_profile_add_signal(history, prof_porta, pia->signals[CHIP_6520_PA0],		"PA0");
	signal_history_profile_add_signal(history, prof_porta, pia->signals[CHIP_6520_PA1],		"PA1");
	signal_history_profile_add_signal(history, prof_porta, pia->signals[CHIP_6520_PA2],		"PA2");
	signal_history_profile_add_signal(history, prof_porta, pia->signals[CHIP_6520_PA3],		"PA3");
	signal_history_profile_add_signal(history, prof_porta, pia->signals[CHIP_6520_PA4],		"PA4");
	signal_history_profile_add_signal(history, prof_porta, pia->signals[CHIP_6520_PA5],		"PA5");
	signal_history_profile_add_signal(history, prof_porta, pia->signals[CHIP_6520_PA6],		"PA6");
	signal_history_profile_add_signal(history, prof_porta, pia->signals[CHIP_6520_PA7],		"PA7");

	uint32_t prof_portb = signal_history_profile_create(history, chip_name, "Port-B");
	signal_history_profile_add_signal(history, prof_portb, pia->signals[CHIP_6520_PB0],		"PB0");
	signal_history_profile_add_signal(history, prof_portb, pia->signals[CHIP_6520_PB1],		"PB1");
	signal_history_profile_add_signal(history, prof_portb, pia->signals[CHIP_6520_PB2],		"PB2");
	signal_history_profile_add_signal(history, prof_portb, pia->signals[CHIP_6520_PB3],		"PB3");
	signal_history_profile_add_signal(history, prof_portb, pia->signals[CHIP_6520_PB4],		"PB4");
	signal_history_profile_add_signal(history, prof_portb, pia->signals[CHIP_6520_PB5],		"PB5");
	signal_history_profile_add_signal(history, prof_portb, pia->signals[CHIP_6520_PB6],		"PB6");
	signal_history_profile_add_signal(history, prof_portb, pia->signals[CHIP_6520_PB7],		"PB7");
}

void chip_6522_signal_history_profiles(struct Chip6522 *via, const char *chip_name, struct SignalHistory *history) {

	uint32_t prof_ctrl = signal_history_profile_create(history, chip_name, "Control Signals");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_RESET_B],	"/RESET");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_PHI2],		"PHI2");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_CA1],		"CA1");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_CA2],		"CA2");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_CB1],		"CB1");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_CB2],		"CB2");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_IRQ_B],	"/IRQ");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_RW],		"RW");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_RS0],		"RS0");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_RS1],		"RS1");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_RS2],		"RS2");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_RS3],		"RS3");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_CS1],		"CS1");
	signal_history_profile_add_signal(history, prof_ctrl, via->signals[CHIP_6522_CS2_B],	"/CS2");

	uint32_t prof_porta = signal_history_profile_create(history, chip_name, "Port-A");
	signal_history_profile_add_signal(history, prof_porta, via->signals[CHIP_6522_PA0],		"PA0");
	signal_history_profile_add_signal(history, prof_porta, via->signals[CHIP_6522_PA1],		"PA1");
	signal_history_profile_add_signal(history, prof_porta, via->signals[CHIP_6522_PA2],		"PA2");
	signal_history_profile_add_signal(history, prof_porta, via->signals[CHIP_6522_PA3],		"PA3");
	signal_history_profile_add_signal(history, prof_porta, via->signals[CHIP_6522_PA4],		"PA4");
	signal_history_profile_add_signal(history, prof_porta, via->signals[CHIP_6522_PA5],		"PA5");
	signal_history_profile_add_signal(history, prof_porta, via->signals[CHIP_6522_PA6],		"PA6");
	signal_history_profile_add_signal(history, prof_porta, via->signals[CHIP_6522_PA7],		"PA7");

	uint32_t prof_portb = signal_history_profile_create(history, chip_name, "Port-B");
	signal_history_profile_add_signal(history, prof_portb, via->signals[CHIP_6522_PB0],		"PB0");
	signal_history_profile_add_signal(history, prof_portb, via->signals[CHIP_6522_PB1],		"PB1");
	signal_history_profile_add_signal(history, prof_portb, via->signals[CHIP_6522_PB2],		"PB2");
	signal_history_profile_add_signal(history, prof_portb, via->signals[CHIP_6522_PB3],		"PB3");
	signal_history_profile_add_signal(history, prof_portb, via->signals[CHIP_6522_PB4],		"PB4");
	signal_history_profile_add_signal(history, prof_portb, via->signals[CHIP_6522_PB5],		"PB5");
	signal_history_profile_add_signal(history, prof_portb, via->signals[CHIP_6522_PB6],		"PB6");
	signal_history_profile_add_signal(history, prof_portb, via->signals[CHIP_6522_PB7],		"PB7");
}

void chip_63xx_signal_history_profiles(struct Chip63xxRom *rom, const char *chip_name, struct SignalHistory *history) {

	uint32_t prof_ctrl = signal_history_profile_create(history, chip_name, "Control Signals");
	if (rom->data_size == ROM_6316_DATA_SIZE) {
		signal_history_profile_add_signal(history, prof_ctrl, rom->signals[CHIP_6316_CS1_B],	"/CS1");
		signal_history_profile_add_signal(history, prof_ctrl, rom->signals[CHIP_6316_CS2_B],	"/CS2");
		signal_history_profile_add_signal(history, prof_ctrl, rom->signals[CHIP_6316_CS3],		"CS3");
	} else {
		signal_history_profile_add_signal(history, prof_ctrl, rom->signals[CHIP_6332_CS1_B],	"/CS1");
		signal_history_profile_add_signal(history, prof_ctrl, rom->signals[CHIP_6332_CS3],		"CS3");
	}
}

void perif_datassette_signal_history_profiles(struct PerifDatassette *datassette, const char *chip_name, struct SignalHistory *history) {

	uint32_t prof_ctrl = signal_history_profile_create(history, chip_name, "Signals");
	signal_history_profile_add_signal(history, prof_ctrl, datassette->signals[PIN_DS1530_MOTOR],		"MOTOR");
	signal_history_profile_add_signal(history, prof_ctrl, datassette->signals[PIN_DS1530_SENSE],		"SENSE");
	signal_history_profile_add_signal(history, prof_ctrl, datassette->signals[PIN_DS1530_DATA_FROM_DS],	"TO-DS");
	signal_history_profile_add_signal(history, prof_ctrl, datassette->signals[PIN_DS1530_DATA_TO_DS],	"FROM-DS");
}

void perif_disk2031_signal_history_profiles(struct PerifDisk2031 *fd2031, const char *chip_name, struct SignalHistory *history) {

	uint32_t prof_ctrl = signal_history_profile_create(history, chip_name, "Signals");
	signal_history_profile_add_signal(history, prof_ctrl, fd2031->signals[PERIF_FD2031_EOI_B],	"/EOI");
	signal_history_profile_add_signal(history, prof_ctrl, fd2031->signals[PERIF_FD2031_DAV_B],	"/DAV");
	signal_history_profile_add_signal(history, prof_ctrl, fd2031->signals[PERIF_FD2031_NRFD_B],	"/NRFD");
	signal_history_profile_add_signal(history, prof_ctrl, fd2031->signals[PERIF_FD2031_NDAC_B],	"/NDAC");
	signal_history_profile_add_signal(history, prof_ctrl, fd2031->signals[PERIF_FD2031_ATN_B],	"/ATN");
	signal_history_profile_add_signal(history, prof_ctrl, fd2031->signals[PERIF_FD2031_SRQ_B],	"/SRQ");
	signal_history_profile_add_signal(history, prof_ctrl, fd2031->signals[PERIF_FD2031_IFC_B],	"/IFC");

	uint32_t prof_io = signal_history_profile_create(history, chip_name, "I/O");
	signal_history_profile_add_signal(history, prof_io, fd2031->signals[PERIF_FD2031_DIO0],	"DIO0");
	signal_history_profile_add_signal(history, prof_io, fd2031->signals[PERIF_FD2031_DIO1],	"DIO1");
	signal_history_profile_add_signal(history, prof_io, fd2031->signals[PERIF_FD2031_DIO2],	"DIO2");
	signal_history_profile_add_signal(history, prof_io, fd2031->signals[PERIF_FD2031_DIO3],	"DIO3");
	signal_history_profile_add_signal(history, prof_io, fd2031->signals[PERIF_FD2031_DIO4],	"DIO4");
	signal_history_profile_add_signal(history, prof_io, fd2031->signals[PERIF_FD2031_DIO5],	"DIO5");
	signal_history_profile_add_signal(history, prof_io, fd2031->signals[PERIF_FD2031_DIO6],	"DIO6");
	signal_history_profile_add_signal(history, prof_io, fd2031->signals[PERIF_FD2031_DIO7],	"DIO7");
}
