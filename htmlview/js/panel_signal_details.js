// js/panel_signal_details.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelSignalDetails extends Panel {

	constructor(dmsapi) {
		super();

		this.dmsapi = dmsapi
		this.panel_id = 'pnlSignalDetails';
		this.panel_title = 'Signal';

		// no data
		var no_data = $('<p/>')
			.text('Click on a wire to display signal information')
			.attr('id', 'no_data')
			.attr('style', 'text-align: center')
			.addClass('hidden')
			.appendTo(this.panel_content)
		;

		// data
		var data_div = $('<div/>')
						.attr('id', 'data')
						.appendTo(this.panel_content)
		;

		var table = $('<table/>')
						.addClass('panel_info')
						.appendTo(data_div)
		;

		table.append($('<tr/>')
						.append($('<th/>', { text: 'Name' }))
						.append($('<td/>', { id: 'signal_name' }))
		);
		table.append($('<tr/>')
						.append($('<th/>', { text: 'Value' }))
						.append($('<td/>', { id: 'signal_value' }))
		);
		table.append($('<tr/>')
						.append($('<th/>', { text: 'Last written by' }))
						.append($('<td/>', { id: 'signal_writer' }))
		);

		$('<button/>', {text: 'Set breakpoint'})
			.addClass('panel_button')
			.appendTo(data_div)
			.on('click', () => {
				this.dmsapi.breakpoint_signal_set(this.current);
				this.on_breakpoint_set();
			}
		);

		$('<button/>', {text: 'Refresh'})
			.addClass('panel_button')
			.appendTo(data_div)
			.on('click', () => {
				this.update(this.current);
			}
		);

		this.create_js_panel();

		data_div.addClass('hidden');
		no_data.removeClass('hidden');

	}

	update(signal_name) {
		const format_bool = function(val) {
			const str = ['LOW', 'HIGH'];
			return str[val];
		};

		const format_hex = function(val, hexlen) {
			return '$' + ('0000' + val.toString(16)).substr(-hexlen);
		};

		this.current = signal_name;
		const details = this.dmsapi.signal_details(signal_name);

		// visibility of data
		if (details.signal > 0) {
			this.panel_content.children('#data').removeClass('hidden');
			this.panel_content.children('#no_data').addClass('hidden');
		} else {
			this.panel_content.children('#data').addClass('hidden');
			this.panel_content.children('#no_data').removeClass('hidden');
			return;
		}

		this.panel_content.find('#signal_name').text(signal_name);
		this.panel_content.find('#signal_writer').text(details.writer_name);

		this.panel_content.find('#signal_value').text(format_bool(details.value));
	}

	// public callbacks
	on_breakpoint_set = function() {}
}
