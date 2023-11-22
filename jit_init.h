#pragma once

#define JIT_EXPORT __attribute__((visibility("default")))
#define JIT_LOCAL __attribute__((visibility("hidden")))

#ifdef LLVM_IR_EMIT
#define nullptr 0
#define int32_t __int32_t
#define _Int64 long
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed _Int64 int64_t;
typedef signed _Int64 intmax_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned _Int64 uint64_t;
typedef unsigned _Int64 u_int64_t;
typedef unsigned _Int64 uintmax_t;
#else
#include <cinttypes>
#endif

#ifdef __cplusplus
extern "C" {
#endif


struct table_column_info {
    char column_name[128];
    int32_t type;
    uint32_t offset;
};

#ifdef __cplusplus
}
#endif