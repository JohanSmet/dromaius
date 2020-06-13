// js/panel_clock.js - Johan Smet - BSD-3-Clause (see LICENSE)

class PanelClock {
	constructor(container) {
		// title
		container.append($("<h2 />", { text: "Clock" }));

		// value  table
		var table = $('<table>').addClass('panel_info');

		// >> output
		table.append($('<tr>')
						.append($('<th>', { text: 'Output: ' }))
						.append($('<td>', { id: 'clock_output', style: 'width:5em' }))
		);

		// >> cycle_count
		table.append($('<tr>')
						.append($('<th>', { text: 'Cycle: ' }))
						.append($('<td>', { id: 'clock_cycle_count' }))
		);

		container.append(table);
	}

	update(clock_info) {

		var format_status_bit = function(reg_p, bit) {
			$('#reg_p_bit' + bit).text((reg_p & (1 << bit)) >> bit);
		}

		$('#clock_output').text(clock_info.output);
		$('#clock_cycle_count').text(clock_info.cycle_count);
	}
}
