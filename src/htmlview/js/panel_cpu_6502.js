// js/panel_cpu_6502.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelCpu6502 extends Panel {

	constructor() {
		var create_row = function(label, reg) {
			return $('<tr>')
					.addClass('cpu_register')
					.append($('<td>', { text: label }))
					.append($('<td>', { id: 'reg_' + reg + '_hex' }))
					.append($('<td>', { id: 'reg_' + reg + '_dec' }))
			;
		};

		super();

		this.panel_id = 'pnlCpu6502';
		this.panel_title = 'CPU: MOS 6502';

		// register table
		var table = $('<table>')
				.addClass('cpu_register')
				.appendTo(this.panel_content);
		;

		// >> header
		table.append($('<tr>')
						.append($('<th>', { text: 'Register' }))
						.append($('<th>', { text: 'Hex' }))
						.append($('<th>', { text: 'Dec' }))
		);

		// >> rows
		table.append(create_row('Accumulator', 'a'));
		table.append(create_row('Index-X', 'x'));
		table.append(create_row('Index-Y', 'y'));
		table.append(create_row('Stack Pointer', 'sp'));
		table.append(create_row('Program Counter', 'pc'));
		table.append(create_row('Instruction', 'ir'));

		// cpu status/flags table
		var cpu_status = $('<table>')
				.addClass('cpu_status')
				.appendTo(this.panel_content)
		;

		cpu_status.append($('<tr>')
						.append($('<th>', { text: 'Status' }))
						.append($('<th>', { text: 'N' }))
						.append($('<th>', { text: 'V' }))
						.append($('<th>', { text: '-' }))
						.append($('<th>', { text: 'B' }))
						.append($('<th>', { text: 'D' }))
						.append($('<th>', { text: 'I' }))
						.append($('<th>', { text: 'Z' }))
						.append($('<th>', { text: 'C' }))
		);

		cpu_status.append($('<tr>')
						.append($('<td>', { id: 'reg_p_hex', text: '0' }))
						.append($('<td>', { id: 'reg_p_bit7', text: '0' }))
						.append($('<td>', { id: 'reg_p_bit6', text: '0' }))
						.append($('<td>', { id: 'reg_p_bit5', text: '0' }))
						.append($('<td>', { id: 'reg_p_bit4', text: '0' }))
						.append($('<td>', { id: 'reg_p_bit3', text: '0' }))
						.append($('<td>', { id: 'reg_p_bit2', text: '0' }))
						.append($('<td>', { id: 'reg_p_bit1', text: '0' }))
						.append($('<td>', { id: 'reg_p_bit0', text: '0' }))
		);

		this.create_js_panel();
	}

	update(cpu_info) {
		// helper functions
		var format_reg = function(reg, val, hexlen) {
			$('#reg_' + reg + '_dec').text(val.toString(10));
			$('#reg_' + reg + '_hex').text('$' + ('0000' + val.toString(16)).substr(-hexlen));
		};

		var format_status_bit = function(reg_p, bit) {
			$('#reg_p_bit' + bit).text((reg_p & (1 << bit)) >> bit);
		}

		// registers
		format_reg('a', cpu_info.reg_a, 2);
		format_reg('x', cpu_info.reg_x, 2);
		format_reg('y', cpu_info.reg_y, 2);
		format_reg('sp', cpu_info.reg_sp, 2);
		format_reg('pc', cpu_info.reg_pc, 4);
		format_reg('ir', cpu_info.reg_ir, 2);

		// status
		format_reg('p', cpu_info.reg_p, 2);

		for (var i = 0; i < 8; ++i) {
			format_status_bit(cpu_info.reg_p, i);
		}

	}
}
