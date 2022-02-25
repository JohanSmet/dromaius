// img_d64.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Parse .d64 disk images

#include "img_d64.h"
#include <stb/stb_ds.h>

///////////////////////////////////////////////////////////////////////////////
//
// lookup tables / data
//

static const uint32_t D64_SECTOR_SIZE = 256;
static const uint32_t D64_TRACK_OFFSETS[] = {
	0x00000, 0x01500, 0x02A00, 0x03F00, 0x05400, 0x06900, 0x07E00, 0x09300,
	0x0A800, 0x0BD00, 0x0D200, 0x0E700, 0x0FC00, 0x11100, 0x12600, 0x13B00,
	0x15000, 0x16500, 0x17800, 0x18B00, 0x19E00, 0x1B100, 0x1C400, 0x1D700,
	0x1EA00, 0x1FC00, 0x20E00, 0x22000, 0x23200, 0x24400, 0x25600, 0x26700,
	0x27800, 0x28900, 0x29A00, 0x2AB00, 0x2BC00, 0x2CD00, 0x2DE00, 0x2EF00,
	0x30000, 0x31100
};

static const char *D64_FILE_TYPES[] = {
	[0] = "DEL",
	[1] = "SEQ",
	[2] = "PRG",
	[3] = "USR",
	[4] = "REL",
};

static const uint8_t D64_DIRECTORY_TRACK = 18;

///////////////////////////////////////////////////////////////////////////////
//
// private functions
//

static int8_t *d64prv_pointer_to_track_sector(DiskImageD64 *d64, uint8_t track, uint8_t sector) {
	return d64->raw_data + D64_TRACK_OFFSETS[track - 1] + (sector * D64_SECTOR_SIZE);
}

static void d64prv_parse_directory_entries(DiskImageD64 *d64) {

	uint8_t current_track = D64_DIRECTORY_TRACK;
	uint8_t current_sector = 1;

	while (current_track != 0 && current_sector != 0) {

		// get a pointer to the first directory entry in the sector
		D64DirEntry *entry = (D64DirEntry *) d64prv_pointer_to_track_sector(d64, current_track, current_sector);

		// first directory entry contains pointer to the next directory sector
		current_track = entry->next_dir_track;
		current_sector = entry->next_dir_sector;

		// iterate over the eight entries in this sector
		for (int i = 0; i < 8; ++i) {
			// TODO: check if this is the right condition
			// skip 'empty' file entries
			if (entry->file_size == 0) {
				continue;
			}

			arrpush(d64->dir_entries, entry);
			entry += 1;
		}
	}
}

static uint16_t combine_track_sector(uint8_t track, uint8_t sector) {
	return (uint16_t) ((track << 8) | sector);
}

static inline size_t dl_push_uint16(int8_t **arr, uint16_t value) {
	arrpush(*arr, (int8_t) (value & 0xff));
	arrpush(*arr, (int8_t) (value >> 8));
	return arrlenu(*arr) - 2;
}

static inline void dl_write_uint16(int8_t *arr, uint16_t value, size_t offset) {
	arr[offset] = (int8_t) (value & 0xff);
	arr[offset+1] = (int8_t) (value >> 8);
}

