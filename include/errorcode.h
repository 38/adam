#ifndef __ERRORCODE_H__
#include <stdint.h>
typedef uint32_t retstatus_t;
typedef struct {
    retstatus_t status;
    const char* desc;
} errorcode_t;
retstatus_t errorcode_raise(const errorcode_t* code, const char* file, int line);
const char* errorcode_strerr(const char* prefix);
void errorcode_clear(void);
retstatus_t errorcode_last(void);

extern errorcode_t global_errorcodes[];
#define RAISE(what) return errorcode_raise(what, __FILE__, __LINE__)

#define EGLOBAL 0x00000000U
#define ETYPE(code) ((code)&0xffff0000U)

#define ESUCCESS global_errorcodes[0]
#define EUNKNOWN global_errorcodes[1]

#define __ERRORCODE_H__
