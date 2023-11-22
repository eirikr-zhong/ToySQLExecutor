

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
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


struct table_column_info {
    char column_name[128];
    int32_t type;
    uint32_t offset;
};

const struct table_column_info g_table_infos[] = {{.column_name="a",.type = 1,.offset = 0}};

#ifdef __cplusplus
}
#endif

const struct table_column_info g_table_info[] = {{.column_name = "a", .type = 1, .offset = 0}};

JIT_EXPORT int strcmp(const char *l, const char *r) {
    for (; *l == *r && *l; l++, r++)
        ;
    return *(unsigned char *) l - *(unsigned char *) r;
}

JIT_EXPORT const struct table_column_info *query_table_column(const struct table_column_info *infos, uint32_t info_count, const char *column_name) {
    for (uint32_t i = 0; i < info_count; i++) {
        const struct table_column_info *info = infos + i;
        if (strcmp(column_name, info->column_name) == 0)
            return info;
    }
    return 0;
}

JIT_EXPORT int64_t get_table_column_int64(const struct table_column_info* info,const char* addr)
{
    const char* value = addr + info->offset;
    return *(const int64_t*)value;
}