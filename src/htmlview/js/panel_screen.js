// js/panel_display.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelScreen extends Panel {

	constructor(display_info) {
		super();

		// title
		this.panel_id = 'pnlScreen';
		this.panel_title = 'Screen';

		// canvas
		var canvas = $('<canvas/>', {id: 'display', width: display_info.width, height:display_info.height})
				.appendTo(this.panel_content)
				[0];

		this.display_context = canvas.getContext("2d");
		this.display_imdata = this.display_context.createImageData(display_info.width, display_info.height);

		this.create_js_panel();
	}

	update(dmsapi) {
		this.display_imdata.data.set(dmsapi.display_data());
		this.display_context.putImageData(this.display_imdata, 0, 0);
	}

	display_context = null;
	display_imdata = null;
}

