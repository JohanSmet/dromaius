// js/panel_datassette.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelDatassette extends Panel {

	constructor(dmsapi) {
		super();

		this.panel_id = 'pnlDatassette';
		this.panel_title = 'Datassette 1530';
		this.dmsapi = dmsapi

		$('<input/>')
			.attr('type', 'file')
			.attr('id', 'tap_file')
			.attr('accept', '.tap')
			.appendTo(this.panel_content)
			.on('change', (e) => {
				var file = e.target.files[0];

				var reader = new FileReader();
				reader.onload = (e) => {
					this.dmsapi.datassette_load_tap(e.target.result);
				};
				reader.readAsArrayBuffer(file);
			})
		;

		$('<div/>')
			.attr('id', 'ds_status')
			.text('Idle')
			.appendTo(this.panel_content)
		;

		var button_div = $('<div/>')
							.appendTo(this.panel_content)
		;

		$('<button/>')
			.text('Play')
			.appendTo(button_div)
			.on('click', () => { this.dmsapi.datassette_play(); })
		;

		$('<button/>')
			.text('Stop')
			.appendTo(button_div)
			.on('click', () => { this.dmsapi.datassette_stop(); })
		;

		this.create_js_panel();
	}

	update() {
		$("div#ds_status").text(this.dmsapi.datassette_status());
	}
}
