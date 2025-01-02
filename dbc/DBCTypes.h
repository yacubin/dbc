/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_TYPES_H
#define _DBC_TYPES_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <dbc/DBCKeywords.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char* kDBCStrEmpty;

typedef uint8_t DBCByteOrder;
enum {
  kDBCBigEndian = '0',
  kDBCLittleEndian = '1',
};

const char* DBCByteOrder_toCString(DBCByteOrder byteOrder);

typedef uint8_t DBCSigValueType;
enum {
  kDBCSigValueInteger = 0,
  kDBCSigValueFloat = 1,
  kDBCSigValueDouble = 2,
  kDBCSigValueUnknown = 3,
};

const char* DBCSigValueType_toCString(DBCSigValueType valueType);

typedef uint32_t DBCEnvId;

#define DBC_READ_FLAG (1 << 0)
#define DBC_WRITE_FLAG (1 << 1)
#define DBC_STRING_FLAG (1 << 15)

typedef uint16_t DBCEnvAccessType;

typedef uint8_t DBCEnvValueType;
enum {
  kDBCEnvValueInteger = 0,
  kDBCEnvValueFloat = 1,
  kDBCEnvValueString = 2,
};

typedef struct DBCEnvInfo DBCEnvInfo;
struct DBCEnvInfo {
  DBCEnvId id; /* obsolete */
  DBCEnvValueType valueType;
  DBCEnvAccessType accessType;

  double minimum;
  double maximum;
  double  initialValue;

  const char* unit;
};

typedef uint8_t DBCObjectType;
enum {
  kDBCObjectDocument,
  kDBCObjectNetNode,
  kDBCObjectMessage,
  kDBCObjectSignal,
  kDBCObjectEnvVar,
};

typedef uint8_t DBCAttrValueType;
enum {
  kDBCAttrValueInt,
  kDBCAttrValueHex,
  kDBCAttrValueFloat,
  kDBCAttrValueString,
  kDBCAttrValueEnum,
};

const char* DBCAttrValueType_toCString(DBCAttrValueType);

struct DBCAttrValueInfo {
  DBCAttrValueType type;
  union {
    struct {
      int32_t first;
      int32_t second;
    } vInt;

    struct {
      int32_t first;
      int32_t second;
    } vHex;

    struct {
      double first;
      double second;
    } vFloat;

    struct {
      const char** data;
      size_t count;
    } vEnum;
  };
};

typedef uint8_t DBCVariantType;
enum {
  kDBCVariantInt = 0,
  kDBCVariantUint = 1,
  kDBCVariantFloat = 2,
  kDBCVariantString = 3,
};

typedef struct DBCVariant DBCVariant;
struct DBCVariant {
  DBCVariantType type;
  union {
    int64_t vInt;
    double vFloat;
    const char* vString;
  };
};

typedef struct DBCValDesc DBCValDesc;
struct DBCValDesc {
  uint32_t value;
  const char* description;
};

typedef struct DBCSigInfo DBCSigInfo;
struct DBCSigInfo {
  uint32_t sizeInBits;
  DBCByteOrder byteOrder;
  bool isUnsigned;
  double factor;
  double offset;
  double minimum;
  double maximum;
  const char* unit;
};

typedef struct DBCRange DBCRange;
struct DBCRange {
  uint32_t first;
  uint32_t second;
};

typedef struct DBCBitTiming DBCBitTiming;
struct DBCBitTiming {
  uint32_t baudrate;
  uint32_t btr1;
  uint32_t btr2;
};

typedef struct DBCMulSwValue DBCMulSwValue;
struct DBCMulSwValue {
  bool has;
  uint32_t value;
};

typedef struct DBCTarget DBCTarget;
struct DBCTarget {
  DBCObjectType objectType;

  union {
    struct {
      const char* name;
    } netNode;

    struct {
      uint32_t id;
    } message;

    struct {
      uint32_t messageId;
      const char* name;
    } signal;

    struct {
      const char* name;
    } envVar;
  };
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _DBC_TYPES_H */
