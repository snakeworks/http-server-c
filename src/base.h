#ifndef BASE_H
#define BASE_H

#include <stddef.h>
#include <stdint.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t
#define f64 double
#define usize size_t

char *read_file_to_cstr(const char *path);

#endif // BASE_H
