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
				.attr('style', 'width: 320px; height:200px; margin-left: 2px; margin-top: 4px')
				.appendTo(this.panel_content)
				[0];

		this.display_context = canvas.getContext("2d");
		this.display_imdata = this.display_context.createImageData(display_info.width, display_info.height);
		this.display_info = display_info;

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

		var scale_x = (this.panel.content.clientWidth - 8) / this.display_info.width;
		var scale_y = (this.panel.content.clientHeight - 8) / this.display_info.height;
		var scale = Math.min(scale_x, scale_y);

		var width = this.display_info.width * scale;
		var height = this.display_info.height * scale;

		canvas.style.width = width + 'px';
		canvas.style.height = height + 'px';
		canvas.style.marginLeft = (this.panel.content.clientWidth - width) / 2 + 'px';
		canvas.style.marginTop = (this.panel.content.clientHeight - height) / 2 + 'px';
	}

	display_context = null;
	display_imdata = null;
	display_info = null;
}

