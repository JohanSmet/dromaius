// globals
var dmsapi = null;
var display_context = null;
var display_imdata = null;

var signals_names = [];

var svg_doc = null;

var panel_cpu = new PanelCpu6502($('div#cpu'));

// functions
function setup_svg_document() {
	// FIXME: support more than 1 page
	var svg_el = document.getElementById('schematic_page');
	svg_doc = svg_el.getSVGDocument();
}

function setup_emulation() {
	dmsapi = new Module.DmsApi();

	dmsapi.launch_commodore_pet();

	// svg
	setup_svg_document();

	// signals
	var signal_info = dmsapi.signal_info();

	// >> initialize empty name for all signals
	for (var d = 0; d < signal_info.count.size(); ++d) {
		signals_names.push([]);
		for (var i = 0; i < signal_info.count.get(d); ++i) {
			signals_names[d].push('');
		}
	}

	// >> insert named signals
	var sigkeys = signal_info.names.keys();

	for (var s = 0; s < sigkeys.size(); ++s) {
		var sig_name = sigkeys.get(s);
		var signal = signal_info.names.get(sig_name);

		if (signal.count == 1) {
			signals_names[signal.domain][signal.start] = '--color-' + sig_name.replace('/', 'bar');
		}
	}

	// setup image-data for the monitor display
	var display_info = dmsapi.display_info();

	display_context = $('#display')[0].getContext("2d");
	display_imdata = display_context.createImageData(display_info.width, display_info.height);

	// resize canvas
	$('#display')[0].width = display_info.width;
	$('#display')[0].height = display_info.height;

	setInterval(execution_timer, 1000 / 25);
}

function refresh_signals() {
	var sig_data = dmsapi.signal_data(0);
	var colors = ["#009900", "#00FF00"];

	for (var s_id = 0; s_id < sig_data.length; ++s_id) {
		var s_name = signals_names[0][s_id];
		svg_doc.documentElement.style.setProperty(s_name, colors[sig_data[s_id]]);
	}
}

function refresh_cpu_panel() {
	var cpu_info = dmsapi.cpu_info();
	panel_cpu.update(cpu_info);
}

function execution_timer() {
	dmsapi.context_execute();

	refresh_signals();
	refresh_cpu_panel();

	// refresh the screen
	display_imdata.data.set(dmsapi.display_data());
	display_context.putImageData(display_imdata, 0, 0);
}

$('#btnStep').on('click', function (event) {
	dmsapi.context_step();
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
