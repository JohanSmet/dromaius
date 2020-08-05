// js/editor_view.js - Johan Smet - BSD-3-Clause (see LICENSE)

const BASE_PATH = 'circuits/commodore_pet_2001n/';

const SCHEMATIC_SHEETS = [
	{'page' : 1, 'title': 'Micro-processor / Memory Expansion'},
	{'page' : 2, 'title': 'IEEE-488 Interface'},
	{'page' : 3, 'title': 'Cassette & Keyboard'},
	{'page' : 6, 'title': 'Master Timing'},
	{'page' : 7, 'title': 'Display Logic'},
	{'page' : 8, 'title': 'Display RAMs'}
];

export class CircuitView {
	svg_document = null;
	svg_load_count = 0;
	svg_hovered_signal = '';

	constructor(container) {
		var tabbar = $('<div/>').attr('class', 'tab');
		container.append(tabbar);

		for (const sheet in SCHEMATIC_SHEETS) {
			this.add_sheet(container, tabbar, SCHEMATIC_SHEETS[sheet].page, SCHEMATIC_SHEETS[sheet].title);
		}
	}

	// private functions
	add_sheet(container, tabbar, sheet, title) {
		var name = 'sheet_' + String(sheet).padStart(2, '0');

		var sheet_div = $('<div/>')
			.attr('id', name)
			.attr('class', 'schematic tab_content')
			;

		var svg = $('<object/>')
			.attr('id', name)
			.attr('type', 'image/svg+xml')
			.attr('class', 'schematic')
			.attr('data', BASE_PATH + name + '.svg')
			;

		svg[0].addEventListener('load', () => this.on_svg_loaded());

		container.append(sheet_div);
		sheet_div.append(svg);

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
		var svg_el = $('object' + obj_id + '.schematic')[0];
		this.svg_document = svg_el.getSVGDocument();

		var g = this.svg_document.children[0].children.Schematic.children;
		for (const child in g) {
			if (g[child].id && g[child].id.startsWith("wire#")) {
				// extract signal name
				const end = g[child].id.search('_');
				const name = g[child].id.substr(5, end - 5);

				g[child].addEventListener('mouseover', () => this.on_wire_mouseover(name));
				g[child].addEventListener('mouseout', () => this.on_wire_mouseout(name));
				g[child].addEventListener('click', () => this.on_wire_mouseclick(name));
			}
		}
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

	on_svg_loaded() {
		if (++this.svg_load_count >= SCHEMATIC_SHEETS.length) {
			this.on_tab_button_click($('.tab_button')[0]);
		}
	}

	// public events
	on_signal_hovered = function(signal) {}
	on_signal_clicked = function(signal) {}


}
