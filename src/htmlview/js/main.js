
// constants
const API_NUMBER = 'number';
const API_STRING = 'string';

// api functions
var dmsapi_create_context = null;
var dmsapi_release_context = null;
var dmsapi_launch_commodore_pet = null;
var dmsapi_context_execute = null;
var dmsapi_context_step = null;
var dmsapi_context_run = null;
var dmsapi_context_pause = null;
var dmsapi_context_reset = null;

var dmsapi_cpu_6502_info = null;
var dmsapi_display_info = null;
var dmsapi_free_json = null;
var dmsapi_display_data = null;

// globals
var dmsapi_context = null;
var display_sharmem = null
var display_context = null;
var display_imdata = null;

// functions
function wasm_wrap_functions() {
	dmsapi_create_context = Module.cwrap('dmsapi_create_context', API_NUMBER, []);
	dmsapi_release_context = Module.cwrap('dmsapi_release_context', null, [API_NUMBER]);
	dmsapi_launch_commodore_pet = Module.cwrap('dmsapi_launch_commodore_pet', null, [API_NUMBER]);
	dmsapi_context_execute = Module.cwrap('dmsapi_context_execute', null, [API_NUMBER]);
	dmsapi_context_step = Module.cwrap('dmsapi_context_step', null, [API_NUMBER]);
	dmsapi_context_run = Module.cwrap('dmsapi_context_run', null, [API_NUMBER]);
	dmsapi_context_pause = Module.cwrap('dmsapi_context_pause', null, [API_NUMBER]);
	dmsapi_context_reset = Module.cwrap('dmsapi_context_reset', null, [API_NUMBER]);

	dmsapi_cpu_6502_info = Module.cwrap('dmsapi_cpu_6502_info', API_STRING, [API_NUMBER]);
	dmsapi_display_info = Module.cwrap('dmsapi_display_info', API_STRING, [API_NUMBER]);
	dmsapi_free_json = Module.cwrap('dmsapi_free_json', null, [API_STRING]);

	dmsapi_display_data = Module.cwrap('dmsapi_display_data', null, [API_NUMBER, API_NUMBER]);
}

function allocate_shared_memory(size) {
	var offset = Module._malloc(size);
	Module.HEAP8.set(new Uint8Array(size), offset);
	return {
		"data" : Module.HEAPU8.subarray(offset, offset + size),
		"offset": offset
	}
}

function parse_json(wasm_str) {
	try {
		var obj = JSON.parse(wasm_str);
		dmsapi_free_json(wasm_str);
		return obj;
	} catch (err) {
		console.error(err);
		console.log(wasm_str);
	}
}

function setup_emulation() {
	dmsapi_context = dmsapi_create_context();
	dmsapi_launch_commodore_pet(dmsapi_context);

	// setup shared memory and image-data for the monitor display
	var display_info = parse_json(dmsapi_display_info(dmsapi_context));
	display_sharmem = allocate_shared_memory(display_info.width * display_info.height * 4);

	display_context = $('#display')[0].getContext("2d");
	display_imdata = display_context.createImageData(display_info.width, display_info.height);

	// resize canvas
	$('#display')[0].width = display_info.width;
	$('#display')[0].height = display_info.height;

	setInterval(execution_timer, 1000 / 25);
}

function execution_timer() {
	dmsapi_context_execute(dmsapi_context);

	// refresh the screen
	dmsapi_display_data(dmsapi_context, display_sharmem.offset);
	display_imdata.data.set(display_sharmem.data);
	display_context.putImageData(display_imdata, 0, 0);
}

$('#btnStep').on('click', function (event) {
	dmsapi_context_step(dmsapi_context);
});

$('#btnRun').on('click', function (event) {
	dmsapi_context_run(dmsapi_context);
});

$('#btnPause').on('click', function (event) {
	dmsapi_context_pause(dmsapi_context);
});

$('#btnReset').on('click', function (event) {
	dmsapi_context_reset(dmsapi_context);
});
