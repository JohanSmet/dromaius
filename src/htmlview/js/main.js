// js/main.js - Johan Smet - BSD-3-Clause (see LICENSE)

import {PanelCpu6502} from './panel_cpu_6502.js';
import {PanelClock} from './panel_clock.js';
import {PanelBreakpointsSignal} from './panel_breakpoints_signal.js';
import {PanelSignalDetails} from './panel_signal_details.js';
import {PanelScreen} from './panel_screen.js';
import {PanelKeyboard} from './panel_keyboard.js';
import {PanelDatassette} from './panel_datassette.js';
import {PanelDisk2031} from './panel_disk2031.js';
import {CircuitView} from './circuit_view.js';

// configuration
const UI_REFRESH_RATE = 30;
const UI_SIGNAL_COLORS = ["#009900", "#00FF00"];

export class MainUI {

	constructor(emscripten_module) {
		// get access to the dromaius API
		this.dmsapi = new emscripten_module.DmsApi();

		// start the emulator
		this.switch_machine('Pet');

		// UI - menu bar
		this.ui_setup_menubar();
	}

	switch_machine(machine) {
		// stop emulation
		if (this.panel_screen) {
			this.deactivate_refresh_timer();
			this.ui_close_panels();
			this.dmsapi.stop_emulation();

			$('div#right_column').empty();
			this.circuit_view = null;
		}

		// start machine
		const lite = machine == 'PetLite';
		this.dmsapi.launch_commodore_pet(lite);

		// build list of css variables used to control the display color of a signal
		this.signal_css_vars = []
		this.setup_signal_css_variables();

		// UI - panels
		this.ui_setup_panels();

		// UI - circuit viewport
		this.circuit_view = new CircuitView($('div#right_column'), lite);

		// UI - signal hovering
		this.hovered_signal = '';
		this.ui_setup_signal_hovering()

		// start ui refresh timer
		this.refresh_timer = 0
		this.last_simulation_tick_low = 0;
		this.last_simulation_tick_high = 0;
		this.activate_refresh_timer();
	}

	refresh_timer_func() {
		if (this.dmsapi.context_execute()) {
			this.refresh_signals();
			this.refresh_cpu_panel();
			this.refresh_clock_panel();

			// refresh the screen
			this.panel_screen.update(this.dmsapi);

			// refresh the keyboard
			this.panel_keyboard.update();

			// refresh datassette
			this.panel_datassette.update();

			// enable/disable approriate menu buttons
			this.ui_refresh_menubar();
		}
	}

	activate_refresh_timer() {
		if (this.refresh_timer == 0) {
			this.refresh_timer = setInterval(() => { this.refresh_timer_func()}, 1000 / UI_REFRESH_RATE);
		}
	}

	deactivate_refresh_timer() {
		if (this.refresh_timer != 0) {
			clearInterval(this.refresh_timer);
			this.refresh_timer = 0;
		}
	}

	refresh_signals() {
		if (this.circuit_view.svg_document == null) {
			return;
		}

		var svg_style = document.documentElement.style;

		const sig_data = this.dmsapi.signal_data();
		for (var s_id = 0; s_id < sig_data.length; ++s_id) {
			var s_name = this.signal_css_vars[s_id];
			svg_style.setProperty(s_name, UI_SIGNAL_COLORS[sig_data[s_id]]);
		}

		if (this.hovered_signal != '') {
			svg_style.setProperty(this.hovered_signal, 'blue');
		}
	}

	refresh_cpu_panel() {
		var cpu_info = this.dmsapi.cpu_info();
		this.panel_cpu.update(cpu_info);
	}

	refresh_clock_panel() {
		var clock_info = this.dmsapi.clock_info();
		this.panel_clock.update(clock_info);

		if (clock_info.current_tick_low == this.last_simulation_tick_low &&
		    clock_info.current_tick_high == this.last_simulation_tick_high) {
			this.deactivate_refresh_timer();
		}

		this.last_simulation_tick_low = clock_info.current_tick_low;
		this.last_simulation_tick_high = clock_info.current_tick_high;
	}

	setup_signal_css_variables() {
		const signal_info = this.dmsapi.signal_info();

		// >> initialize to empty for all signals (even unnamed signals)
		for (var i = 0; i < signal_info.count; ++i) {
			this.signal_css_vars.push('');
		}

		// >> define variable for each named signal
		var sigkeys = signal_info.names.keys();

		for (var s = 0; s < sigkeys.size(); ++s) {
			var sig_name = sigkeys.get(s);
			var signal = signal_info.names.get(sig_name);
			this.signal_css_vars[signal] = this.css_variable_for_signal(sig_name);
		}
	}

