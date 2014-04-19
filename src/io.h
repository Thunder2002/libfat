#ifndef __FATLIB_IO_H__
#define __FATLIB_IO_H__

#include "def.h"
#include "error.h"
#include "fat.h"

error_t io_read_block(char *buffer, uint32_t offset, uint32_t len);
error_t io_read_sector(char *buffer, uint32_t offset);
fat_datetime_t io_parse_fat_datetime(char *buffer, uint16_t offset);
uint16_t io_parse_uint16(char *buffer, uint16_t offset);
uint32_t io_parse_uint32(char *buffer, uint16_t offset);

#endif