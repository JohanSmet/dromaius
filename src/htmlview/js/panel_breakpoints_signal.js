// js/panel_breakpoints_signal.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelBreakpointsSignal extends Panel {
	dmsapi = null

	constructor(signal_names, dmsapi) {
		super();

		// title
		this.panel_id = 'pnlBreakpointsSignal';
		this.panel_title = "Signal Breakpoints";

		// break points table
		var table = $('<table>', {id: 'bps_table'})
				.addClass('panel_info')
				.appendTo(this.panel_content)
		;

		// add new breakpoint
		var div = $('<div/>', {style: 'margin-top:.5em'}).appendTo(this.panel_content);

		var combo = $('<select/>', {id: 'bps_add', style:'margin:.5em'}).appendTo(div);

		for (var s = 0; s < signal_names.size(); ++s) {
			combo.append($('<option/>').attr('value', signal_names.get(s)).text(signal_names.get(s)));
		}

		$('<button/>', {text: 'Add'}).appendTo(div).on('click', () => {
			this.dmsapi.breakpoint_signal_set(($('select#bps_add').children('option:selected').val()));
			this.update();
		});

		this.dmsapi = dmsapi;

		this.create_js_panel();
	}

	update() {
		var table = $('#bps_table');
		table.children().remove();

		// >> existing breakpoints
		var bps = this.dmsapi.breakpoint_signal_list();

		for (var s = 0; s < bps.size(); ++s) {
			table.append($('<tr>')
							.append($('<td>', { text: bps.get(s) }))
							.append($('<td>', { text: 'Remove', signal: bps.get(s) }).addClass('actionRemove'))
							);
		}

		$('table#bps_table td.actionRemove').on('click', (e) => this.remove_breakpoint($(e.target).attr('signal')));
	}

	remove_breakpoint(signal_name) {
		this.dmsapi.breakpoint_signal_clear(signal_name);
		this.update();
	}
}
