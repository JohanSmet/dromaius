// gui/panel_dev_commodore_pet.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_dev_commodore_pet.h"

#include <dev_commodore_pet.h>
#include <perif_pet_crt.h>

#if DMS_SIGNAL_TRACING
#include "signal_dumptrace.h"
#endif

#include "ui_context.h"
#include "widgets.h"

#include "panel_chip_6520.h"
#include "panel_chip_6522.h"
#include "panel_cpu_6502.h"
#include "panel_memory.h"
#include "panel_monitor.h"
#include "panel_input_pet.h"
#include "panel_display_rgba.h"
#include "panel_signals.h"
#include "panel_datassette.h"
#include "panel_disk_2031.h"

static const UITree<DevCommodorePet>::data_t PET_HARDWARE = {
	UI_TREE_NODE("Memory")
		UI_TREE_LEAF("Main Ram")
			UI_TREE_ACTION_PANEL("View")
				return panel_memory_create(ctx, {220, 120}, ctx->unique_panel_id("RAM").c_str(), 0x0000, 0x8000);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
		UI_TREE_LEAF("Video Ram")
			UI_TREE_ACTION_PANEL("View")
				return panel_memory_create(ctx, {220, 120}, ctx->unique_panel_id("VRAM").c_str(), 0x8000, 40*25);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
		UI_TREE_LEAF("Basic ROM")
			UI_TREE_ACTION_PANEL("View")
				return panel_memory_create(ctx, {442, 120}, ctx->unique_panel_id("ROM").c_str(), 0xb000, 0x3000);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
		UI_TREE_LEAF("Editor ROM")
			UI_TREE_ACTION_PANEL("View")
				return panel_memory_create(ctx, {442, 120}, ctx->unique_panel_id("ROM").c_str(), 0xe000, 0x0800);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
		UI_TREE_LEAF("Kernal ROM")
			UI_TREE_ACTION_PANEL("View")
				return panel_memory_create(ctx, {442, 120}, ctx->unique_panel_id("ROM").c_str(), 0xf000, 0x1000);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
	UI_TREE_NODE_END,

	UI_TREE_NODE("CPU")
		UI_TREE_LEAF("MOS Technology 6502")
			UI_TREE_ACTION_PANEL("View")
				return panel_cpu_6502_create(ctx, {220, 342}, dev->cpu);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
	UI_TREE_NODE_END,

	UI_TREE_NODE("Support Chips")
		UI_TREE_LEAF("PIA (6520) - IEEE-488")
			UI_TREE_ACTION_PANEL("View")
				return panel_chip_6520_create(ctx, {420, 342}, dev->pia_1);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
		UI_TREE_LEAF("PIA (6520) - Keyboard")
			UI_TREE_ACTION_PANEL("View")
				return panel_chip_6520_create(ctx, {420, 342}, dev->pia_2);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
		UI_TREE_LEAF("VIA (6522)")
			UI_TREE_ACTION_PANEL("View")
				return panel_chip_6522_create(ctx, {420, 342}, dev->via);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
	UI_TREE_NODE_END,

	UI_TREE_NODE("Peripherals")
		UI_TREE_LEAF("Datassette")
			UI_TREE_ACTION_PANEL("Open")
				return panel_datassette_create(ctx, {340, 310}, dev->datassette);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
		UI_TREE_LEAF("Floppy Disk")
			UI_TREE_ACTION_PANEL("Open")
				return panel_fd2031_create(ctx, {340, 310}, dev->disk_2031);
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
	UI_TREE_NODE_END,

	UI_TREE_NODE("Tools")
		UI_TREE_LEAF("Monitor")
			UI_TREE_ACTION_PANEL("Open")
				return panel_monitor_create(ctx, {340, 310});
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
		UI_TREE_LEAF("Signal Debugger")
			UI_TREE_ACTION_PANEL("Open")
				return panel_signals_create(ctx, {340, 310});
			UI_TREE_ACTION_END
		UI_TREE_LEAF_END,
	UI_TREE_NODE_END,

};

class PanelDevCommodorePet : public Panel {
public:
	PanelDevCommodorePet(UIContext *ctx, ImVec2 pos, DevCommodorePet *device) :
		Panel(ctx),
		position(pos),
		device(device) {
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title)) {

			#if DMS_SIGNAL_TRACING
				setup_signal_tracing();
			#endif // DMS_SIGNAL_TRACING

			UITree<DevCommodorePet>::display(PET_HARDWARE, ui_context, device);
		}

		ImGui::End();
	}

private:

#ifdef DMS_SIGNAL_TRACING
	void setup_signal_tracing() {
		if (device->simulator->signal_pool->trace) {
			if (ImGui::Button("Stop trace")) {
				signal_trace_close(device->simulator->signal_pool->trace);
				device->simulator->signal_pool->trace = NULL;
			}
		} else {
			if (ImGui::Button("Start trace")) {
				device->simulator->signal_pool->trace = signal_trace_open("dromaius.lxt", device->simulator->signal_pool);
				auto *trace = device->simulator->signal_pool->trace;

				signal_trace_set_timestep_duration(trace, device->simulator->tick_duration_ps);

				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_EOI_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_EOI_IN_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_EOI_OUT_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DAV_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DAV_IN_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DAV_OUT_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_NRFD_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_NRFD_IN_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_NRFD_OUT_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_NDAC_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_NDAC_IN_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_NDAC_OUT_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_ATN_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_ATN_IN_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_ATN_OUT_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_IFC_B]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_SRQ_IN_B]);

				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DIO0]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DIO1]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DIO2]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DIO3]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DIO4]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DIO5]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DIO6]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DIO7]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DI0]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DI1]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DI2]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DI3]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DI4]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DI5]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DI6]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DI7]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DO0]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DO1]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DO2]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DO3]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DO4]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DO5]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DO6]);
				signal_trace_enable_signal(trace, device->signals[SIG_P2001N_DO7]);
			}
		}
	}
#endif // DMS_SIGNAL_TRACING

private:
	ImVec2			position;
	const ImVec2	size = {0, 0};
	DevCommodorePet *device;

	constexpr static const char *title = "Device - Commodore PET 2001N";
};

Panel::uptr_t panel_dev_commodore_pet_create(UIContext *ctx, ImVec2 pos, DevCommodorePet *device) {

	// a keyboard panel is always useful
	auto keyboard_pnl = panel_input_pet_create(ctx, {340, 512}, device->keypad);
	ctx->panel_add(std::move(keyboard_pnl));

	// display panel
	auto display_pnl = panel_display_rgba_create(ctx, {340, 10}, device->screen);
	ctx->panel_add(std::move(display_pnl));

	// create panel for the commodore pet
	return std::make_unique<PanelDevCommodorePet>(ctx, pos, device);
}
