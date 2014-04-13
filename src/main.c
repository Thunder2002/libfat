#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef __LIBFAT_ERROR_H__
#define __LIBFAT_ERROR_H__

typedef enum {
	E_SUCCESS = 0,
	E_ERROR = 1,
	E_INVALID_MBR_SIG = 2
} error_t;

char *error_desc[] = {
	[E_SUCCESS] = "No error",
	[E_ERROR] = "Error",
	[E_INVALID_MBR_SIG] = "Invalid MBR signature"
};

#endif 

#ifndef __LIBFAT_PART_TYPE_H__
#define __LIBFAT_PART_TYPE_H__

typedef enum {
	PART_TYPE_EMPTY = 0x00,
	PART_TYPE_FAT12 = 0x01,
	PART_TYPE_FAT16_32 = 0x04,
	PART_TYPE_EXTENDED = 0x05,
	PART_TYPE_FAT16 = 0x06,
	PART_TYPE_NTFS = 0x07,
	PART_TYPE_FAT32 = 0x0B,
	PART_TYPE_FAT32_LBA = 0x0C,
	PART_TYPE_DYNAMIC = 0x42,
	PART_TYPE_LINUX_SWAP = 0x82,
	PART_TYPE_LINUX_LVM = 0x83
} part_type_t;

char *part_type_desc[] = {
	[PART_TYPE_EMPTY] = "Empty",
	[PART_TYPE_FAT12] = "FAT12",
	[PART_TYPE_FAT16_32] = "FAT16 <= 32MiB",
	[PART_TYPE_EXTENDED] = "Extended",
	[PART_TYPE_FAT16] = "FAT16",
	[PART_TYPE_NTFS] = "NTFS",
	[PART_TYPE_FAT32] = "FAT32",
	[PART_TYPE_FAT32_LBA] = "FAT32 LBA",
	[PART_TYPE_DYNAMIC] = "Dynamic",
	[PART_TYPE_LINUX_SWAP] = "Linux SWAP",
	[PART_TYPE_LINUX_LVM] = "Linux LVM"
};

#endif 

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

typedef char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

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

error_t io_read_block(char *buffer, uint32_t offset, uint32_t len);
error_t io_read_sector(char *buffer, uint32_t offset);
uint16_t io_parse_uint16(char *buffer, uint16_t offset);
uint32_t io_parse_uint32(char *buffer, uint16_t offset);
error_t mbr_parse(char *buffer, part_t parts[]);
error_t fat_read_sector0(char *buffer, part_t *part, fat_t *fat);
error_t fat_read_entries(char *buffer, part_t *part, fat_t *fat, fat_entry_t *entry);
void debug_print_buffer(char *buffer, uint32_t len);
void debug_print_part(part_t *part);
void debug_print_fat(fat_t *fat);
void debug_print_fat_entry(fat_entry_t *entry);
void debug_print_fat_datetime(fat_datetime_t dt);

/**
 * Reads a block of data at the given offset of the given length into the 
 * provided buffer. The buffer needs to be preallocated and big enought.
 * @param buffer The byte buffer to read the block into.
 * @param offset The offset of the block in bytes.
 * @param len The length of the block in bytes.
 * @return Whether the read operation was successful or an error occured.
 */
error_t io_read_block(char *buffer, uint32_t offset, uint32_t len) {
	FILE *file;
	file = fopen("512mb_kingston_sd.hex", "r");
	if (file == NULL) {
		return E_ERROR;
	}
	if (fseek(file, offset, SEEK_SET) != 0) {
		return E_ERROR;
	}	
	if (fread(buffer, (size_t)len, 1, file) != 1) {
		return E_ERROR;
	}
	fclose(file);

	return E_SUCCESS;
}

/**
 * Reads the sector at the given offset into the provided buffer. The buffer
 * needs to be preallocated and big enought.
 * @param buffer The byte buffer to read the sector into.
 * @param offset The sector offset as the sector number, not bytes!
 * @return Whether the read operation was successful or an error occured.
 */
error_t io_read_sector(char *buffer, uint32_t offset) {
	return io_read_block(buffer, offset * SECTOR_LEN, SECTOR_LEN);
}

/** 
 * Parsing a FAT datetime from the given buffer at the given offset.
 * @param buffer The byte buffer with the datetime.
 * @param offset The offset inside the given buffer.
 * @return The parsed and filled FAT datetime.
 */
