#ifndef _COMMON_H
#define _COMMON_H

// Error type code
typedef enum {
	NO_ERROR = 0,
	FILE_READ_ERROR,
	FILE_WRITE_ERROR,
	MEMORY_ERROR,
	OTHER_ERROR,
} ErrorType;

#endif