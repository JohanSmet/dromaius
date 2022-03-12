// js/panel_disk2031.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelDisk2031 extends Panel {

	constructor(dmsapi) {
		super();

		this.panel_id = 'pnlDisk2031';
		this.panel_title = 'Floppy Disk 2031';
		this.dmsapi = dmsapi
		this.prev_status = '';

		var input_div = $('<div/>')
							.appendTo(this.panel_content)
		;

		var file_input = $('<input/>')
			.attr('type', 'file')
			.attr('id', 'd64_file')
			.attr('accept', '.d64')
			.addClass('hidden')
			.appendTo(input_div)
			.on('change', (e) => {
				var file = e.target.files[0];
				$('span#lblFile').text('(' + file.name + ')');

				var reader = new FileReader();
				reader.onload = (e) => {
					this.dmsapi.disk2031_load_d64(e.target.result);
				};
				reader.readAsArrayBuffer(file);
			})
		;

		$('<button/>')
			.text('Upload a local .d64 file')
			.addClass('panel_button')
			.appendTo(input_div)
			.on('click', () => {file_input.click();})
		;

		$('<span/>')
			.attr('id', 'lblFile')
			.addClass('info')
			.text('(no .d64 loaded)')
			.appendTo(input_div)
		;

		var config_div = $('<div/>')
							.appendTo(this.panel_content)
		;

		$('<span/>')
			.text('Address: ')
			.addClass('info')
			.addClass('left')
			.appendTo(config_div)
		;

		var selAddr = $('<select/>')
			.attr('id', 'd2031_addr')
			.attr('style', 'width: 3em; margin-left: 0.5em')
			.appendTo(config_div)
			.append($('<option/>').attr('value', 8).text('8'))
			.append($('<option/>').attr('value', 9).text('9'))
			.on('change', (e) => {
				this.dmsapi.disk2031_set_address(parseInt(selAddr[0].selectedOptions[0].value));
			})
		;

		this.create_js_panel();
	}

	update() {
	}
}
