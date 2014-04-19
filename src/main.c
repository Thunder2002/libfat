#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "def.h"
#include "error.h"
#include "fat.h"
#include "part_type.h"
#include "io.h"

const char *error_desc[] = {
	[E_SUCCESS] = "No error",
	[E_ERROR] = "Error",
	[E_INVALID_MBR_SIG] = "Invalid MBR signature"
};

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