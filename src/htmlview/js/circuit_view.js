// js/editor_view.js - Johan Smet - BSD-3-Clause (see LICENSE)

const BASE_PATH = 'circuits/commodore_pet_2001n/';

export class CircuitView {
	constructor(container) {
		var tabbar = $('<div/>').attr('class', 'tab');
		container.append(tabbar);

		this.add_sheet(container, tabbar, 1, 'Micro-processor / Memory Expansion');
		this.add_sheet(container, tabbar, 2, 'IEEE-488 Interface');
		this.add_sheet(container, tabbar, 3, 'Cassette & Keyboard');
		this.add_sheet(container, tabbar, 6, 'Master Timing');
		this.add_sheet(container, tabbar, 7, 'Display Logic');
		this.add_sheet(container, tabbar, 8, 'Display RAMs');
	}

	add_sheet(container, tabbar, sheet, title) {
		var name = 'sheet_' + String(sheet).padStart(2, '0');

		var sheet_div = $('<div/>')
			.attr('id', name)
			.attr('class', 'schematic tab_content');

		var svg = $('<object/>')
			.attr('id', name)
			.attr('type', 'image/svg+xml')
			.attr('class', 'schematic')
			.attr('data', BASE_PATH + name + '.svg');

		container.append(sheet_div);
		sheet_div.append(svg);

		tabbar.append($('<button/>')
						.attr('class', 'tab_button')
						.attr('id', name)
						.attr('title', title)
						.text('Sheet ' + sheet)
		);
	}
}
