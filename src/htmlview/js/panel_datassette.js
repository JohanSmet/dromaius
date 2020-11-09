// js/panel_datassette.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelDatassette extends Panel {

	constructor(dmsapi) {
		super();

		this.panel_id = 'pnlDatassette';
		this.panel_title = 'Datassette 1530';
		this.dmsapi = dmsapi
		this.prev_status = '';

		var input_div = $('<div/>')
							.appendTo(this.panel_content)
		;

		var file_input = $('<input/>')
			.attr('type', 'file')
			.attr('id', 'tap_file')
			.attr('accept', '.tap')
			.addClass('hidden')
			.appendTo(input_div)
			.on('change', (e) => {
				var file = e.target.files[0];
				$('span#lblFile').text('(' + file.name + ')');

				var reader = new FileReader();
				reader.onload = (e) => {
					this.dmsapi.datassette_load_tap(e.target.result);
				};
				reader.readAsArrayBuffer(file);
			})
		;

		$('<button/>')
			.text('Upload a local .tap file')
			.addClass('panel_button')
			.appendTo(input_div)
			.on('click', () => {file_input.click();})
		;

		$('<span/>')
			.attr('id', 'lblFile')
			.addClass('info')
			.text('(no .tap loaded)')
			.appendTo(input_div)
		;

		var status_div = $('<div/>')
			.appendTo(this.panel_content)
		;

		$('<span/>')
			.text('Status: ')
			.addClass('info')
			.addClass('left')
			.appendTo(status_div)
		;

		$('<span/>')
			.attr('id', 'ds_status')
			.text('Idle')
			.addClass('info')
			.appendTo(status_div)
		;

		$('<span/>')
			.attr('id', 'ds_position')
			.text('Counter: 000')
			.addClass('info')
			.addClass('right')
			.appendTo(status_div)
		;

		var button_div = $('<div/>')
							.appendTo(this.panel_content)
		;

		this.buttons = []

		this.buttons.push($('<button/>')
			.text('Rec')
			.addClass('ds_button')
			.appendTo(button_div)
			.on('click', () => { this.dmsapi.datassette_record(); })
		);

		this.buttons.push($('<button/>')
			.text('Play')
			.addClass('ds_button')
			.appendTo(button_div)
			.on('click', () => { this.dmsapi.datassette_play(); })
		);

		this.buttons.push($('<button/>')
			.text('Rew')
			.addClass('ds_button')
			.appendTo(button_div)
			.on('click', () => { this.dmsapi.datassette_rewind(); })
		);

		this.buttons.push($('<button/>')
			.text('FFwd')
			.addClass('ds_button')
			.appendTo(button_div)
			.on('click', () => { this.dmsapi.datassette_fast_forward(); })
		);

		this.buttons.push($('<button/>')
			.text('Stop')
			.addClass('ds_button')
			.appendTo(button_div)
			.on('click', () => { this.dmsapi.datassette_stop(); })
		);

		this.buttons.push($('<button/>')
			.text('Eject')
			.addClass('ds_button')
			.appendTo(button_div)
			.on('click', () => { this.dmsapi.datassette_eject(); })
		);

		this.create_js_panel();
	}

	update() {
		const ds_status = this.dmsapi.datassette_status();
		if (ds_status != this.prev_status) {
			$("span#ds_status").text(ds_status);

			const valid_buttons = this.dmsapi.datassette_valid_buttons();

			$.each(this.buttons, function (idx, btn) {
				const button = 1 << idx;

				if ((valid_buttons & button) == button) {
					btn.removeAttr('disabled');
				} else {
					btn.attr('disabled', '');
				}
			});

			this.prev_status = ds_status;
		}

		$('#ds_position').text('Counter: ' + ('000' + this.dmsapi.datassette_position()).substr(-3));
	}
}
