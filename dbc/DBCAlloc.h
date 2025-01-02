/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_ALLOC_H
#define _DBC_ALLOC_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (DBCAllocFn)(void* allocator, size_t size);
typedef void (DBCFreeFn)(void* allocator, void* ptr);

void* DBCAllocDefault(void* allocator, size_t size);
void DBCFreeDefault(void* allocator, void* ptr);
void DBCFreeNope(void* allocator, void* ptr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _DBC_ALLOC_H */
