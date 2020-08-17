// js/panel_signal_details.js - Johan Smet - BSD-3-Clause (see LICENSE)

export class PanelSignalDetails {
	dmsapi = null;
	current = '';

	constructor(container, dmsapi) {
		this.dmsapi = dmsapi;

		// title
		container.append($("<h2 />", { text: "Signal" }));

		var table = $('<table/>')
						.addClass('panel_info')
						.appendTo(container);

		table.append($('<tr/>')
						.append($('<th/>', { text: 'Name' }))
						.append($('<td/>', { id: 'signal_name' }))
		);
		table.append($('<tr/>')
						.append($('<th/>', { text: 'Last written by' }))
						.append($('<td/>', { id: 'signal_writer' }))
		);

		$('<button/>', {text: 'Set breakpoint'}).appendTo(container).on('click', () => {
			this.dmsapi.breakpoint_signal_set(this.current);
			this.on_breakpoint_set();
		});
	}

	update(signal_name) {
		this.current = signal_name;
		const details = this.dmsapi.signal_details(signal_name);

		$('#signal_name').text(signal_name);
		$('#signal_writer').text(details.writer_name);
	}

	// public events
	on_breakpoint_set = function() {}
}
