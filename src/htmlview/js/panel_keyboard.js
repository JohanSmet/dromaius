// js/panel_keyboard.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

const BASE_PATH = 'circuits/commodore_pet_2001n/';

export class PanelKeyboard extends Panel {

	constructor(dmsapi) {
		super();

		this.panel_id = 'pnlKeyboard';
		this.panel_title = 'Keyboard';
		this.dmsapi = dmsapi
		this.shift_locked = false;

		this.ascii_send_text = '';
		this.ascii_send_index = 0;
		this.ascii_send_delay = 0;

		this.panel_content.css('padding', '2px');

		var div_keys = $('<div/>')
			.appendTo(this.panel_content)
		;

		$('<span/>')
			.addClass('info')
			.text('Text input: ')
			.appendTo(this.panel_content)
		;

		$('<input/>')
			.attr('id', 'txtAsciiKeys')
			.attr('autocomplete', 'on')
			.css('width', '75%')
			.appendTo(this.panel_content)
			.on("keypress", (key) => {
				if (key.charCode == 13) {
					this.ascii_send_text = $('#txtAsciiKeys').val() + '\n';
					$('#txtAsciiKeys').val('');

					this.ascii_send_index = 0;
					this.ascii_send_delay = 0;
				}
			})
		;

		div_keys.load(BASE_PATH + 'pet_keyboard.svg', () => {
			this.parse_keyboard();
			this.panel.resize({width: '750px', height: 'auto'});
		});

		this.create_js_panel();

		this.dmsapi.keyboard_set_dwell_time(75);
	}

	parse_keyboard() {
		$.each(this.panel_content.find(" svg g#Keyboard [id^='key_']"), (idx, key) => {
			// parse id
			let [prefix, row, col] = key.id.split('_');

			key.keyboard_row = row;
			key.keyboard_col = col;

			// events
			key.addEventListener('click', () => this.key_pressed(key));
		});
	}

	update() {
		const keys_down = this.dmsapi.keyboard_keys_down();

		this.panel_content.find(" svg g#Keyboard [id^='key_']").removeClass('pressed');

		for (var i = 0; i < keys_down.size(); ++i) {
			const k = keys_down.get(i);
			this.panel_content.find('path#key_' + k.row + '_' + k.column).addClass('pressed');
		}

		if (this.shift_locked) {
			this.panel_content.find("svg g#Keyboard #key_8_0_lock").addClass('pressed');
			this.dmsapi.keyboard_key_pressed(8,0);
		}

		if (this.ascii_send_index < this.ascii_send_text.length &&
			this.dmsapi.keyboard_num_keys_down() == 0) {

			this.ascii_send_delay -= 1;
			if (this.ascii_send_delay < 0) {
				const key = this.KEY_ASCII_TO_MATRIX[this.ascii_send_text[this.ascii_send_index]];

				if (key !== undefined) {
					this.dmsapi.keyboard_key_pressed(key[0], key[1]);
					this.ascii_send_delay = 5;
				}
				this.ascii_send_index += 1;
			}
		}
	}

	key_pressed(key) {
		if (key.id == "key_8_0_lock") {
			this.shift_locked = !this.shift_locked;
		} else {
			this.dmsapi.keyboard_key_pressed(parseInt(key.keyboard_row), parseInt(key.keyboard_col));
		}
	}

	KEY_ASCII_TO_MATRIX = {
		'!'  : [0, 0],
		'#'  : [0, 1],
		'%'  : [0, 2],
		'&'  : [0, 3],
		'('  : [0, 4],
		'"'  : [1, 0],
		'$'  : [1, 1],
		'\'' : [1, 2],
		'\\' : [1, 3],
		')'  : [1, 4],

		'q'  : [2, 0],
		'e'  : [2, 1],
		't'  : [2, 2],
		'u'  : [2, 3],
		'o'  : [2, 4],
		'7'  : [2, 6],
		'9'  : [2, 7],

		'w'  : [3, 0],
		'r'  : [3, 1],
		'y'  : [3, 2],
		'i'  : [3, 3],
		'p'  : [3, 4],
		'8'  : [3, 6],
		'/'  : [3, 7],

		'a'  : [4, 0],
		'd'  : [4, 1],
		'g'  : [4, 2],
		'j'  : [4, 3],
		'l'  : [4, 4],
		'4'  : [4, 6],
		'6'  : [4, 7],

		's'  : [5, 0],
		'f'  : [5, 1],
		'h'  : [5, 2],
		'k'  : [5, 3],
		':'  : [5, 4],
		'5'  : [5, 6],
		'*'  : [5, 7],

		'z'  : [6, 0],
		'c'  : [6, 1],
		'b'  : [6, 2],
		'm'  : [6, 3],
		';'  : [6, 4],
		'\n' : [6, 5],
		'1'  : [6, 6],
		'3'  : [6, 7],

		'x'  : [7, 0],
		'v'  : [7, 1],
		'n'  : [7, 2],
		','  : [7, 3],
		'?'  : [7, 4],
		'2'  : [7, 6],
		'+'  : [7, 7],

		'@'  : [8, 1],
		']'  : [8, 2],
		'>'  : [8, 4],
		'0'  : [8, 6],
		'-'  : [8, 7],

		'['  : [9, 1],
		' '  : [9, 2],
		'<'  : [9, 4],
		'.'  : [9, 6],
		'='  : [9, 7],
	};

}