fat_datetime_t io_parse_fat_datetime(char *buffer, uint16_t offset) {
	fat_datetime_t value;
	uint16_t tmp;

	tmp = io_parse_uint16(buffer, offset);
	value.hour = tmp >> 11 & 0x1F;
	value.minute = tmp >> 5 & 0x3F;
	value.second = tmp & 0x1F;

	tmp = io_parse_uint16(buffer, offset + 2);
	value.year = tmp >> 9 & 0x7F;
	value.month = tmp >> 5 & 0x0F;
	value.day = tmp & 0x1F;

	return value;
}

/** 
 * Parsing an unsigned 16-bit integer from the given buffer at the given offset.
 * @param buffer The byte buffer with the uint16.
 * @param offset The offset inside the given buffer.
 * @return The parsed unsigned 16-bit integer.
 */
uint16_t io_parse_uint16(char *buffer, uint16_t offset) {
	return (((buffer[offset + 1] & 0xff) << 8) |
		(buffer[offset + 0] & 0xff));
}

/** 
 * Parsing an unsigned 32-bit integer from the given buffer at the given offset.
 * @param buffer The byte buffer with the uint32.
 * @param offset The offset inside the given buffer.
 * @return The parsed unsigned 32-bit integer.
 */
uint32_t io_parse_uint32(char *buffer, uint16_t offset) {
	return (((buffer[offset + 3] & 0xff) << 24) | 
		((buffer[offset + 2] & 0xff) << 16) | 
		((buffer[offset + 1] & 0xff) << 8) | 
		(buffer[offset + 0] & 0xff));
}

/**
 * Parses the MBR from the given buffer and outputs all primary partitions. 
 * The provided partition array needs to be preallocated and large enought.
 * @param buffer The byte buffer with the MBR raw data in it.
 * @param parts The array of primary partitions which gets filled.
 * @return Whether parsing the MBR was successful or an error occured.
 */
error_t mbr_parse(char *buffer, part_t parts[]) {
	uint8_t i = 0;
	uint16_t offset;

	// Check MBR signature
	if ((buffer[SECTOR_LEN - 2] & 0xff) != 0x55 || (buffer[SECTOR_LEN - 1] & 0xff) != 0xAA) {
		return E_INVALID_MBR_SIG;
	}

	for (i = 0; i < PARTS_MAX; ++i) {		
		offset = PARTS_TBL_OFFSET + (i * PARTS_ENTRY_LEN);

		parts[i].boot = buffer[offset + 0];
		parts[i].type = buffer[offset + 4];
		parts[i].start = io_parse_uint32(buffer, offset + 8);
		parts[i].sectors = io_parse_uint32(buffer, offset + 12);
	}

	return E_SUCCESS;
}

error_t fat_read_sector0(char *buffer, part_t *part, fat_t *fat) {
	io_read_sector(buffer, part->start);

	fat->bytesPerSector = io_parse_uint16(buffer, 0x0B);
	fat->sectorsPerCluster = (uint8_t)buffer[0x0D];
	fat->reservedSectors = io_parse_uint16(buffer, 0x0E);
	fat->totalFATs = (uint8_t)buffer[0x10];
	fat->maxRootEntries = io_parse_uint16(buffer, 0x11);
	fat->sectorsPerFAT = io_parse_uint16(buffer, 0x16);

	if (part->type == PART_TYPE_FAT16) {		
		fat->totalSectors = io_parse_uint16(buffer, 0x13);
		if (fat->totalSectors == 0) {
			fat->totalSectors = io_parse_uint16(buffer, 0x20);
		}
	} else if (part->type == PART_TYPE_FAT32) {
		fat->totalSectors = io_parse_uint16(buffer, 0x24);
	}

	return E_SUCCESS;
}

