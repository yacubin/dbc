/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_PARSER_H
#define _DBC_PARSER_H

#include <dbc/DBCTypes.h>
#include <dbc/DBCErrorCode.h>
#include <dbc/DBCAlloc.h>

#ifdef __cplusplus
extern "C" {
#endif

enum DBCSectionType {
  kDBCSectionVersion,
  kDBCSectionNewSymbols,
  kDBCSectionBitTiming,
  kDBCSectionNetNodes,
  kDBCSectionValTable,
  kDBCSectionMessage,
  kDBCSectionMessageTransmitter,
  kDBCSectionSignal,
  kDBCSectionSignalType,
  kDBCSectionSignalTypeRef,
  kDBCSectionSignalGroup,
  kDBCSectionSignalValue,
  kDBCSectionSignalValueType,
  kDBCSectionSignalMultiplexed,
  kDBCSectionEnvVar,
  kDBCSectionEnvVarData,
  kDBCSectionEnvVarValue,
  kDBCSectionComment,
  kDBCSectionAttr,
  kDBCSectionAttrRel,
  kDBCSectionAttrDefinition,
  kDBCSectionAttrRelDefinition,
  kDBCSectionAttrDefault,
  kDBCSectionAttrRelDefault,
};

struct DBCSectionVersion {
  int type;
  const char* data;
};

struct DBCSectionNewSymbols {
  int type;
  const char** data;
  size_t count;
};

struct DBCSectionBitTiming {
  int type;
  DBCBitTiming value;
};

struct DBCSectionNetNodes {
  int type;
  const char** names;
  size_t count;
};

struct DBCSectionValTable {
  int type;
  const char* name;
  const DBCValDesc* valDesc;
  size_t count;
};

struct DBCSectionMessage {
  int type;
  uint32_t id;
  uint32_t sizeInBytes;
  const char* name;
  const char* transmitter;
};

struct DBCSectionSignal {
  int type;

  const char* name;

  bool isMultiplexorSwitch;  // "M"
  bool hasMultiplexorValue;  // "m"
  uint32_t multiplexorValue;

  uint32_t startBit;

  DBCSigInfo info;

  const char** receivers;
  size_t count;
};

struct DBCSectionSignalType {
  int type;

  const char* name;
  DBCSigInfo info;
  double defaultValue;
  const char* valTable;
};

struct DBCSectionSignalTypeRef {
  int type;
  uint32_t messageId;
  const char* signalTypeName;
  const char* signalName;
};

struct DBCSectionSignalGroup {
  int type;
  uint32_t messageId;
  uint32_t repetitions;
  const char* name;
  const char** signals;
  size_t count;
};

struct DBCSectionMessageTransmitter {
  int type;
  uint32_t messageId;
  const char** transmitters;
  size_t count;
};

struct DBCSectionSignalValue {
  int type;
  uint32_t messageId;
  const char* signalName;
  const DBCValDesc* valDesc;
  size_t count;
};

struct DBCSectionSignalValueType {
  int type;
  DBCSigValueType value;
  uint32_t messageId;
  const char* signalName;
};

struct DBCSectionEnvVar {
  int type;
  const char* name;
  DBCEnvInfo info;
  const char** accessNode;
  size_t count;
};

struct DBCSectionEnvVarData {
  int type;
  uint32_t dataSize;
  const char* name;
};

struct DBCSectionEnvVarValue {
  int type;
  const char* name;
  const DBCValDesc* valDesc;
  size_t count;
};

struct DBCSectionComment {
  int type;
  DBCTarget target;
  const char* value;
};

struct DBCSectionAttrDefinition {
  int type;
  DBCObjectType objectType;
  struct DBCAttrValueInfo value;
  const char* name;
};

struct DBCSectionAttrDefault {
  int type;
  const char* name;
  DBCVariant value;
};

struct DBCSectionAttr {
  int type;
  const char* name;
  DBCTarget target;
  DBCVariant value;
};

struct DBCSectionAttrRel {
  int type;
  const char* name;
  const char* netNode;
  DBCTarget target;
  DBCVariant value;
};

struct DBCSectionSignalMultiplexed {
  int type;
  uint32_t messageId;
  const char* signalName;
  const char* switchName;
  const DBCRange* range;
  size_t count;
};

typedef union DBCSection DBCSection;
union DBCSection {
  enum DBCSectionType type;
  struct DBCSectionVersion version;
  struct DBCSectionNewSymbols newSymbols;
  struct DBCSectionBitTiming bitTiming;
  struct DBCSectionNetNodes netNodes;
  struct DBCSectionValTable valTable;
  struct DBCSectionMessage message;
  struct DBCSectionMessageTransmitter messageTransmitter;
  struct DBCSectionSignal signal;
  struct DBCSectionSignalType signalType;
  struct DBCSectionSignalTypeRef signalTypeRef;
  struct DBCSectionSignalGroup signalGroup;
  struct DBCSectionSignalValue signalValue;
  struct DBCSectionSignalValueType signalValueType;
  struct DBCSectionSignalMultiplexed signalMultiplexed;
  struct DBCSectionEnvVar envVar;
  struct DBCSectionEnvVarData envVarData;
  struct DBCSectionEnvVarValue envVarValue;
  struct DBCSectionComment comment;
  struct DBCSectionAttr attr;
  struct DBCSectionAttrRel attrRel;
  struct DBCSectionAttrDefinition attrDefinition;
  struct DBCSectionAttrDefault attrDefault;
};

typedef bool (DBCParserCallback) (void* userdata, const DBCSection* Section);

typedef struct DBCParser DBCParser;

DBCParser* DBCParser_create(void* userdata, DBCParserCallback* callback);
DBCParser* DBCParser_create2(void* userdata, DBCParserCallback* callback, void* allocator, DBCAllocFn* alloc, DBCFreeFn* free);
void DBCParser_destroy(DBCParser*);

bool DBCParser_append(DBCParser*, const char* data, size_t size);
bool DBCParser_finish(DBCParser*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _DBC_PARSER_H */
