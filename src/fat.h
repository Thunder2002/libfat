#ifndef __LIBFAT_FAT_H__
#define __LIBFAT_FAT_H__

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_DAY 86400
#define SECTOR_LEN 512
#define PARTS_MAX 4
#define PARTS_TBL_OFFSET 446
#define PARTS_ENTRY_LEN 16
#define FAT_ENTRY_LEN 32
#define FAT_ENTRY_NAME_LEN 8
#define FAT_ENTRY_EXT_LEN 3

typedef struct {
	bool boot; // 1
	uint8_t type; // 1
	uint32_t start; // 4
	uint32_t sectors; // 4
} part_t; // 10

typedef struct {
	uint16_t bytesPerSector; // 2
	uint8_t sectorsPerCluster; // 1
	uint8_t totalFATs; // 1
	uint16_t reservedSectors; // 2
	uint32_t totalSectors; // 4
	uint16_t maxRootEntries; // 2
	uint16_t sectorsPerFAT; // 2
} fat_t; // 14

typedef struct __attribute__((__packed__)) {
	uint8_t hour : 5;
	uint8_t minute : 6;
	uint8_t second : 5;
	uint8_t year : 7;
	uint8_t month : 4;
	uint8_t day : 5;
} fat_datetime_t; // 4 expected / 6 real // TODO

typedef struct {
	uint8_t name[8]; // 8
	uint8_t ext[3]; // 3
	uint8_t attr; // 1
	fat_datetime_t created; // 4
	fat_datetime_t changed; // 4
	uint32_t cluster; // 4
	uint32_t size; // 4
} fat_entry_t; // 28

error_t mbr_parse(char *buffer, part_t parts[]);
error_t fat_read_sector0(char *buffer, part_t *part, fat_t *fat);
error_t fat_read_entries(char *buffer, part_t *part, fat_t *fat, fat_entry_t *entry);
void debug_print_buffer(char *buffer, uint32_t len);
void debug_print_part(part_t *part);
void debug_print_fat(fat_t *fat);
void debug_print_fat_entry(fat_entry_t *entry);
void debug_print_fat_datetime(fat_datetime_t dt);

#endif