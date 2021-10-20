// js/panel_clock.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelClock extends Panel {

	constructor() {
		super();

		this.panel_id = 'pnlClock';
		this.panel_title = 'Clock';

		// value  table
		var table = $('<table>')
						.addClass('panel_info')
						.appendTo(this.panel_content)
		;

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

		this.create_js_panel();
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
