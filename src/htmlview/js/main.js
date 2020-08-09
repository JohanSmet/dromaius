// js/main.js - Johan Smet - BSD-3-Clause (see LICENSE)

import {PanelCpu6502} from './panel_cpu_6502.js';
import {PanelClock} from './panel_clock.js';
import {CircuitView} from './circuit_view.js';

// globals
var dmsapi = null;
var display_context = null;
var display_imdata = null;

var signals_names = [];
var hovered_signal = '';

var panel_cpu = new PanelCpu6502($('div#cpu'));
var panel_clock = new PanelClock($('div#clock'));
var circuit_view = new CircuitView($('div#right_column'));

// functions
export function setup_emulation(emscripten_mod) {
	dmsapi = new emscripten_mod.DmsApi();

	dmsapi.launch_commodore_pet();

	// signals
	var signal_info = dmsapi.signal_info();

	// >> initialize empty name for all signals
	for (var i = 0; i < signal_info.count; ++i) {
		signals_names.push('');
	}

	// >> insert named signals
	var sigkeys = signal_info.names.keys();

	for (var s = 0; s < sigkeys.size(); ++s) {
		var sig_name = sigkeys.get(s);
		var signal = signal_info.names.get(sig_name);

		if (signal.count == 1) {
			signals_names[signal.start] = '--color-' + sig_name.replace('/', 'bar');
		}
	}

	// setup image-data for the monitor display
	var display_info = dmsapi.display_info();

	display_context = $('#display')[0].getContext("2d");
	display_imdata = display_context.createImageData(display_info.width, display_info.height);

	// resize canvas
	$('#display')[0].width = display_info.width;
	$('#display')[0].height = display_info.height;

	// signal selection
	circuit_view.on_signal_hovered = function(signal) {
		hovered_signal = '--color-' + signal.replace('/', 'bar');
	}

	circuit_view.on_signal_clicked = function(signal) {
		alert(signal);
	}

	// refresh
	setInterval(execution_timer, 1000 / 25);
}

function refresh_signals() {
	if (circuit_view.svg_document == null) {
		return;
	}

	const colors = ["#009900", "#00FF00"];
	var svg_style = circuit_view.svg_document.documentElement.style;

	var sig_data = dmsapi.signal_data();

	for (var s_id = 0; s_id < sig_data.length; ++s_id) {
		var s_name = signals_names[s_id];
		svg_style.setProperty(s_name, colors[sig_data[s_id]]);
	}

	if (hovered_signal != '') {
		svg_style.setProperty(hovered_signal, 'blue');
	}
}

function refresh_cpu_panel() {
	var cpu_info = dmsapi.cpu_info();
	panel_cpu.update(cpu_info);
}

function refresh_clock_panel() {
	var clock_info = dmsapi.clock_info();
	panel_clock.update(clock_info);
}

function execution_timer() {
	dmsapi.context_execute();

	refresh_signals();
	refresh_cpu_panel();
	refresh_clock_panel();

	// refresh the screen
	display_imdata.data.set(dmsapi.display_data());
	display_context.putImageData(display_imdata, 0, 0);
}

$('#btnStepInstruction').on('click', function (event) {
	dmsapi.context_step_instruction();
});

$('#btnStepClock').on('click', function (event) {
	dmsapi.context_step_clock();
});

$('#btnRun').on('click', function (event) {
	dmsapi.context_run();
});

$('#btnPause').on('click', function (event) {
	dmsapi.context_pause();
});

$('#btnReset').on('click', function (event) {
	dmsapi.context_reset();
});

$("#btnAboutOpen").on("click", function (event) {
	$("#about").show();
});

$("#btnAboutClose").on("click", function (event) {
	$("#about").hide();
});
