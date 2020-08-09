// js/panel_clock.js - Johan Smet - BSD-3-Clause (see LICENSE)

export class PanelClock {
	constructor(container) {
		// title
		container.append($("<h2 />", { text: "Clock" }));

		// value  table
		var table = $('<table>').addClass('panel_info');

		// >> tick count
		table.append($('<tr>')
						.append($('<th>', { text: 'Current Tick: ' }))
						.append($('<td>', { id: 'clock_current_tick', style: 'width:10em' }))
		);

		// >> time elapsed since beginning
		table.append($('<tr>')
						.append($('<th>', { text: 'Time: ' }))
						.append($('<td>', { id: 'clock_elapsed' }))
		);

		container.append(table);
	}

	update(clock_info) {

		$('#clock_current_tick').text(clock_info.current_tick);

		if (clock_info.time_elapsed_ns > 1000000) {
			$('#clock_elapsed').text((clock_info.time_elapsed_ns / 1000000).toFixed(6) + ' ms');
		} else if (clock_info.time_elapsed_ns > 1000) {
			$('#clock_elapsed').text((clock_info.time_elapsed_ns / 1000).toFixed(6) + ' µs');
		} else {
			$('#clock_elapsed').text(clock_info.time_elapsed_ns.toFixed(2) + ' ns');
		}
	}
}
