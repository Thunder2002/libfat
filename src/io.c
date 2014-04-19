#include <stdio.h>
#include <stdbool.h>

#include "io.h"

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