	css_variable_for_signal(name) {
		return '--color-' + name.replace('/', 'bar');
	}

	ui_setup_panels() {
		this.panel_screen = new PanelScreen(this.dmsapi.display_info());
		this.panel_clock = new PanelClock();
		this.panel_cpu = new PanelCpu6502();

		this.panel_signal_details = new PanelSignalDetails(this.dmsapi);
		this.panel_signal_details.on_breakpoint_set = (signal) => {
			this.panel_breakpoints_signal.update();
		};

		this.panel_breakpoints_signal = new PanelBreakpointsSignal(this.dmsapi);
		this.panel_datassette = new PanelDatassette(this.dmsapi);
		this.panel_disk2031 = new PanelDisk2031(this.dmsapi);
		this.panel_keyboard = new PanelKeyboard(this.dmsapi);
	}

	ui_close_panels() {
		this.panel_screen.close();
		this.panel_clock.close();
		this.panel_cpu.close();

		this.panel_signal_details.close();

		this.panel_breakpoints_signal.close();
		this.panel_datassette.close();
		this.panel_disk2031.close();
		this.panel_keyboard.close();
	}

	ui_setup_menubar() {
		$('#btnStepInstruction').on('click', () => { this.ui_cmd_step_instruction(); });
		$('#btnStepClock').on('click', () => { this.ui_cmd_step_clock(); });

		$('#cmbMachine').on('change', () => {
			const type = $('#cmbMachine').children('option:selected').val();
			this.switch_machine(type);
		});

		$('#cmbStepClock').on('change', () => {
			const signal_name =	$('#cmbStepClock').children('option:selected').val();
			this.ui_cmd_change_set_clock_signal(signal_name);
		});

		$('#btnRun').on('click', () => { this.ui_cmd_run(); });
		$('#btnPause').on('click', () => { this.ui_cmd_pause(); });
		$('#btnReset').on('click', () => { this.ui_cmd_reset(); });

		$("#btnAboutOpen").on("click", function (event) {
			$("#about").show();
			$("#btnAboutOpen").attr("disabled", "");
			$("#btnHelpOpen").attr("disabled", "");
		});

		$("#btnAboutClose").on("click", function (event) {
			$("#about").hide();
			$("#btnAboutOpen").removeAttr("disabled");
			$("#btnHelpOpen").removeAttr("disabled");
		});

		$("#btnHelpOpen").on("click", function (event) {
			$("#help").show();
			$("#btnAboutOpen").attr("disabled", "");
			$("#btnHelpOpen").attr("disabled", "");
		});

		$("#btnHelpClose").on("click", function (event) {
			$("#help").hide();
			$("#btnAboutOpen").removeAttr("disabled");
			$("#btnHelpOpen").removeAttr("disabled");
		});
	}

	ui_refresh_menubar(state) {
		var enable_btn = function(btn, enable) {
			if (enable) {
				btn.removeAttr('disabled');
			} else {
				btn.attr('disabled', '');
			}
		}

		const context_state = this.dmsapi.context_status();
		const waiting = context_state == "wait";

		enable_btn($('#btnStepInstruction'), waiting);
		enable_btn($('#btnStepClock'), waiting);
		enable_btn($('#cmbStepClock'), waiting);
		enable_btn($('#btnRun'), waiting);
		enable_btn($('#btnPause'), context_state == "run");
	}

	ui_setup_signal_hovering() {
		this.circuit_view.on_signal_hovered = (signal) => {
			var style = this.circuit_view.svg_document.style;

			if (signal != '') {
				this.hovered_signal = this.css_variable_for_signal(signal);
				style.setProperty(this.hovered_signal, 'blue');
			} else {
				this.hovered_signal = '';
				for (var i = style.length; i--;) {
					style.removeProperty(style[i]);
				}
			}
		}

		this.circuit_view.on_signal_clicked = (signal) => {
			this.panel_signal_details.update(signal);
		}
	}

	ui_cmd_step_instruction() {
		this.dmsapi.context_step_instruction();
		this.activate_refresh_timer();
	}

	ui_cmd_step_clock() {
		this.dmsapi.context_step_clock();
		this.activate_refresh_timer();
	}

	ui_cmd_change_set_clock_signal(signal_name) {
		this.dmsapi.context_select_step_clock(signal_name);
	}

	ui_cmd_run() {
		this.dmsapi.context_run();
		this.activate_refresh_timer();
	}

	ui_cmd_pause() {
		this.dmsapi.context_pause();
	}

	ui_cmd_reset() {
		this.dmsapi.context_reset();
		this.activate_refresh_timer();
	}
};
