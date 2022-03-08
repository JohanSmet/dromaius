// gui/widgets.cpp - Johan Smet - BSD-3-Clause (see LICENSE)
//
// functions for widgets used in multiple panels

#include "widgets.h"

bool ui_bit_array_table_start(const char *id, int columns, bool header_column, const char *label) {

	static constexpr const char *table_header_digits[] = {
		"0", "1", "2", "3", "4", "5", "6", "7",
		"8", "9", "A", "B", "C", "D", "E", "F"
	};

	if (!ImGui::BeginTable(id, columns + header_column, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {
		return false;
	}

	if (header_column) {
		ImGui::TableSetupColumn((label) ? label : "##type");
	}
	for (int i = columns - 1; i >= 0; --i) {
		ImGui::TableSetupColumn(table_header_digits[i]);
	}
	ImGui::TableHeadersRow();


	return true;
}

void ui_bit_array_table_end() {
	ImGui::EndTable();
}

void ui_bit_array_table_row(const char *label, int columns, SignalValue *values) {
	ImGui::TableNextRow();
	if (label) {
		ImGui::TableNextColumn();
		ImGui::Text("%s", label);
	}

	for (int i = columns - 1; i >= 0; --i) {
		ImGui::TableNextColumn();
		ui_signal_short(values[i]);
	}
}
