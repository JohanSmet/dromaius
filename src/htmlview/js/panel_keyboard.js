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

		this.panel_content.css('padding', '2px');

		this.panel_content.load(BASE_PATH + 'pet_keyboard.svg', () => {
			this.parse_keyboard();
			this.panel.resize({width: '750px', height: 'auto'});
		});

		this.create_js_panel();
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
	}

	key_pressed(key) {
		if (key.id == "key_8_0_lock") {
			this.shift_locked = !this.shift_locked;
		} else {
			this.dmsapi.keyboard_key_pressed(parseInt(key.keyboard_row), parseInt(key.keyboard_col));
		}
	}
}
