#ifndef __LIBFAT_ERROR_H__
#define __LIBFAT_ERROR_H__

typedef enum {
	E_SUCCESS = 0,
	E_ERROR = 1,
	E_INVALID_MBR_SIG = 2
} error_t;

#endif 