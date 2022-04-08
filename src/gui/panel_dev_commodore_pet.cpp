// gui/panel_dev_commodore_pet.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include "panel_dev_commodore_pet.h"

#include <dev_commodore_pet.h>
#include <perif_pet_crt.h>

#if DMS_SIGNAL_TRACING
#include "signal_dumptrace.h"
#endif

#include "ui_context.h"
#include "widgets.h"
#include "imgui_ex.h"

#include "panel_chip_6520.h"
#include "panel_chip_6522.h"
#include "panel_cpu_6502.h"
#include "panel_memory.h"
#include "panel_monitor.h"
#include "panel_input_pet.h"
#include "panel_display_rgba.h"
#include "panel_signals.h"
#include "panel_logic_analyzer.h"
#include "panel_datassette.h"
#include "panel_disk_2031.h"

class PanelDevCommodorePet : public Panel {
public:
	PanelDevCommodorePet(UIContext *ctx, ImVec2 pos, DevCommodorePet *dev) :
		Panel(ctx),
		position(pos),
		device(dev) {

		auto &cat_memory = pet_hardware.add_category("Memory");
		cat_memory.add_leaf("Main Ram")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {220, 120}, ui_context->unique_panel_id("RAM").c_str(), 0x0000, 0x8000));
					});
		cat_memory.add_leaf("Video Ram")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {220, 120}, ui_context->unique_panel_id("VRAM").c_str(), 0x8000, 40*25));
					});
		cat_memory.add_leaf("Basic Rom")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(), 0xb000, 0x3000));
					});
		cat_memory.add_leaf("Editor Rom")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(), 0xe000, 0x0800));
					});
		cat_memory.add_leaf("Kernal Rom")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_memory_create(ui_context, {442, 120}, ui_context->unique_panel_id("ROM").c_str(), 0xf000, 0x1000));
					});

		auto &cat_cpu = pet_hardware.add_category("CPU");
		cat_cpu.add_leaf("MOS Technology 6502")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_cpu_6502_create(ui_context, {220, 342}, device->cpu));
					});

		auto &cat_support = pet_hardware.add_category("Support Chips");
		cat_support.add_leaf("PIA (6520) - IEEE-488")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_chip_6520_create(ui_context, {420, 342}, device->pia_1));
					});

		cat_support.add_leaf("PIA (6520) - Keyboard")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_chip_6520_create(ui_context, {420, 342}, device->pia_2));
					});
		cat_support.add_leaf("VIA (6522)")
					.add_action("View", [&]() {
						ui_context->panel_add(panel_chip_6522_create(ui_context, {420, 342}, device->via));
					});

		auto &cat_perif = pet_hardware.add_category("Peripherals");
		cat_perif.add_leaf("Datassette")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_datassette_create(ui_context, {340, 310}, device->datassette));
					});
		cat_perif.add_leaf("Floppy Disk")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_fd2031_create(ui_context, {340, 310}, device->disk_2031));
					});

		auto &cat_tools = pet_hardware.add_category("Tools");
		cat_tools.add_leaf("Monitor")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_monitor_create(ui_context, {340, 310}));
					});
		cat_tools.add_leaf("Logic Analyzer")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_logic_analyzer_create(ui_context, {340, 310}));
					});
		cat_tools.add_leaf("Signal Debugger")
					.add_action("Open", [&]() {
						ui_context->panel_add(panel_signals_create(ui_context, {340, 310}));
					});

		diag_mode = device->diag_mode;
	}

	void display() override {

		ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

		if (ImGui::Begin(title)) {

			#if DMS_SIGNAL_TRACING
				setup_signal_tracing();
			#endif // DMS_SIGNAL_TRACING

			pet_hardware.display();

			if (ImGui::CollapsingHeader("Advanced")) {
				if (ImGui::Checkbox("Assert Diagnostice Sense Line", &diag_mode)) {
					dev_commodore_pet_diag_mode(device, diag_mode);
				}
			}
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

	UITree			pet_hardware;
	bool			diag_mode = false;

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
