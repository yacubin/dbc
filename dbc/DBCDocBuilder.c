/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "DBCDocBuilder.h"

DBCDocBuilder* DBCDocBuilder_create()
{
  // Add psevdo message `Vector__XXX`
  return NULL;
}

void DBCDocBuilder_destroy(DBCDocBuilder* thiz)
{
  /* Not implemented */
}

void DBCDocBuilder_setVersion(DBCDocBuilder* thiz, const char* version)
{
  /* Not implemented */
}

void DBCDocBuilder_setSymbols(DBCDocBuilder* thiz, const char** symbols, size_t count)
{
  /* Not implemented */
}

void DBCDocBuilder_setBitTiming(DBCDocBuilder* thiz, uint32_t baudrate, uint32_t btr1, uint32_t btr2)
{
  /* Not implemented */
}

void DBCDocBuilder_setNetNodes(DBCDocBuilder* thiz, const char** nodes, size_t count)
{
  /* Not implemented */
}

void DBCDocBuilder_addValTable(DBCDocBuilder* thiz, const char* name, const DBCValDesc* desc, size_t count)
{
  /* Not implemented */
}

void DBCDocBuilder_addMessage(DBCDocBuilder* thiz, uint32_t id, const char* name, uint32_t messageSize, const char* transmitter)
{
  /* Not implemented */
}

void DBCDocBuilder_addMessageTransmitter(DBCDocBuilder* thiz, uint32_t messageId, const char** transmitters, size_t count)
{
  /* Not implemented */
}