static size_t dl_push_string(int8_t **arr, const uint8_t *string, size_t size, bool trim) {

	size_t result = 0;

	for (size_t i = 0; i < size; ++i) {

		if (string[i] == 0xa0) {
			// non-breaking space: replace with regular space or stop if trim == true
			if (!trim) {
				arrpush(*arr, ' ');
			} else {
				break;
			}
		} else {
			arrpush(*arr, (int8_t) string[i]);
		}

		result += 1;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

bool img_d64_parse_memory(int8_t *input, DiskImageD64 *d64) {
	assert(input);
	assert(d64);

	d64->raw_data = input;

	// process directory information
	d64prv_parse_directory_entries(d64);

	// parse block availability map
	d64->bam = (D64Bam *) d64prv_pointer_to_track_sector(d64, D64_DIRECTORY_TRACK, 0);
	d64->free_sectors = 0;

	for (size_t i = 0; i < sizeof(d64->bam->bam); i += 4) {
		if (i != (size_t) (4 * (D64_DIRECTORY_TRACK - 1))) {
			d64->free_sectors = (uint16_t) (d64->free_sectors + d64->bam->bam[i]);
		}
	}

	return false;
}

bool img_d64_is_valid(DiskImageD64 *d64) {
	return d64 && d64->raw_data && d64->bam;
}

ptrdiff_t img_d64_basic_directory_list(DiskImageD64 *d64, int8_t **listing) {
	assert(d64);
	assert(listing);

	const uint8_t FOOTER_TEXT[] = "BLOCKS FREE.";
	const uint8_t TEXT_SPACES[24] = "                        ";


	if (d64->dirlist_buffer) {
		stbds_header(d64->dirlist_buffer)->length = 0;
	}

	const uint16_t base_address = 0x0401;
	dl_push_uint16(&d64->dirlist_buffer, base_address);		// load address

	// line number 0: disk information
	size_t line_offset = dl_push_uint16(&d64->dirlist_buffer, 0x0000);		// save room for next line pointer
	dl_push_uint16(&d64->dirlist_buffer, 0x0000);		// line number (0)
	arrpush(d64->dirlist_buffer, 0x12);					// reverse on
	arrpush(d64->dirlist_buffer, '"');
	dl_push_string(&d64->dirlist_buffer, d64->bam->disk_name, 16, false);
	arrpush(d64->dirlist_buffer, '"');
	arrpush(d64->dirlist_buffer, ' ');
	arrpush(d64->dirlist_buffer, (int8_t) d64->bam->disk_id[0]);
	arrpush(d64->dirlist_buffer, (int8_t) d64->bam->disk_id[1]);
	arrpush(d64->dirlist_buffer, ' ');
	arrpush(d64->dirlist_buffer, (int8_t) d64->bam->dos_type[0]);
	arrpush(d64->dirlist_buffer, (int8_t) d64->bam->dos_type[1]);
	arrpush(d64->dirlist_buffer, 0x00);					// end of line

	dl_write_uint16(d64->dirlist_buffer, (uint16_t) (base_address + arrlenu(d64->dirlist_buffer) - 2), line_offset);

	// files
	for (size_t i = 0; i < arrlenu(d64->dir_entries); ++i) {
		D64DirEntry *file = d64->dir_entries[i];

		line_offset = dl_push_uint16(&d64->dirlist_buffer, 0x0000);		// save room for next line pointer

		// file size as line number
		dl_push_uint16(&d64->dirlist_buffer, file->file_size);

		if (file->file_size < 10) {
			dl_push_string(&d64->dirlist_buffer, TEXT_SPACES, 3, false);
		} else if (file->file_size < 100) {
			dl_push_string(&d64->dirlist_buffer, TEXT_SPACES, 2, false);
		} else if (file->file_size < 1000) {
			dl_push_string(&d64->dirlist_buffer, TEXT_SPACES, 1, false);
		}

		// filename
		arrpush(d64->dirlist_buffer, '"');
		size_t len = dl_push_string(&d64->dirlist_buffer, file->name, 16, true);
		arrpush(d64->dirlist_buffer, '"');
		dl_push_string(&d64->dirlist_buffer, TEXT_SPACES, 18 - len, false);

		// filetype
		if ((file->file_type & 0x0f) <= 4) {
			dl_push_string(&d64->dirlist_buffer, (const uint8_t *) D64_FILE_TYPES[file->file_type & 0x0f], 3, false);
		}

		// end of line
		arrpush(d64->dirlist_buffer, 0x00);

		dl_write_uint16(d64->dirlist_buffer, (uint16_t) (base_address + arrlenu(d64->dirlist_buffer) - 2), line_offset);
	}

	// footer
	line_offset = dl_push_uint16(&d64->dirlist_buffer, 0x0000);		// save room for next line pointer
	dl_push_uint16(&d64->dirlist_buffer, d64->free_sectors);		// line number == free sectors
	dl_push_string(&d64->dirlist_buffer, FOOTER_TEXT, sizeof(FOOTER_TEXT), false);
	arrpush(d64->dirlist_buffer, 0x00);					// end of line
	dl_write_uint16(d64->dirlist_buffer, (uint16_t) (base_address + arrlenu(d64->dirlist_buffer) - 2), line_offset);

	dl_push_uint16(&d64->dirlist_buffer, 0x0000);		// end of program

	*listing = d64->dirlist_buffer;
	return arrlen(d64->dirlist_buffer);
}

uint16_t img_d64_file_start_track_sector(DiskImageD64 *d64, uint8_t *name, size_t name_len) {
	assert(d64);
	assert(name);
	assert(name_len >= 1);

	D64DirEntry *entry = NULL;

	if (name_len == 1 && name[0] == '*') {
		entry = d64->dir_entries[0];
	} else {
		// floppy disk probably won't have so much file that a linear search is going to be a problem
		for (size_t i = 0; i < arrlenu(d64->dir_entries); ++i) {
			if (memcmp(d64->dir_entries[i]->name, name, name_len) == 0) {
				entry = d64->dir_entries[i];
				break;
			}
		}
	}

	if (!entry) {
		return 0;
	}

	return combine_track_sector(entry->start_of_file_track, entry->start_of_file_sector);
}

ptrdiff_t img_d64_file_block(DiskImageD64 *d64, uint16_t track_sector, uint8_t **data, uint16_t *next_track_sector) {
	assert(d64);
	assert(data);
	assert(next_track_sector);

	uint8_t *block = (uint8_t *) d64prv_pointer_to_track_sector(d64, (uint8_t) (track_sector >> 8), (uint8_t) (track_sector & 0xff));

	// first two bytes of block are the link to next track/sector, if track == 0: this is the last block
	*data = block + 2;

	if (block[0] == 0) {
		*next_track_sector = 0;
		return block[1];
	}

	*next_track_sector = combine_track_sector(block[0], block[1]);
	return D64_SECTOR_SIZE - 2;
}
