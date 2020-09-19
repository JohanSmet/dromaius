// js/panel.js - Johan Smet - BSD-3-Clause (see LICENSE)

import { jsPanel } from './jspanel/jspanel.js';

export class Panel {

	panel_title = "No Title";
	panel_content = $('<div/>');
	panel = null;

	create_js_panel() {
		this.panel = jsPanel.create({
			headerTitle: this.panel_title,
			headerControls : {
				maximize: 'remove',
				close: 'remove',
			},
			dragit: {
				grid: [20, 20],
			},
			contentSize:'350px auto',
			position: 'left-top 0 15 down div#content',
			content: this.panel_content[0],
			theme: 'dark'
		});
	}
}
