/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_DOCBUILDER_H
#define _DBC_DOCBUILDER_H

#include <dbc/DBCTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DBCDocBuilder DBCDocBuilder;

DBCDocBuilder* DBCDocBuilder_create();
void DBCDocBuilder_destroy(DBCDocBuilder*);
void DBCDocBuilder_setVersion(DBCDocBuilder*, const char* version);
void DBCDocBuilder_setSymbols(DBCDocBuilder*, const char** symbols, size_t count);
void DBCDocBuilder_setBitTiming(DBCDocBuilder*, uint32_t baudrate, uint32_t btr1, uint32_t btr2);
void DBCDocBuilder_setNetNodes(DBCDocBuilder*, const char** nodes, size_t count);
void DBCDocBuilder_addValTable(DBCDocBuilder*, const char* name, const DBCValDesc* desc, size_t count);
void DBCDocBuilder_addMessage(DBCDocBuilder*, uint32_t id, const char* name, uint32_t messageSize, const char* transmitter);
void DBCDocBuilder_addMessageTransmitter(DBCDocBuilder*, uint32_t messageId, const char** transmitters, size_t count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _DBC_DOCBUILDER_H */
