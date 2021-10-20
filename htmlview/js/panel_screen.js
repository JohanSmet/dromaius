// js/panel_display.js - Johan Smet - BSD-3-Clause (see LICENSE)
import {Panel} from './panel.js';

export class PanelScreen extends Panel {

	constructor(display_info) {
		super();

		// title
		this.panel_id = 'pnlScreen';
		this.panel_title = 'Screen';

		// canvas
		var canvas = $('<canvas/>', {id: 'display'})
				.attr('width', display_info.width)
				.attr('height', display_info.height)
				.appendTo(this.panel_content)
				[0];

		this.display_context = canvas.getContext("2d");
		this.display_imdata = this.display_context.createImageData(display_info.width, display_info.height);

		// jsPanel
		this.create_js_panel();

		// resizing
		this.panel.options.resizeit.aspectRatio = 'content';
		this.panel.options.resizeit.stop.push((panel, paneldata, event) => this.resize_canvas(canvas));
		this.resize_canvas(canvas);
	}

	update(dmsapi) {
		this.display_imdata.data.set(dmsapi.display_data());
		this.display_context.putImageData(this.display_imdata, 0, 0);
	}

	resize_canvas(canvas) {
		canvas.style.width = (this.panel.content.clientWidth - 10) + 'px';
		canvas.style.height = (this.panel.content.clientHeight - 10) + 'px';
		canvas.style.marginLeft = '5px';
		canvas.style.marginTop = '5px';
	}

	display_context = null;
	display_imdata = null;
}