error_t fat_read_entries(char *buffer, part_t *part, fat_t *fat, fat_entry_t *entry) {
	uint32_t fatTblOffset = (fat->bytesPerSector * fat->reservedSectors);
	uint32_t fatDataOffset = fatTblOffset + (fat->totalFATs * fat->sectorsPerFAT * fat->bytesPerSector);
	// Should work due to specs but doesn't, maybe because of FAT16 + VFAT for LFN
	/*if (part->type == PART_TYPE_FAT16) {
		fatDataOffset += (fat->maxRootEntries * FAT_ENTRY_LEN) / fat->bytesPerSector;
	}*/

	uint8_t i, j;
	uint32_t offset = 0;
	uint16_t bufferOffset;
	uint32_t datetime;
	bool terminate = false;
	do {
		io_read_sector(buffer, part->start + ((fatDataOffset + offset) / SECTOR_LEN));
		//debug_print_buffer(buffer, SECTOR_LEN);

		bufferOffset = 0;
		for (i = 0; i < 16 && !terminate; ++i) {
			// Skip long file name entries
			if (buffer[bufferOffset + 0x0B] == 0x0F && buffer[bufferOffset + 0x1A] == 0) {
				bufferOffset += FAT_ENTRY_LEN;
				continue;
			}

			// Terminate on null entry
			if (buffer[bufferOffset + 0x1A] == 0) {
				terminate = true;
				continue;
			}

			// Read entry values
			for (j = 0; j < FAT_ENTRY_NAME_LEN; ++j) {
				entry->name[j] = buffer[bufferOffset + 0x00 + j];
			}
			for (j = 0; j < FAT_ENTRY_EXT_LEN; ++j) {
				entry->ext[j] = buffer[bufferOffset + 0x08 + j];	
			}
			entry->attr = buffer[bufferOffset + 0x0B];
			entry->cluster = io_parse_uint16(buffer, bufferOffset + 0x1A) |
				(io_parse_uint16(buffer, bufferOffset + 0x14) << 16);
			entry->size = io_parse_uint32(buffer, bufferOffset + 0x1C);
			entry->created = io_parse_fat_datetime(buffer, bufferOffset + 0x0E);
			entry->changed = io_parse_fat_datetime(buffer, bufferOffset + 0x16);

			bufferOffset += FAT_ENTRY_LEN;

			debug_print_fat_entry(entry);
		}

		offset += fat->bytesPerSector;
	} while (!terminate);
}

void debug_print_buffer(char *buffer, uint32_t len) {
	int i;
	for (i = 1; i <= len; ++i) {
		printf("%02X", buffer[i - 1] & 0xff);
			
		if (i % 24 == 0 && i > 0) {			
			printf("\n");
		} else if (i % 8 == 0 && i > 0) {
			printf(" | ");
		} else {
			printf(" ");
		}
	}
	printf("\n");
}

void debug_print_part(part_t *part) {
	printf("Boot: %d\n", part->boot);
	printf("Type: %s (%d)\n", part_type_desc[part->type], part->type);
	printf("Start: %d [%x]\n", part->start, part->start);
	printf("Sectors: %d [%x]\n", part->sectors, part->sectors);
}

void debug_print_fat(fat_t *fat) {
	printf("Bytes per sector: %d\n", fat->bytesPerSector);
	printf("Sectors per cluster: %d\n", fat->sectorsPerCluster);
	printf("Total FATs: %d\n", fat->totalFATs);
	printf("Max root entries: %d\n", fat->maxRootEntries);
	printf("Reserved sectors: %d\n", fat->reservedSectors);
	printf("Sectors per FAT: %d\n", fat->sectorsPerFAT);
	printf("Total sectors: %d\n", fat->totalSectors);
}

void debug_print_fat_entry(fat_entry_t *entry) {
	uint8_t i;
	printf("Name: ");
	for (i = 0; i < FAT_ENTRY_NAME_LEN; ++i) {
		putchar(entry->name[i]);
	}
	putchar('.');
	for (i = 0; i < FAT_ENTRY_EXT_LEN; ++i) {
		putchar(entry->ext[i]);
	}
	printf(" | ");

	printf("Attr: %#02x | ", entry->attr);
	printf("Cluster: %#08x | ", entry->cluster);
	printf("Size: %ld\n", entry->size);

	printf("Created: ");
	debug_print_fat_datetime(entry->created);
	printf(" | ");
	printf("Changed: ");
	debug_print_fat_datetime(entry->changed);
	printf("\n");
}

void debug_print_fat_datetime(fat_datetime_t dt) {
	printf("%02d.%02d.%04d %02d:%02d:%02d",
		dt.day & 0x1F,
		dt.month & 0x0F,
		(dt.year & 0x7F) + 1980,
		dt.hour & 0x1F, 
		dt.minute & 0x3F,
		(dt.second & 0x1F) * 2);
}

int main(void) {	
	int i;	

	char buffer[SECTOR_LEN];
	if (io_read_sector(buffer, 0) == E_SUCCESS) {
		//debug_print_buffer(buffer, SECTOR_LEN);
		part_t parts[PARTS_MAX];
		if (mbr_parse(buffer, parts) == E_SUCCESS) {		
			debug_print_part(&parts[0]);

			fat_t fat;
			fat_read_sector0(buffer, &parts[0], &fat);
			//debug_print_buffer(buffer, SECTOR_LEN);
			debug_print_fat(&fat);

			fat_entry_t entry;
			fat_read_entries(buffer, &parts[0], &fat, &entry);
		}
	}

	return EXIT_SUCCESS;	
}