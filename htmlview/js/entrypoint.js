// js/entrypoint.js - Johan Smet - BSD-3-Clause (see LICENSE)

var Module = {

	print: function(text) {
		if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
        console.log(text);
	},

	printErr: function(text) {
		if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
        console.error(text);
	},

	locateFile: (path, prefix) => {
		if (path.endsWith(".data")) {
			return prefix + 'wasm/' + path;
		}
		return prefix + path;
	},

	statusElement : (() => document.getElementById('status'))(),
	progressElement : (() => document.getElementById('progress'))(),
	spinnerElement : (() => document.getElementById('spinner'))(),

	setStatus: function(text) {
		if (!this.setStatus.last)
			this.setStatus.last = { time: Date.now(), text: '' };
		if (text === this.setStatus.last.text)
			return;

		var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
		var now = Date.now();
		if (m && now - this.setStatus.last.time < 30)
			return; // if this is a progress update, skip it if too soon

		this.setStatus.last.time = now;
		this.setStatus.last.text = text;

		if (m) {
			text = m[1];
			this.progressElement.value = parseInt(m[2])*100;
			this.progressElement.max = parseInt(m[4])*100;
			this.progressElement.hidden = false;
			this.spinnerElement.hidden = false;
		} else {
			this.progressElement.value = null;
			this.progressElement.max = null;
			this.progressElement.hidden = true;
			if (!text) this.spinnerElement.hidden = true;
		}
		this.statusElement.innerHTML = text;
	},

	totalDependencies: 0,
	monitorRunDependencies: function (left) {
		this.totalDependencies = Math.max(this.totalDependencies, left);
		this.setStatus(left ? 'Preparing... (' + (this.totalDependencies - left) + '/' + this.totalDependencies + ')' :
							  'All downloads complete.');
	}
};
