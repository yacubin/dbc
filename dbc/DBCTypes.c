/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "DBCTypes.h"

#include <string.h>

#include "dbc/DBCAssert.h"

const char* kDBCStrEmpty = "";

const char* DBCByteOrder_toCString(DBCByteOrder byteOrder)
{
  switch (byteOrder) {
  case kDBCBigEndian:
    return "BIG_ENDIAN";
  case kDBCLittleEndian:
    return "LITTLE_ENDIAN";
  }

  DBC_ASSERT_NOT_REACHED();
  return NULL;
}

const char* DBCSigValueType_toCString(DBCSigValueType valueType)
{
  switch (valueType) {
  case kDBCSigValueInteger:
    return "INTEGER";
  case kDBCSigValueFloat:
    return "FLOAT";
  case kDBCSigValueDouble:
    return "DOUBLE";
  case kDBCSigValueUnknown:
    return "UNKNOWN";
  }

  DBC_ASSERT_NOT_REACHED();
  return NULL;
}

const char* DBCAttrValueType_toCString(DBCAttrValueType valueType)
{
  switch (valueType) {
  case kDBCAttrValueInt:
    return "INT";
  case kDBCAttrValueHex:
    return "HEX";
  case kDBCAttrValueFloat:
    return "FLOAT";
  case kDBCAttrValueString:
    return "STRING";
  case kDBCAttrValueEnum:
    return "ENUM";
  }

  DBC_ASSERT_NOT_REACHED();
  return NULL;
}
