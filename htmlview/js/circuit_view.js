// js/editor_view.js - Johan Smet - BSD-3-Clause (see LICENSE)

const BASE_PATH = 'circuits/commodore_pet_2001n/';

const SCHEMATIC_SHEETS = [
	{'page' : 1, 'title': 'Micro-processor / Memory Expansion'},
	{'page' : 2, 'title': 'IEEE-488 Interface'},
	{'page' : 3, 'title': 'Cassette & Keyboard'},
	{'page' : 4, 'title': 'ROMS'},
	{'page' : 5, 'title': 'RAMS'},
	{'page' : 6, 'title': 'Master Timing'},
	{'page' : 7, 'title': 'Display Logic'},
	{'page' : 8, 'title': 'Display RAMs'}
];

export class CircuitView {
	svg_document = null;
	svg_layer = null;
	svg_load_count = 0;
	svg_hovered_signal = '';

	svg_pan_active = false;
	svg_pan_origin = null;
	svg_pan_transform = null;

	constructor(container, lite) {
		var tabbar = $('<div/>').attr('class', 'tab');
		container.append(tabbar);

		this.svg_load_count = (lite) ? 3 : 0;

		for (const sheet in SCHEMATIC_SHEETS) {
			this.add_sheet(container, tabbar, SCHEMATIC_SHEETS[sheet].page, SCHEMATIC_SHEETS[sheet].title, lite);
		}
	}

	// private functions
	add_sheet(container, tabbar, sheet, title, lite) {
		var name = 'sheet_' + String(sheet).padStart(2, '0');

		var sheet_div = $('<div/>')
			.attr('id', name)
			.attr('class', 'schematic tab_content')
			.appendTo(container)
			;

		if (lite && sheet == 6) {
			sheet_div.append($('<span/>')
								.addClass('info')
								.text("The Master Timing components are not emulated by the Pet Lite machine emulator for performance reasons."));
		} else if (lite && sheet >= 7) {
			sheet_div.append($('<span/>')
								.addClass('info')
								.text("The Display Logic components are not emulated by the Pet Lite machine emulator for performance reasons."));
		} else {
			sheet_div.load(BASE_PATH + name + '.svg', () => this.on_svg_loaded(name));
		}

		tabbar.append($('<button/>')
						.attr('class', 'tab_button')
						.attr('id', name)
						.attr('title', title)
						.text('Sheet ' + sheet)
						.on('click', (event) => this.on_tab_button_click(event.target))
		);
	}

	on_tab_button_click(button) {
		var obj_id = '#' + $(button).attr('id');

		// hide all schematics - display the clicked one
		$('.tab_content').hide();
		$('div' + obj_id).show();

		// show selection on the tabbar
		$('.tab_button').removeClass('active');
		$(button).addClass('active');

		// keep reference to selected svg
		this.svg_document = $('div' + obj_id + ' svg')[0];
		this.svg_layer = null;
	}

	on_wire_mouseover(signal) {
		this.svg_hovered_signal = signal;
		this.on_signal_hovered(signal);
	}

	on_wire_mouseout(signal) {
		this.svg_hovered_signal = '';
		this.on_signal_hovered('');
	}

	on_wire_mouseclick(signal) {
		this.on_signal_clicked(signal);
	}

	event_svg_point(svg, evt) {
		var p = svg.createSVGPoint();
		p.x = evt.clientX;
		p.y = evt.clientY;
		return p;
	}

	svg_element_transform(svg_el, transform) {
		var val = "matrix(" + transform.a + "," + transform.b + "," + transform.c + "," + transform.d + "," + transform.e + "," + transform.f + ")";
		svg_el.setAttribute("transform", val);
	}

	svg_transform_layer() {
		if (this.svg_layer === null) {
			this.svg_layer = this.svg_document.getElementById('Schematic');

			if (this.svg_document.getAttribute("viewBox")) {
				this.svg_element_transform(this.svg_layer, this.svg_layer.getCTM());
				this.svg_document.removeAttribute("viewBox");
			}
		}

		return this.svg_layer;
	}

	on_svg_mousewheel(evt) {
		var layer = this.svg_transform_layer();

		const scale = (evt.deltaY < 0) ? 1.2 : 0.8;
		const center = this.event_svg_point(this.svg_document, evt).matrixTransform(layer.getCTM().inverse());
		const scale_transform = this.svg_document.createSVGMatrix()
										.translate(center.x, center.y)
										.scale(scale)
										.translate(-center.x, -center.y);

		this.svg_element_transform(layer, layer.getCTM().multiply(scale_transform));
	}

	on_svg_mousedown(evt) {
		var layer = this.svg_transform_layer();

		this.svg_pan_active = true;
		this.svg_pan_transform = layer.getCTM().inverse();
		this.svg_pan_origin = this.event_svg_point(this.svg_document, evt).matrixTransform(this.svg_pan_transform);
	}

	on_svg_mouseup(evt) {
		if (this.svg_pan_active) {
			this.svg_pan_active = false;
		}
	}

	on_svg_mousemove(evt) {
		if (this.svg_pan_active) {
			var p = this.event_svg_point(this.svg_document, evt).matrixTransform(this.svg_pan_transform);
			var transform = this.svg_pan_transform.inverse().translate(p.x - this.svg_pan_origin.x, p.y - this.svg_pan_origin.y);
			this.svg_element_transform(this.svg_layer, transform);
		}
	}

	on_svg_loaded(name) {

		// add event handlers to the signal wires
		$.each($("div#" + name + " svg g#Schematic [id^='wire#']"), (idx, wire) => {
			// extract signal name
			const end = wire.id.search('_');
			const name = wire.id.substr(5, end - 5);

			wire.addEventListener('mouseover', () => this.on_wire_mouseover(name));
			wire.addEventListener('mouseout', () => this.on_wire_mouseout(name));
			wire.addEventListener('click', () => this.on_wire_mouseclick(name));
		});

		// add pan & zoom handlers
		var svg = $("div#" + name + " svg")[0];
		svg.addEventListener('wheel', (evt) => this.on_svg_mousewheel(evt));
		svg.addEventListener('mousedown', (evt) => this.on_svg_mousedown(evt));
		svg.addEventListener('mouseup', (evt) => this.on_svg_mouseup(evt));
		svg.addEventListener('mousemove', (evt) => this.on_svg_mousemove(evt));

		// if all sheets are loaded: select the first one
		if (++this.svg_load_count >= SCHEMATIC_SHEETS.length) {
			this.on_tab_button_click($('.tab_button')[0]);
		}
	}

	// public events
	on_signal_hovered = function(signal) {}
	on_signal_clicked = function(signal) {}


}
