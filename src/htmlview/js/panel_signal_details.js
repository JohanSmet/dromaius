// js/panel_signal_details.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelSignalDetails extends Panel {

	constructor(dmsapi) {
		super();

		this.dmsapi = dmsapi
		this.panel_id = 'pnlSignalDetails';
		this.panel_title = 'Signal';

		// data
		var table = $('<table/>')
						.addClass('panel_info')
						.appendTo(this.panel_content)
		;

		table.append($('<tr/>')
						.append($('<th/>', { text: 'Name' }))
						.append($('<td/>', { id: 'signal_name' }))
		);
		table.append($('<tr/>')
						.append($('<th/>', { text: 'Last written by' }))
						.append($('<td/>', { id: 'signal_writer' }))
		);

		$('<button/>', {text: 'Set breakpoint'})
			.appendTo(this.panel_content)
			.on('click', () => {
				this.dmsapi.breakpoint_signal_set(this.current);
				this.on_breakpoint_set();
			}
		);

		this.create_js_panel();
	}

	update(signal_name) {
		this.current = signal_name;
		const details = this.dmsapi.signal_details(signal_name);

		$('#signal_name').text(signal_name);
		$('#signal_writer').text(details.writer_name);
	}

	// public callbacks
	on_breakpoint_set = function() {}
}
