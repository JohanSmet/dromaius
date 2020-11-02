// js/main.js - Johan Smet - BSD-3-Clause (see LICENSE)

import {PanelCpu6502} from './panel_cpu_6502.js';
import {PanelClock} from './panel_clock.js';
import {PanelBreakpointsSignal} from './panel_breakpoints_signal.js';
import {PanelSignalDetails} from './panel_signal_details.js';
import {PanelScreen} from './panel_screen.js';
import {PanelKeyboard} from './panel_keyboard.js';
import {CircuitView} from './circuit_view.js';

// configuration
const UI_REFRESH_RATE = 30;
const UI_SIGNAL_COLORS = ["#009900", "#00FF00"];

export class MainUI {

	constructor(emscripten_module) {
		// get access to the dromaius API
		this.dmsapi = new emscripten_module.DmsApi();

		// start the emulator
		this.dmsapi.launch_commodore_pet();

		// build list of css variables used to control the display color of a signal
		this.signal_css_vars = []
		this.setup_signal_css_variables();

		// UI - menu bar
		this.ui_setup_menubar();

		// UI - circuit viewport
		this.circuit_view = new CircuitView($('div#right_column'));

		// UI - panels
		this.panel_screen = new PanelScreen(this.dmsapi.display_info());
		this.panel_clock = new PanelClock();
		this.panel_cpu = new PanelCpu6502();

		this.panel_signal_details = new PanelSignalDetails(this.dmsapi);
		this.panel_signal_details.on_breakpoint_set = (signal) => {
			this.panel_breakpoints_signal.update();
		};

		this.panel_breakpoints_signal = new PanelBreakpointsSignal(this.dmsapi);
		this.panel_keyboard = new PanelKeyboard(this.dmsapi);

		// UI - signal hovering
		this.hovered_signal = '';
		this.ui_setup_signal_hovering()

		// start ui refresh timer
		setInterval(() => { this.refresh_handler()}, 1000 / UI_REFRESH_RATE);
	}

	refresh_handler() {
		if (this.dmsapi.context_execute()) {
			this.refresh_signals();
			this.refresh_cpu_panel();
			this.refresh_clock_panel();

			// refresh the screen
			this.panel_screen.update(this.dmsapi);

			// refresh the keyboard
			this.panel_keyboard.update();
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

			if (signal.count == 1) {
				this.signal_css_vars[signal.start] = this.css_variable_for_signal(sig_name);
			}
		}
	}

	css_variable_for_signal(name) {
		return '--color-' + name.replace('/', 'bar');
	}

	ui_setup_menubar() {
		$('#btnStepInstruction').on('click', () => { this.ui_cmd_step_instruction(); });
		$('#btnStepClock').on('click', () => { this.ui_cmd_step_clock(); });

		$('#cmbStepClock').on('change', () => {
			const signal_name =	$('#cmbStepClock').children('option:selected').val();
			this.ui_cmd_change_set_clock_signal(signal_name);
		});

		$('#btnRun').on('click', () => { this.ui_cmd_run(); });
		$('#btnPause').on('click', () => { this.ui_cmd_pause(); });
		$('#btnReset').on('click', () => { this.ui_cmd_reset(); });

		$("#btnAboutOpen").on("click", function (event) {
			$("#about").show();
		});

		$("#btnAboutClose").on("click", function (event) {
			$("#about").hide();
		});
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
	}

	ui_cmd_step_clock() {
		this.dmsapi.context_step_clock();
	}

	ui_cmd_change_set_clock_signal(signal_name) {
		this.dmsapi.context_select_step_clock(signal_name);
	}

	ui_cmd_run() {
		this.dmsapi.context_run();
	}

	ui_cmd_pause() {
		this.dmsapi.context_pause();
	}

	ui_cmd_reset() {
		this.dmsapi.context_reset();
	}
};
