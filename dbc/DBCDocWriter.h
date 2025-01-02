/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_DOCWRITER_H
#define _DBC_DOCWRITER_H

#include <stdbool.h>

#include <dbc/DBCAlloc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DBCDocument DBCDocument;
typedef struct DBCDocWriter DBCDocWriter;

DBCDocWriter* DBCDocWriter_create(const char* name);
DBCDocWriter* DBCDocWriter_create2(const char* name, void* allocator, DBCAllocFn* alloc, DBCFreeFn* free);
void DBCDocWriter_destroy(DBCDocWriter*);

bool DBCDocWriter_write(DBCDocWriter*, const void* data, size_t size);
DBCDocument* DBCDocWriter_toDocument(DBCDocWriter*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _DBC_DOCWRITER_H */
