// js/panel_breakpoints_signal.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelBreakpointsSignal extends Panel {
	dmsapi = null

	constructor(signal_names, dmsapi) {
		super();

		this.panel_id = 'pnlBreakpointsSignal';
		this.panel_title = "Signal Breakpoints";
		this.dmsapi = dmsapi;

		// add new breakpoint
		var selSignal = $('<select/>', {id: 'bps_add', style:'width: 100%'});
		for (var s = 0; s < signal_names.size(); ++s) {
			selSignal.append($('<option/>').attr('value', signal_names.get(s)).text(signal_names.get(s)));
		}

		var btnAdd = $('<button/>', {style: 'width: 100%'})
			.text('Add')
			.on('click', () => {
				this.dmsapi.breakpoint_signal_set(selSignal[0].selectedOptions[0].value);
				this.update();
			})
		;

		$('<table/>')
			.addClass('bps_list')
			.appendTo(this.panel_content)
			.append($('<tr/>')
						.append($('<th/>', {style:'width: 65%'}).append(selSignal))
						.append($('<th/>', {style:'width: 35%'}).append(btnAdd))
			)
		;

		// break points table
		var table = $('<table>', {id: 'bps_table'})
				.addClass('bps_list')
				.appendTo(this.panel_content)
		;

		this.create_js_panel();
	}

	update() {
		var table = $('#bps_table');
		table.children().remove();

		// >> existing breakpoints
		var bps = this.dmsapi.breakpoint_signal_list();

		for (var s = 0; s < bps.size(); ++s) {
			const name = bps.get(s);

			var button = $('<button/>', {style: 'width:100%'})
					.text('Remove')
					.on('click', () => { this.remove_breakpoint(name); })
			;

			table.append($('<tr/>')
					.append($('<td/>').addClass('left').text(name))
					.append($('<td/>').addClass('right').append(button))
				)
			;
		}

		this.panel.resize({height: 'auto'});
	}

	remove_breakpoint(signal_name) {
		this.dmsapi.breakpoint_signal_clear(signal_name);
		this.update();
	}
}
