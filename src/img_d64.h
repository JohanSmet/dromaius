// img_d64.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Parse .d64 disk images

#ifndef DROMAIUS_IMG_D64_H
#define DROMAIUS_IMG_D64_H

#include "types.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
typedef struct {
	uint8_t next_dir_track;
	uint8_t next_dir_sector;
	uint8_t file_type;
	uint8_t start_of_file_track;
	uint8_t start_of_file_sector;
	uint8_t	name[16];
	uint8_t rel_track;
	uint8_t rel_sector;
	uint8_t rel_record_length;
	uint8_t unused_1[6];
	uint16_t file_size;
} D64DirEntry;

typedef struct {
	uint8_t first_dir_track;
	uint8_t first_dir_sector;
	uint8_t dos_version;
	uint8_t unused_1;
	uint8_t bam[140];
	uint8_t disk_name[16];
	uint8_t unused_2[2];
	uint8_t disk_id[2];
	uint8_t unused_3;
	uint8_t dos_type[2];
	uint8_t unused_4[89];
} D64Bam;

#pragma pack(pop)

static_assert(sizeof(D64DirEntry) == 32, "D64DirEntry wrong size");
static_assert(sizeof(D64Bam) == 256, "D64Bam wrong size");

typedef struct {
	int8_t *		raw_data;

	D64DirEntry **	dir_entries;
	D64Bam *		bam;
	uint16_t		free_sectors;

	int8_t *		dirlist_buffer;
} DiskImageD64;

bool img_d64_parse_memory(int8_t *input, DiskImageD64 *d64);
bool img_d64_is_valid(DiskImageD64 *d64);
ptrdiff_t img_d64_basic_directory_list(DiskImageD64 *d64, int8_t **listing);
uint16_t img_d64_file_start_track_sector(DiskImageD64 *d64, uint8_t *name, size_t name_len);
ptrdiff_t img_d64_file_block(DiskImageD64 *d64, uint16_t track_sector, uint8_t **data, uint16_t *next_track_sector);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_IMG_D64_H
