/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "DBCDocument.h"

#include <string.h>

#include "dbc/DBCAssert.h"
#include "dbc/DBCLog.h"

typedef void DBCObject;
typedef uint32_t DBCClassId;

typedef struct DBCClass DBCClass;
typedef struct DBCContext DBCContext;
typedef struct DBCString DBCString;
typedef struct DBCArray DBCArray;
typedef struct DBCRanges DBCRanges;
typedef struct DBCSymbols DBCSymbols;
typedef struct DBCAttrDefines DBCAttrDefines;
typedef struct DBCAttrMap DBCAttrMap;
typedef union DBCAttrVariant DBCAttrVariant;

enum {
  kDBCDocumentIndex,
  kDBCNetNodeIndex,
  kDBCMessageIndex,
  kDBCSignalIndex,
  kDBCEnvVarIndex,
  kDBCSigGroupIndex,
  kDBCValTableIndex,
  kDBCSigTypeIndex,
};

#define DBC_OBJECT_MAX 8
static const char* s_desc[DBC_OBJECT_MAX] = {
  "Document",
  "Network Node",
  "Message",
  "Signal",
  "Environment Variable",
  "Signal Group",
  "Value Table",
  "Signal Type",
};

#define DBC_ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

struct DBCString {
  char* characters;
};

static void DBCString_init(DBCContext* context, DBCString* thiz);
static void DBCString_finalize(DBCContext* context, DBCString* thiz);
static bool DBCString_setCharacters(DBCContext* context, DBCString* thiz, const char* characters);

struct DBCSymbols {
  size_t count;
  char** items;
};

static void DBCSymbols_init(DBCContext* context, DBCSymbols* thiz);
static bool DBCSymbols_set(DBCContext* context, DBCSymbols* thiz, const char** symbols, size_t count);
static void DBCSymbols_finalize(DBCContext* context, DBCSymbols* thiz);

#define kDBCArrayStartCapacity (32)
#define kDBCArrayMaxSize (UINT16_MAX)

struct DBCArray {
  DBCObject** objects;
  uint32_t size;
  uint32_t capacity;
};

static void DBCArray_init(DBCContext*, DBCArray*);
static void DBCArray_finalize(DBCContext*, DBCArray*);
static bool DBCArray_reserve(DBCContext*, DBCArray*, size_t newCapacity);
static bool DBCArray_append(DBCContext*, DBCArray*, DBCObject* object);

union DBCAttrVariant {
  int64_t vInt;
  double vFloat;
  DBCString vString;
};

struct DBCAttribute {
  DBCAttrProto* proto;
  DBCAttrVariant variant;
};

struct DBCAttrMap {
  DBCAttribute* data;
  uint16_t size;
  uint16_t capacity;
};

struct DBCAttrProto {
  uint32_t refCount;
  DBCClass* relateClass;

  char* name;
  struct DBCAttrValueInfo params;

  bool hasDefaultVariant;
  DBCAttrVariant defaultVariant;
};

static void DBCAttrMap_init(DBCContext*, DBCAttrMap*);
static void DBCAttrMap_finalize(DBCContext*, DBCAttrMap*);

struct DBCAttrDefines {
  DBCAttrProto** data;
  uint16_t size;
  uint16_t capacity;
};

static void DBCAttrDefines_init(DBCContext*, DBCAttrDefines*);
static void DBCAttrDefines_finalize(DBCContext*, DBCAttrDefines*);
static DBCAttrProto* DBCAttrDefines_find(DBCContext*, const DBCAttrDefines*, const char* name);
static bool DBCAttrDefines_add(DBCContext* context, DBCAttrDefines* thiz, DBCAttrProto* attr);
static bool DBCAttrDefines_remove(DBCContext* context, DBCAttrDefines* thiz, DBCAttrProto* attr);

struct DBCClass {
  DBCClassId id;
  DBCDocument* document;
  DBCContext* context;
};

#define kDBCRangesStartCapacity (8)
#define kDBCRangesMaxSize (64)

struct DBCRanges {
  uint16_t count;
  uint16_t capacity;
  union {
    DBCRange rangeInline;
    DBCRange* rangePointer;
  };
};

static void DBCRanges_init(DBCContext*, DBCRanges*);
static void DBCRanges_finalize(DBCContext*, DBCRanges*);
static bool DBCRanges_add(DBCContext*, DBCRanges*, uint32_t first, uint32_t second);

struct DBCContext {
  void* allocator;
  DBCAllocFn* alloc;
  DBCFreeFn* free;
  DBCAttrDefines attrProtos;
  struct DBCClass classes[DBC_OBJECT_MAX];
};

static inline void* DBCAlloc(DBCContext* context, size_t size)
{
  return context->alloc(context->allocator, size);
}

static inline void DBCFree(DBCContext* context, void* ptr)
{
  context->free(context->allocator, ptr);
}

/* DBCString */

void DBCString_init(DBCContext* context, DBCString* thiz)
{
  thiz->characters = NULL;
}

void DBCString_finalize(DBCContext* context, DBCString* thiz)
{
  if (thiz->characters != NULL)
    DBCFree(context, thiz->characters);
}

bool DBCString_setCharacters(DBCContext* context, DBCString* thiz, const char* characters)
{
  char* str;
  if (characters == NULL)
    str = NULL;
  else {
    size_t lenz = strlen(characters) + 1;
    str = DBCAlloc(context, lenz);
    if (str == NULL)
      return false;
    memcpy(str, characters, lenz);
  }

  if (thiz->characters != NULL)
    DBCFree(context, thiz->characters);

  thiz->characters = str;
  return true;
}

/* DBCSymbols */

void DBCSymbols_init(DBCContext* context, DBCSymbols* thiz)
{
  thiz->items = NULL;
  thiz->count = 0;
}

bool DBCSymbols_set(DBCContext* context, DBCSymbols* thiz, const char** symbols, size_t count)
{
  char c;
  size_t i;

  size_t totalInBytes = sizeof(char*);
  for (i = 0; i < count; i++) {
    totalInBytes += sizeof(char*);
    totalInBytes += strlen(symbols[i]) + 1;
  }

  char* dst = (char*)DBCAlloc(context, totalInBytes);
  if (dst == NULL)
    return false;

  if (thiz->items != NULL)
    DBCFree(context, thiz->items);

  thiz->count = count;
  thiz->items = (char**)dst;
  dst += sizeof(char*) * (count + 1);

  for (i = 0; i < count; i++) {
    const char* src = symbols[i];
    thiz->items[i] = dst;
    do {
      c = *dst++ = *src++;
    } while (c != '\0');
  }

  thiz->items[count] = NULL;
  DBC_ASSERT(((char*)thiz->items + totalInBytes) == dst);
  return true;
}

void DBCSymbols_finalize(DBCContext* context, DBCSymbols* thiz)
{
  if (thiz->items != NULL)
    DBCFree(context, thiz->items);
}

struct DBCObjectBase {
  DBCClassId classId;
  uint32_t refCount;
  const char* name;
  DBCContext* context;
};

struct DBCObjectChild {
  DBCClassId classId;
  uint32_t refCount;
  const char* name;
  DBCContext* context;
  DBCObject* parent;
};

struct DBCObjectAttr {
  DBCClassId classId;
  uint32_t refCount;
  const char* name;
  DBCContext* context;
  DBCObject* parent;
  DBCString comment;
  DBCAttrMap attributes;
};

struct DBCValTable {
  struct DBCObjectBase base;

  uint32_t size;
  uint32_t* value;
  char** description;
};

struct DBCSigType {
  struct DBCObjectBase base;

  DBCSigInfo info;
  double defaultValue;
  DBCValTable* valTable;
};

struct DBCSigGroup {
  struct DBCObjectChild base;

  uint32_t repetitions;
  DBCArray signals;
};

struct DBCNetNode {
  struct DBCObjectAttr base;
};

struct DBCMessage {
  struct DBCObjectAttr base;

  uint32_t id;
  uint32_t sizeInBytes;

  DBCArray groups;
  DBCArray signals;
  DBCArray transmitters;

  bool isPseudo;
};

struct DBCSignal {
  struct DBCObjectAttr base;

  DBCSigValueType valueType;

  struct {
    DBCSignal* signal; // "M"
    DBCRanges ranges;  // "m"
  } multiplexer;

  uint32_t startBit;
  DBCSigInfo info;
  DBCArray receivers;

  DBCValTable* valTable;
  DBCSigGroup* sigGroup;
  DBCSigType* sigType;
};

struct DBCEnvVar {
  struct DBCObjectAttr base;

  DBCEnvInfo info;
  uint32_t dataSize;
  DBCValTable* valTable;
  DBCArray accessNodes;
};

struct DBCDocument {
  struct DBCObjectAttr base;

  DBCString version;
  DBCSymbols symbols;

  bool hasBitTiming;
  DBCBitTiming bitTiming;

  DBCArray netNodes;
  DBCArray messages; // DBCAvlTree
  DBCArray envVars;
  DBCArray valTables;
  DBCArray sigTypes;
  
  DBCContext context;
};

static DBCNetNode* DBCNetNode_create(DBCContext*, const char* name);
static DBCMessage* DBCMessage_create(DBCContext*, const char* name, uint32_t id, uint32_t sizeInBytes);
static DBCSignal* DBCSignal_create(DBCContext*, const char* name, uint32_t startBit, const DBCSigInfo* info);
static DBCEnvVar* DBCEnvVar_create(DBCContext*, const char* name, const DBCEnvInfo* info);
static DBCSigGroup* DBCSigGroup_create(DBCContext*, const char* name, uint32_t repetitions);
static DBCValTable* DBCValTable_create(DBCContext*, const char* name, const DBCValDesc* items, size_t size);
static DBCSigType* DBCSigType_create(DBCContext*, const char* name, const DBCSigInfo* info, double defaultValue);

static void DBCDocument_deinit(DBCDocument*);
static void DBCNetNode_deinit(DBCNetNode*);
static void DBCMessage_deinit(DBCMessage*);
static void DBCSignal_deinit(DBCSignal*);
static void DBCEnvVar_deinit(DBCEnvVar*);
static void DBCSigGroup_deinit(DBCSigGroup*);
static void DBCValTable_deinit(DBCValTable*);
static void DBCSigType_deinit(DBCSigType*);

/* DBCRanges */

void DBCRanges_init(DBCContext* context, DBCRanges* thiz)
{
  thiz->count = 0;
  thiz->capacity = 0;
  thiz->rangeInline.first = 0;
  thiz->rangeInline.second = 0;
}

void DBCRanges_finalize(DBCContext* context, DBCRanges* thiz)
{
  if (thiz->count > 1)
    DBCFree(context, thiz->rangePointer);
}

bool DBCRanges_add(DBCContext* context, DBCRanges* thiz, uint32_t first, uint32_t second)
{
  if (thiz->count >= kDBCRangesMaxSize)
    return false;

  if (second < first) {
    uint32_t temp = first;
    first = second;
    second = temp;
  }

  if (thiz->capacity == 0) {
    if (thiz->count == 0) {
      thiz->rangeInline.first = first;
      thiz->rangeInline.second = second;
    }
    else {
      DBC_ASSERT(thiz->count == 1);
      DBCRange* newPointer = DBCAlloc(context, sizeof(DBCRange) * kDBCRangesStartCapacity);
      if (newPointer == NULL)
        return false;
      newPointer[0] = thiz->rangeInline;
      newPointer[1].first = first;
      newPointer[1].second = second;
      thiz->rangePointer = newPointer;
    }
  }
  else {
    if (thiz->capacity <= thiz->count) {
      uint16_t newCapacity = (uint16_t)((size_t)thiz->capacity * 2 / 3);
      if (kDBCRangesMaxSize < newCapacity)
        newCapacity = kDBCArrayMaxSize;

      DBCRange* newPointer = DBCAlloc(context, sizeof(DBCRange) * newCapacity);
      if (newPointer == NULL)
        return false;

      if (thiz->count > 0)
        memcpy(newPointer, thiz->rangePointer, sizeof(DBCRange) * thiz->count);

      DBCFree(context, thiz->rangePointer);
      thiz->rangePointer = newPointer;
    }
    thiz->rangePointer[thiz->count].first = first;
    thiz->rangePointer[thiz->count].second = second;
  }

  thiz->count++;
  return true;
}

static bool DBCAttrSetInt(DBCAttrProto* proto, DBCAttrVariant* variant, int64_t value)
{
  switch (proto->params.type) {
  case kDBCAttrValueInt:
    if (proto->params.vInt.first <= value && value <= proto->params.vInt.second) {
      variant->vInt = value;
      return true;
    }
    if (proto->params.vInt.first == 0 && 0 == proto->params.vInt.second) {
      variant->vInt = value;
      return true;
    }
    break;

  case kDBCAttrValueHex:
    if (proto->params.vHex.first <= value && value <= proto->params.vHex.second) {
      variant->vInt = value;
      return true;
    }
    if (proto->params.vHex.first == 0 && 0 == proto->params.vHex.second) {
      variant->vInt = value;
      return true;
    }
    break;

  case kDBCAttrValueFloat:
    variant->vFloat = (double)value;
    return true;

  case kDBCAttrValueEnum:
    if (0 <= value && value < (int64_t)proto->params.vEnum.count) {
      variant->vInt = value;
      return true;
    }
    break;

  default:
    break;
  }

  return false;
}

static bool DBCAttrSetFloat(DBCAttrProto* proto, DBCAttrVariant* variant, double value)
{
  switch (proto->params.type) {
  case kDBCAttrValueFloat:
    if (proto->params.vFloat.first <= value && value <= proto->params.vFloat.second) {
      variant->vFloat = value;
      return true;
    }
    if (proto->params.vFloat.first == 0.0 && 0.0 == proto->params.vFloat.second) {
      variant->vFloat = value;
      return true;
    }
    break;

  default:
    break;
  }

  return false;
}

static bool DBCAttrSetString(DBCAttrProto* proto, DBCAttrVariant* variant, const char* value)
{
  switch (proto->params.type) {
  case kDBCAttrValueString:
    return DBCString_setCharacters(proto->relateClass->context, &variant->vString, value);

  case kDBCAttrValueEnum:  {
    uint16_t i;
    for (i = 0; i < proto->params.vEnum.count; i++) {
      if (!strcmp(proto->params.vEnum.data[i], value)) {
        variant->vInt = i;
        return true;
      }
    }
    if (*value == '\0') {
      DBC_LOGW("An empty string is treated as 0 %s of '%s' Attribute", DBCAttrValueType_toCString(proto->params.type), proto->name);
      variant->vInt = 0;
      return true;
    }
    break;
  }

  default:
    break;
  }

  DBC_LOGE("Can't set '%s' string to %s '%s' Attribute", value, DBCAttrValueType_toCString(proto->params.type), proto->name);
  return false;
}

static DBCAttrProto* DBCAttrProto_create(DBCClass* clazz, const char* name, const struct DBCAttrValueInfo* params)
{
  DBCAttrProto* thiz;
  const size_t classSize = DBC_ALIGN_UP(sizeof(*thiz), sizeof(void*));

  size_t payloadSize = 0;

  if (params->type == kDBCAttrValueEnum) {
    for (size_t i = 0; i < params->vEnum.count; i++) {
      const char* desc = params->vEnum.data[i];
      payloadSize += strlen(desc) + 1;
    }
    payloadSize += params->vEnum.count * sizeof(char*);
  }

  size_t nlenz = strlen(name) + 1;
  thiz = (DBCAttrProto*)DBCAlloc(clazz->context, classSize + payloadSize + nlenz);
  if (thiz == NULL)
    return NULL;

  thiz->refCount = 1;
  thiz->relateClass = clazz;

  thiz->params = *params;
  thiz->hasDefaultVariant = false;
  memset(&thiz->defaultVariant, 0, sizeof(thiz->defaultVariant));

  char* str = (char*)thiz + classSize;
  if (params->type == kDBCAttrValueEnum) {
    thiz->params.vEnum.data = (const char**)str;
    str += params->vEnum.count * sizeof(char*);

    for (size_t i = 0; i < params->vEnum.count; i++) {
      const char* desc = params->vEnum.data[i];
      size_t nz = strlen(desc) + 1;
      thiz->params.vEnum.data[i] = str;
      memcpy(str, desc, nz);
      str += nz;
    }
  }

  memcpy(str, name, nlenz);
  thiz->name = str;
  str += nlenz;

  DBC_ASSERT(str == ((char*)thiz + classSize + payloadSize + nlenz));

  return thiz;
}

static void DBCAttrProto_retain(DBCContext* context, DBCAttrProto* thiz)
{
  thiz->refCount++;
}

static void DBCAttrProto_release(DBCContext* context, DBCAttrProto* thiz)
{
  if (--thiz->refCount == 0) {
    if (thiz->params.type == kDBCAttrValueString)
      DBCString_finalize(context, &thiz->defaultVariant.vString);
    DBCFree(context, thiz);
  }
}

bool DBCAttrProto_isRelateDocument(const DBCAttrProto* thiz)
{
  return thiz->relateClass->id == kDBCDocumentIndex;
}

bool DBCAttrProto_isRelateNetNode(const DBCAttrProto* thiz)
{
  return thiz->relateClass->id == kDBCNetNodeIndex;
}

bool DBCAttrProto_isRelateMessage(const DBCAttrProto* thiz)
{
  return thiz->relateClass->id == kDBCMessageIndex;
}

bool DBCAttrProto_isRelateSignal(const DBCAttrProto* thiz)
{
  return thiz->relateClass->id == kDBCSignalIndex;
}

bool DBCAttrProto_isRelateEnvVar(const DBCAttrProto* thiz)
{
  return thiz->relateClass->id == kDBCEnvVarIndex;
}

bool DBCAttrProto_hasDefault(const DBCAttrProto* thiz)
{
  return thiz->hasDefaultVariant;
}

bool DBCAttrProto_setDefaultInt(DBCAttrProto* thiz, int64_t value)
{
  if (!DBCAttrSetInt(thiz, &thiz->defaultVariant, value))
    return false;
  thiz->hasDefaultVariant = true;
  return true;
}

bool DBCAttrProto_setDefaultFloat(DBCAttrProto* thiz, double value)
{
  if (!DBCAttrSetFloat(thiz, &thiz->defaultVariant, value))
    return false;
  thiz->hasDefaultVariant = true;
  return true;
}

bool DBCAttrProto_setDefaultString(DBCAttrProto* thiz, const char* value)
{
  if (!DBCAttrSetString(thiz, &thiz->defaultVariant, value))
    return false;
  thiz->hasDefaultVariant = true;
  return true;
}

static void DBCAttribute_init(DBCContext* context, DBCAttribute* thiz, DBCAttrProto* proto)
{
  thiz->proto = proto;
  DBCAttrProto_retain(context, proto);

  switch (proto->params.type) {
  case kDBCAttrValueInt:
  case kDBCAttrValueHex:
  case kDBCAttrValueEnum:
    thiz->variant.vInt = 0;
    break;
  case kDBCAttrValueFloat:
    thiz->variant.vFloat = 0;
    break;
  case kDBCAttrValueString:
    DBCString_init(context, &thiz->variant.vString);
    break;
  default:
    DBC_ASSERT_NOT_REACHED();
    break;
  }
}

static void DBCAttribute_finalize(DBCContext* context, DBCAttribute* thiz)
{
  if (thiz->proto->params.type == kDBCAttrValueString)
    DBCString_finalize(context, &thiz->variant.vString);
  DBCAttrProto_release(context, thiz->proto);
}

DBCAttrValueType DBCAttribute_type(const DBCAttribute* thiz)
{
  return thiz->proto->params.type;
}

const char* DBCAttribute_name(const DBCAttribute* thiz)
{
  return thiz->proto->name;
}

int64_t DBCAttribute_asIntValue(const DBCAttribute* thiz)
{
  switch (thiz->proto->params.type) {
  case kDBCAttrValueInt:
  case kDBCAttrValueHex:
    return thiz->variant.vInt;
  default:
    DBC_ASSERT_NOT_REACHED();
  }
  return 0;
}

double DBCAttribute_asFloatValue(const DBCAttribute* thiz)
{
  DBC_ASSERT(thiz->proto->params.type == kDBCAttrValueFloat);
  return thiz->variant.vFloat;
}

const char* DBCAttribute_asStringValue(const DBCAttribute* thiz)
{
  switch (thiz->proto->params.type) {
  case kDBCAttrValueString:
    return thiz->variant.vString.characters;
  case kDBCAttrValueEnum:
    return thiz->proto->params.vEnum.data[thiz->variant.vInt];
  default:
    DBC_ASSERT_NOT_REACHED();
  }
  return kDBCStrEmpty;
}

void DBCAttrDefines_init(DBCContext* context, DBCAttrDefines* thiz)
{
  thiz->data = NULL;
  thiz->size = 0;
  thiz->capacity = 0;
}

void DBCAttrDefines_finalize(DBCContext* context, DBCAttrDefines* thiz)
{
  uint16_t i;
  for (i = 0; i < thiz->size; i++) {
    DBCAttrProto_release(context, thiz->data[i]);
  }
  DBCFree(context, thiz->data);
}

DBCAttrProto* DBCAttrDefines_find(DBCContext* context, const DBCAttrDefines* thiz, const char* name)
{
  uint16_t i;
  for (i = 0; i < thiz->size; i++) {
    if (!strcmp(thiz->data[i]->name, name))
      return thiz->data[i];
  }
  return NULL;
}

bool DBCAttrDefines_reserve(DBCContext* context, DBCAttrDefines* thiz, size_t newCapacity)
{
  if (kDBCArrayMaxSize < newCapacity)
    return false;

  if (thiz->capacity < newCapacity) {
    DBCAttrProto** newData = (DBCAttrProto**)DBCAlloc(context, newCapacity * sizeof(*thiz->data));
    if (newData == NULL)
      return false;

    if (thiz->size != 0) {
      memcpy(newData, thiz->data, thiz->size * sizeof(*thiz->data));
      DBCFree(context, thiz->data);
    }

    thiz->data = newData;
    thiz->capacity = (uint16_t)newCapacity;
  }

  return true;
}

bool DBCAttrDefines_add(DBCContext* context, DBCAttrDefines* thiz, DBCAttrProto* attr)
{
  if (thiz->capacity <= thiz->size) {
    if (kDBCArrayMaxSize == thiz->capacity)
      return false;

    uint16_t newCapacity;
    if (thiz->capacity == 0)
      newCapacity = kDBCArrayStartCapacity;
    else
      newCapacity = (uint16_t)((size_t)thiz->capacity * 2 / 3);

    if (newCapacity < thiz->capacity)
      newCapacity = kDBCArrayMaxSize;

    if (!DBCAttrDefines_reserve(context, thiz, newCapacity))
      return false;
  }

  DBCAttrProto_retain(context, attr);
  thiz->data[thiz->size++] = attr;

  return true;
}

bool DBCAttrDefines_remove(DBCContext* context, DBCAttrDefines* thiz, DBCAttrProto* attr)
{
  for (uint16_t i = 0; i < thiz->size; i++) {
    if (thiz->data[i] == attr) {
      DBCAttrProto_release(context, attr);
      while ((i + 1) < thiz->size) {
        thiz->data[i] = thiz->data[i + 1];
        i++;
      }
      thiz->size--;
      return true;
    }
  }
  return false;
}

void DBCAttrMap_init(DBCContext* context, DBCAttrMap* thiz)
{
  thiz->data = NULL;
  thiz->size = 0;
  thiz->capacity = 0;
}

void DBCAttrMap_finalize(DBCContext* context, DBCAttrMap* thiz)
{
  uint16_t i;
  for (i = 0; i < thiz->size; i++) {
    DBCAttribute_finalize(context, &thiz->data[i]);
  }
  DBCFree(context, thiz->data);
}

static bool DBCAttrMap_reserve(DBCContext* context, DBCAttrMap* thiz, size_t newCapacity)
{
  if (kDBCArrayMaxSize < newCapacity)
    return false;

  if (thiz->capacity < newCapacity) {
    DBCAttribute* newData = (DBCAttribute*)DBCAlloc(context, newCapacity * sizeof(*thiz->data));
    if (newData == NULL)
      return false;

    if (thiz->size != 0) {
      memcpy(newData, thiz->data, thiz->size * sizeof(*thiz->data));
      DBCFree(context, thiz->data);
    }

    thiz->data = newData;
    thiz->capacity = (uint16_t)newCapacity;
  }

  return true;
}

static bool DBCAttrMap_add(DBCContext* context, DBCAttrMap* thiz, DBCAttribute* attr)
{
  if (thiz->capacity <= thiz->size) {
    if (kDBCArrayMaxSize == thiz->capacity) {
      return false;
    }

    uint16_t newCapacity;
    if (thiz->capacity == 0)
      newCapacity = kDBCArrayStartCapacity;
    else
      newCapacity = (uint16_t)((size_t)thiz->capacity * 2 / 3);

    if (newCapacity < thiz->capacity)
      newCapacity = kDBCArrayMaxSize;

    if (!DBCAttrMap_reserve(context, thiz, newCapacity)) {
      return false;
    }
  }

  memcpy(&thiz->data[thiz->size], attr, sizeof(*attr));
  thiz->size++;
  return true;
}

static DBCAttribute* DBCAttrMap_find(DBCContext* context, const DBCAttrMap* thiz, const char* name)
{
  uint16_t i;
  for (i = 0; i < thiz->size; i++) {
    if (!strcmp(thiz->data[i].proto->name, name))
      return &thiz->data[i];
  }
  return NULL;
}

static inline void DBCObjectBase_init(DBCContext* context, struct DBCObjectBase* thiz, DBCClassId id)
{
  thiz->name = NULL;
  thiz->classId = id;
  thiz->refCount = 1;
  thiz->context = context;
}

static inline void DBCObjectChild_init(DBCContext* context, struct DBCObjectChild* thiz, DBCClassId id)
{
  DBCObjectBase_init(context, (struct DBCObjectBase*)thiz, id);
  thiz->parent = NULL;
}

static inline void DBCObjectAttr_init(DBCContext* context, struct DBCObjectAttr* thiz, DBCClassId id)
{
  DBCObjectChild_init(context, (struct DBCObjectChild*)thiz, id);
  DBCString_init(context, &thiz->comment);
  DBCAttrMap_init(context, &thiz->attributes);
}

static void DBCObjectAttr_finalize(DBCContext* context, struct DBCObjectAttr* thiz)
{
  DBCString_finalize(context, &thiz->comment);
  DBCAttrMap_finalize(context, &thiz->attributes);
}

static bool DBCObjectAttr_getAttrInt(const struct DBCObjectAttr* thiz, const char* name, int64_t* result)
{
  DBCAttrProto* proto;
  DBCAttrVariant* variant;

  DBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    proto = attr->proto;
    variant = &attr->variant;
  }
  else {
    proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
    variant = (proto != NULL && proto->hasDefaultVariant && proto->relateClass->id == thiz->classId) ? &proto->defaultVariant : NULL;
  }

  bool success = false;
  int64_t value = 0;

  if (variant != NULL) {
    switch (proto->params.type) {
    case kDBCAttrValueInt:
    case kDBCAttrValueHex:
      success = true;
      value = variant->vInt;
      break;
    }
  }

  if (success && result != NULL)
    *result = value;

  return success;
}

static bool DBCObjectAttr_setAttrInt(struct DBCObjectAttr* thiz, const char* name, int64_t value)
{
  DBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    return DBCAttrSetInt(attr->proto, &attr->variant, value);
  }

  DBCAttrProto* proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
  if (proto == NULL)
    return false;

  if (thiz->classId != proto->relateClass->id)
    return false;

  DBCAttribute temp;
  DBCAttribute_init(thiz->context, &temp, proto);
  if (!DBCAttrSetInt(temp.proto, &temp.variant, value)) {
    DBCAttribute_finalize(thiz->context, &temp);
    return false;
  }

  return DBCAttrMap_add(thiz->context, &thiz->attributes, &temp);
}

static bool DBCObjectAttr_getAttrFloat(const struct DBCObjectAttr* thiz, const char* name, double* result)
{
  DBCAttrProto* proto;
  DBCAttrVariant* variant;

  DBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    proto = attr->proto;
    variant = &attr->variant;
  }
  else {
    proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
    variant = (proto != NULL && proto->hasDefaultVariant && proto->relateClass->id == thiz->classId) ? &proto->defaultVariant : NULL;
  }

  bool success = false;
  double value = 0;

  if (variant != NULL) {
    switch (proto->params.type) {
    case kDBCAttrValueInt:
    case kDBCAttrValueHex:
    case kDBCAttrValueEnum:
      success = true;
      value = (double)variant->vInt;
      break;
    case kDBCAttrValueFloat:
      success = true;
      value = variant->vFloat;
      break;
    }
  }

  if (success && result != NULL)
    *result = value;

  return success;
}

static bool DBCObjectAttr_setAttrFloat(struct DBCObjectAttr* thiz, const char* name, double value)
{
  DBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    return DBCAttrSetFloat(attr->proto, &attr->variant, value);
  }

  DBCAttrProto* proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
  if (proto == NULL)
    return false;

  if (thiz->classId != proto->relateClass->id)
    return false;

  DBCAttribute temp;
  DBCAttribute_init(thiz->context, &temp, proto);
  if (!DBCAttrSetFloat(temp.proto, &temp.variant, value)) {
    DBCAttribute_finalize(thiz->context, &temp);
    return false;
  }

  return DBCAttrMap_add(thiz->context, &thiz->attributes, &temp);
}

static bool DBCObjectAttr_getAttrString(const struct DBCObjectAttr* thiz, const char* name, const char** result)
{
  DBCAttrProto* proto;
  DBCAttrVariant* variant;

  DBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    proto = attr->proto;
    variant = &attr->variant;
  }
  else {
    proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
    variant = (proto != NULL && proto->hasDefaultVariant && proto->relateClass->id == thiz->classId) ? &proto->defaultVariant : NULL;
  }

  bool success = false;
  const char* value = kDBCStrEmpty;

  if (variant != NULL) {
    switch (proto->params.type) {
    case kDBCAttrValueEnum:
      success = true;
      value = proto->params.vEnum.data[variant->vInt];
      break;
    case kDBCAttrValueString:
      success = true;
      value = variant->vString.characters;
      break;
    }
  }

  if (success && result != NULL)
    *result = value;

  return success;
}

static bool DBCObjectAttr_setAttrString(struct DBCObjectAttr* thiz, const char* name, const char* value)
{
  DBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    return DBCAttrSetString(attr->proto, &attr->variant, value);
  }

  DBCAttrProto* proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
  if (proto == NULL)
    return false;

  if (thiz->classId != proto->relateClass->id)
    return false;

  DBCAttribute temp;
  DBCAttribute_init(thiz->context, &temp, proto);
  if (!DBCAttrSetString(temp.proto, &temp.variant, value)) {
    DBCAttribute_finalize(thiz->context, &temp);
    return false;
  }

  return DBCAttrMap_add(thiz->context, &thiz->attributes, &temp);
}

static inline void DBCObject_retain(DBCObject* o)
{
  ((struct DBCObjectBase*)o)->refCount++;
}

static inline void DBCObject_release(DBCObject* o)
{
  struct DBCObjectAttr* object = (struct DBCObjectAttr*)o;

  if (--object->refCount != 0)
    return;

  DBCContext* context = object->context;
  void* allocator = context->allocator;
  DBCFreeFn* freeFn = context->free;

  switch (object->classId) {
  case kDBCDocumentIndex:
    DBCDocument_deinit((DBCDocument*)o);
    break;

  case kDBCNetNodeIndex:
    DBCNetNode_deinit((DBCNetNode*)o);
    break;

  case kDBCMessageIndex:
    DBCMessage_deinit((DBCMessage*)o);
    break;

  case kDBCSignalIndex:
    DBCSignal_deinit((DBCSignal*)o);
    break;

  case kDBCEnvVarIndex:
    DBCEnvVar_deinit((DBCEnvVar*)o);
    break;

  case kDBCSigGroupIndex:
    DBCSigGroup_deinit((DBCSigGroup*)o);
    break;

  case kDBCValTableIndex:
    DBCValTable_deinit((DBCValTable*)o);
    break;

  case kDBCSigTypeIndex:
    DBCSigType_deinit((DBCSigType*)o);
    break;

  default:
    DBC_ASSERT_NOT_REACHED();
    break;
  }

  freeFn(allocator, o);
}

static inline DBCNetNode* toDBCNetNode(DBCObject* o)
{
  DBC_ASSERT(((struct DBCObjectBase*)o)->classId == kDBCNetNodeIndex);
  return (DBCNetNode*)o;
}

static inline DBCMessage* toDBCMessage(DBCObject* o)
{
  DBC_ASSERT(((struct DBCObjectBase*)o)->classId == kDBCMessageIndex);
  return (DBCMessage*)o;
}

static inline DBCSignal* toDBCSignal(DBCObject* o)
{
  DBC_ASSERT(((struct DBCObjectBase*)o)->classId == kDBCSignalIndex);
  return (DBCSignal*)o;
}

static inline DBCEnvVar* toDBCEnvVar(DBCObject* o)
{
  DBC_ASSERT(((struct DBCObjectBase*)o)->classId == kDBCEnvVarIndex);
  return (DBCEnvVar*)o;
}

static inline DBCSigType* toDBCSigType(DBCObject* o)
{
  DBC_ASSERT(((struct DBCObjectBase*)o)->classId == kDBCSigTypeIndex);
  return (DBCSigType*)o;
}

static inline DBCSigGroup* toDBCSigGroup(DBCObject* o)
{
  DBC_ASSERT(((struct DBCObjectBase*)o)->classId == kDBCSigGroupIndex);
  return (DBCSigGroup*)o;
}

static inline DBCValTable* toDBCValTable(DBCObject* o)
{
  DBC_ASSERT(((struct DBCObjectBase*)o)->classId == kDBCValTableIndex);
  return (DBCValTable*)o;
}

void DBCArray_init(DBCContext* context, DBCArray* thiz)
{
  thiz->objects = NULL;
  thiz->size = 0;
  thiz->capacity = 0;
}

void DBCArray_finalize(DBCContext* context, DBCArray* thiz)
{
  if (thiz->objects != NULL) {
    size_t index;
    for (index = 0; index < thiz->size; index++)
      DBCObject_release(thiz->objects[index]);
    DBCFree(context, thiz->objects);
  }
}

bool DBCArray_reserve(DBCContext* context, DBCArray* thiz, size_t newCapacity)
{
  if (kDBCArrayMaxSize < newCapacity)
    return false;

  if (thiz->capacity < newCapacity) {
    DBCObject** newObjects = (DBCObject**)DBCAlloc(context, newCapacity * sizeof(*thiz->objects));
    if (newObjects == NULL)
      return false;

    if (thiz->size != 0) {
      memcpy(newObjects, thiz->objects, thiz->size * sizeof(*thiz->objects));
      DBCFree(context, thiz->objects);
    }

    thiz->objects = newObjects;
    thiz->capacity = (uint32_t)newCapacity;
  }

  return true;
}

static inline DBCNetNode* DBCArray_at(DBCContext* context, DBCArray* thiz, size_t index)
{
  DBC_ASSERT(index < thiz->size);
  return thiz->objects[index];
}

bool DBCArray_append(DBCContext* context, DBCArray* thiz, DBCObject* object)
{
  if (thiz->capacity <= thiz->size) {
    if (kDBCArrayMaxSize == thiz->capacity)
      return false;

    uint32_t newCapacity;
    if (thiz->capacity == 0)
      newCapacity = kDBCArrayStartCapacity;
    else
      newCapacity = (uint32_t)((thiz->capacity * 3) / 2);

    if (newCapacity < thiz->capacity)
      newCapacity = kDBCArrayMaxSize;

    if (!DBCArray_reserve(context, thiz, newCapacity))
      return false;
  }

  thiz->objects[thiz->size++] = object;
  DBCObject_retain(object);
  return true;
}

DBCDocument* DBCDocument_create(const char* name)
{
  return DBCDocument_create2(name, NULL, &DBCAllocDefault, &DBCFreeDefault);
}

DBCDocument* DBCDocument_create2(const char* name, void* allocator, DBCAllocFn* alloc, DBCFreeFn* free)
{
  if (alloc == NULL) {
    return NULL;
  }
  if (free == NULL) {
    free = &DBCFreeNope;
  }

  DBC_ASSERT(alloc != NULL && free != NULL);

  size_t lenz = strlen(name) + 1;
  DBCDocument* thiz = (DBCDocument*)alloc(allocator, sizeof(struct DBCDocument) + lenz);
  if (thiz == NULL)
    return NULL;

  DBCClassId id;
  DBCClass* clazz;
  DBCContext* context = &thiz->context;

  context->allocator = allocator;
  context->alloc = alloc;
  context->free = free;

  for (id = 0; id < DBC_OBJECT_MAX; id++) {
    clazz = &context->classes[id];
    clazz->id = id;
    clazz->document = thiz;
    clazz->context = context;
    // DBCAttrDefines_init(context, &clazz->attrDefines);
  }

  clazz = &context->classes[kDBCDocumentIndex];

  DBCObjectAttr_init(context, &thiz->base, clazz->id);

  char* str = (char*)thiz + sizeof(struct DBCDocument);
  memcpy(str, name, lenz);
  thiz->base.name = str;

  DBCString_init(context, &thiz->version);
  DBCSymbols_init(context, &thiz->symbols);

  thiz->hasBitTiming = false;
  thiz->bitTiming.baudrate = 0;
  thiz->bitTiming.btr1 = 0;
  thiz->bitTiming.btr2 = 0;

  DBCAttrDefines_init(context, &thiz->context.attrProtos);

  DBCArray_init(context, &thiz->netNodes);
  DBCArray_init(context, &thiz->messages);
  DBCArray_init(context, &thiz->envVars);
  DBCArray_init(context, &thiz->valTables);
  DBCArray_init(context, &thiz->sigTypes);

  return thiz;
}

static void DBCDocument_deinit(DBCDocument* thiz)
{
  DBCContext* context = &thiz->context;

  DBCString_finalize(context, &thiz->version);
  DBCArray_finalize(context, &thiz->netNodes);
  DBCArray_finalize(context, &thiz->messages);
  DBCArray_finalize(context, &thiz->envVars);
  DBCArray_finalize(context, &thiz->valTables);
  DBCArray_finalize(context, &thiz->sigTypes);

  DBCAttrDefines_finalize(context, &thiz->context.attrProtos);
  DBCObjectAttr_finalize(context, &thiz->base);
}

void DBCDocument_release(DBCDocument* thiz)
{
  DBCObject_release(thiz);
}

const char* DBCDocument_name(const DBCDocument* thiz)
{
  return thiz->base.name;
}

const char* DBCDocument_comment(const DBCDocument* thiz)
{
  return thiz->base.comment.characters;
}

bool DBCDocument_setComment(DBCDocument* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

const DBCBitTiming* DBCDocument_bitTiming(const DBCDocument* thiz)
{
  return thiz->hasBitTiming ? &thiz->bitTiming : NULL;
}

DBCMessage* DBCDocument_messageAt(const DBCDocument* thiz, size_t index)
{
  DBC_ASSERT(index < thiz->messages.size);
  return toDBCMessage(thiz->messages.objects[index]);
}

size_t DBCDocument_messageCount(const DBCDocument* thiz)
{
  return thiz->messages.size;
}

DBCAttribute* DBCDocument_findAttribute(const DBCDocument* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t DBCDocument_attributeCount(const DBCDocument* thiz)
{
  return thiz->base.attributes.size;
}

DBCAttribute* DBCDocument_attributeAt(const DBCDocument* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

bool DBCDocument_getAttrInt(const DBCDocument* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool DBCDocument_setAttrInt(DBCDocument* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool DBCDocument_getAttrFloat(const DBCDocument* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool DBCDocument_setAttrFloat(DBCDocument* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool DBCDocument_getAttrString(const DBCDocument* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool DBCDocument_setAttrString(DBCDocument* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

DBCNetNode* DBCDocument_createNetNode(DBCDocument* thiz, const char* name)
{
  return DBCNetNode_create(thiz->base.context, name);
}

DBCMessage* DBCDocument_createMessage(DBCDocument* thiz, const char* name, uint32_t id, uint32_t sizeInBytes)
{
  return DBCMessage_create(thiz->base.context, name, id, sizeInBytes);
}

DBCSignal* DBCDocument_createSignal(DBCDocument* thiz, const char* name, uint32_t startBit, const DBCSigInfo* info)
{
  return DBCSignal_create(thiz->base.context, name, startBit, info);
}

DBCEnvVar* DBCDocument_createEnvVar(DBCDocument* thiz, const char* name, const DBCEnvInfo* info)
{
  return DBCEnvVar_create(thiz->base.context, name, info);
}

DBCSigGroup* DBCDocument_createSigGroup(DBCDocument* thiz, const char* name, uint32_t repetitions)
{
  return DBCSigGroup_create(thiz->base.context, name, repetitions);
}

DBCValTable* DBCDocument_createValTable(DBCDocument* thiz, const char* name, const DBCValDesc* items, size_t size)
{
  return DBCValTable_create(thiz->base.context, name, items, size);
}

DBCSigType* DBCDocument_createSigType(DBCDocument* thiz, const char* name, const DBCSigInfo* info, double defaultValue)
{
  return DBCSigType_create(thiz->base.context, name, info, defaultValue);
}

bool DBCDocument_addNetNode(DBCDocument* thiz, DBCNetNode* netNode)
{
  if (netNode == NULL || netNode->base.context != thiz->base.context)
    return false;

  if (netNode->base.parent != NULL)
    return false;

  if (netNode->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->netNodes, netNode))
    return false;

  netNode->base.parent = thiz;
  return true;
}

bool DBCDocument_addMessage(DBCDocument* thiz, DBCMessage* message)
{
  if (message == NULL || message->base.context != thiz->base.context)
    return false;

  if (message->base.parent != NULL)
    return false;

  if (message->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->messages, message))
    return false;

  message->base.parent = thiz;
  return true;
}

bool DBCDocument_addEnvVar(DBCDocument* thiz, DBCEnvVar* envVar)
{
  if (envVar == NULL || envVar->base.context != thiz->base.context)
    return false;

  if (envVar->base.parent != NULL)
    return false;

  if (envVar->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->envVars, envVar))
    return false;

  envVar->base.parent = thiz;
  return true;
}

bool DBCDocument_addValTable(DBCDocument* thiz, DBCValTable* valTable)
{
  if (valTable == NULL || valTable->base.context != thiz->base.context)
    return false;

  if (valTable->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->valTables, valTable))
    return false;

  return true;
}

bool DBCDocument_addSigType(DBCDocument* thiz, DBCSigType* sigType)
{
  if (sigType == NULL || sigType->base.context != thiz->base.context)
    return false;

  if (sigType->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->sigTypes, sigType))
    return false;

  return true;
}

static bool DBCDocument_defineAttr(DBCDocument* thiz, const char* name, DBCClassId classId, const struct DBCAttrValueInfo* params)
{
  DBCContext* context = thiz->base.context;

  if (DBCAttrDefines_find(context, &thiz->context.attrProtos, name) != NULL)
    return false;

  DBCClass* clazz = &context->classes[classId];
  DBCAttrProto* attr = DBCAttrProto_create(clazz, name, params);
  if (attr == NULL)
    return false;

  bool success = DBCAttrDefines_add(context, &thiz->context.attrProtos, attr);
  DBCAttrProto_release(context, attr);
  return success;
}

bool DBCDocument_defineDocumentAttr(DBCDocument* thiz, const char* name, const struct DBCAttrValueInfo* params)
{
  return DBCDocument_defineAttr(thiz, name, kDBCDocumentIndex, params);
}

bool DBCDocument_defineNetNodeAttr(DBCDocument* thiz, const char* name, const struct DBCAttrValueInfo* params)
{
  return DBCDocument_defineAttr(thiz, name, kDBCNetNodeIndex, params);
}

bool DBCDocument_defineMessageAttr(DBCDocument* thiz, const char* name, const struct DBCAttrValueInfo* params)
{
  return DBCDocument_defineAttr(thiz, name, kDBCMessageIndex, params);
}

bool DBCDocument_defineSignalAttr(DBCDocument* thiz, const char* name, const struct DBCAttrValueInfo* params)
{
  return DBCDocument_defineAttr(thiz, name, kDBCSignalIndex, params);
}

bool DBCDocument_defineEnvVarAttr(DBCDocument* thiz, const char* name, const struct DBCAttrValueInfo* params)
{
  return DBCDocument_defineAttr(thiz, name, kDBCEnvVarIndex, params);
}

DBCAttrProto* DBCDocument_findAttrProto(const DBCDocument* thiz, const char* name)
{
  const DBCAttrDefines* attrProtos = &thiz->context.attrProtos;
  for (size_t index = 0; index < attrProtos->size; index++) {
    DBCAttrProto* proto = attrProtos->data[index];
    if (!strcmp(proto->name, name))
      return proto;
  }
  return NULL;
}

size_t DBCDocument_attrProtoCount(const DBCDocument* thiz)
{
  return thiz->context.attrProtos.size;
}

DBCAttrProto* DBCDocument_attrProtoAt(const DBCDocument* thiz, size_t index)
{
  return thiz->context.attrProtos.data[index];
}

DBCNetNode* DBCNetNode_create(DBCContext* context, const char* name)
{
  size_t lenz = strlen(name) + 1;

  DBCNetNode* thiz = (DBCNetNode*)DBCAlloc(context, sizeof(struct DBCNetNode) + lenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectAttr_init(context, &thiz->base, kDBCNetNodeIndex);

  char* str = (char*)thiz + sizeof(struct DBCNetNode);
  memcpy(str, name, lenz);
  thiz->base.name = str;

  return thiz;
}

static void DBCNetNode_deinit(DBCNetNode* thiz)
{
  DBCContext* context = thiz->base.context;

  DBCObjectAttr_finalize(context, &thiz->base);
}

void DBCNetNode_release(DBCNetNode* thiz)
{
  DBCObject_release(thiz);
}

const char* DBCNetNode_name(const DBCNetNode* thiz)
{
  return thiz->base.name;
}

DBCAttribute* DBCNetNode_findAttribute(const DBCNetNode* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t DBCNetNode_attributeCount(const DBCNetNode* thiz)
{
  return thiz->base.attributes.size;
}

DBCAttribute* DBCNetNode_attributeAt(const DBCNetNode* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

bool DBCNetNode_setComment(DBCNetNode* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

bool DBCNetNode_getAttrInt(const DBCDocument* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool DBCNetNode_setAttrInt(DBCNetNode* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool DBCNetNode_getAttrFloat(const DBCNetNode* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool DBCNetNode_setAttrFloat(DBCNetNode* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool DBCNetNode_getAttrString(const DBCNetNode* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool DBCNetNode_setAttrString(DBCNetNode* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

bool DBCMessage_setComment(DBCMessage* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

bool DBCMessage_getAttrInt(const DBCMessage* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool DBCMessage_setAttrInt(DBCMessage* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool DBCMessage_getAttrFloat(const DBCMessage* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool DBCMessage_setAttrFloat(DBCMessage* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool DBCMessage_getAttrString(const DBCMessage* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool DBCMessage_setAttrString(DBCMessage* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

bool DBCMessage_addTransmitter(DBCMessage* thiz, DBCNetNode* transmitter)
{
  return DBCArray_append(thiz->base.context, &thiz->transmitters, transmitter);
}

DBCSignal* DBCSignal_create(DBCContext* context, const char* name, uint32_t startBit, const DBCSigInfo* info)
{
  size_t nlenz = strlen(name) + 1;
  size_t ulenz = strlen(info->unit) + 1;

  DBCSignal* thiz = (DBCSignal*)DBCAlloc(context, sizeof(struct DBCSignal) + nlenz + ulenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectAttr_init(context, &thiz->base, kDBCSignalIndex);

  char* str = (char*)thiz + sizeof(struct DBCSignal);
  memcpy(str, name, nlenz);
  thiz->base.name = str;

  thiz->valueType = kDBCSigValueDouble;
  thiz->multiplexer.signal = NULL;
  DBCRanges_init(context, &thiz->multiplexer.ranges);

  thiz->startBit = startBit;

  thiz->info.sizeInBits = info->sizeInBits;
  thiz->info.byteOrder = info->byteOrder;
  thiz->info.isUnsigned = info->isUnsigned;
  thiz->info.factor = info->factor;
  thiz->info.offset = info->offset;
  thiz->info.minimum = info->minimum;
  thiz->info.maximum = info->maximum;

  str += nlenz;
  memcpy(str, info->unit, ulenz);
  thiz->info.unit = str;

  thiz->valTable = NULL;
  thiz->sigGroup = NULL;
  thiz->sigType = NULL;

  DBCArray_init(context, &thiz->receivers);

  return thiz;
}

static void DBCSignal_deinit(DBCSignal* thiz)
{
  DBCContext* context = thiz->base.context;

  DBC_ASSERT(thiz->sigGroup == NULL);

  DBCSignal_setValTable(thiz, NULL);
  DBCSignal_setSignalType(thiz, NULL);
  DBCSignal_setMultiplexerSignal(thiz, NULL);
  DBCRanges_finalize(context, &thiz->multiplexer.ranges);
  DBCArray_finalize(context, &thiz->receivers);

  DBCObjectAttr_finalize(context, &thiz->base);
}

void DBCSignal_release(DBCSignal* thiz)
{
  DBCObject_release(thiz);
}

const char* DBCSignal_name(const DBCSignal* thiz)
{
  return thiz->base.name;
}

const char* DBCSignal_comment(const DBCSignal* thiz)
{
  return thiz->base.comment.characters;
}

DBCAttribute* DBCSignal_findAttribute(const DBCSignal* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t DBCSignal_attributeCount(const DBCSignal* thiz)
{
  return thiz->base.attributes.size;
}

DBCAttribute* DBCSignal_attributeAt(const DBCSignal* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

DBCSigGroup* DBCSignal_sigGroup(const DBCSignal* thiz)
{
  return thiz->sigGroup;
}

DBCSigValueType DBCSignal_valueType(const DBCSignal* thiz)
{
  return thiz->valueType;
}

uint32_t DBCSignal_startBit(const DBCSignal* thiz)
{
  return thiz->startBit;
}

uint32_t DBCSignal_sizeInBits(const DBCSignal* thiz)
{
  return thiz->info.sizeInBits;
}

DBCByteOrder DBCSignal_byteOrder(const DBCSignal* thiz)
{
  return thiz->info.byteOrder;
}

bool DBCSignal_isUnsigned(const DBCSignal* thiz)
{
  return thiz->info.isUnsigned;
}

double DBCSignal_factor(const DBCSignal* thiz)
{
  return thiz->info.factor;
}

double DBCSignal_offset(const DBCSignal* thiz)
{
  return thiz->info.offset;
}

double DBCSignal_minimum(const DBCSignal* thiz)
{
  return thiz->info.minimum;
}

double DBCSignal_maximum(const DBCSignal* thiz)
{
  return thiz->info.maximum;
}

const char* DBCSignal_unit(const DBCSignal* thiz)
{
  return thiz->info.unit;
}

size_t DBCSignal_receiverCount(const DBCSignal* thiz)
{
  return thiz->receivers.size;
}

DBCNetNode* DBCSignal_receiverAt(const DBCSignal* thiz, size_t index)
{
  if (index < thiz->receivers.size)
    return toDBCNetNode(thiz->receivers.objects[index]);
  return NULL;
}

bool DBCSignal_setComment(DBCSignal* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

bool DBCSignal_getAttrInt(const DBCSignal* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool DBCSignal_setAttrInt(DBCSignal* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool DBCSignal_getAttrFloat(const DBCSignal* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool DBCSignal_setAttrFloat(DBCSignal* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool DBCSignal_getAttrString(const DBCSignal* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool DBCSignal_setAttrString(DBCSignal* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

void DBCSignal_setValueType(DBCSignal* thiz, DBCSigValueType type)
{
  thiz->valueType = type;
}

bool DBCSignal_addReceiver(DBCSignal* thiz, DBCNetNode* receiver)
{
  return DBCArray_append(thiz->base.context, &thiz->receivers, receiver);
}

void DBCSignal_setValTable(DBCSignal* thiz, DBCValTable* valTable)
{
  if (thiz->valTable != NULL)
    DBCObject_release(thiz->valTable);

  if (valTable != NULL)
    DBCObject_retain(valTable);

  thiz->valTable = valTable;
}

void DBCSignal_setSignalType(DBCSignal* thiz, DBCSigType* sigType)
{
  if (thiz->sigType != NULL)
    DBCObject_release(thiz->sigType);

  if (sigType != NULL)
    DBCObject_retain(sigType);

  thiz->sigType = sigType;
}

bool DBCSignal_setMultiplexerSignal(DBCSignal* thiz, DBCSignal* multiplexerSignal)
{
  if (multiplexerSignal != NULL) {
    if (thiz->base.parent == NULL || thiz->base.parent != multiplexerSignal->base.parent)
      return false;
  }

  if (thiz->multiplexer.signal != NULL)
    DBCObject_release(thiz->multiplexer.signal);

  if (multiplexerSignal != NULL)
    DBCObject_retain(multiplexerSignal);

  thiz->multiplexer.signal = multiplexerSignal;
  return true;
}

bool DBCSignal_addMultiplexerRange(DBCSignal* thiz, uint32_t first, uint32_t second)
{
  return DBCRanges_add(thiz->base.context, &thiz->multiplexer.ranges, first, second);
}

DBCEnvVar* DBCEnvVar_create(DBCContext* context, const char* name, const DBCEnvInfo* info)
{
  size_t nlenz = strlen(name) + 1;
  size_t ulenz = strlen(info->unit) + 1;

  DBCEnvVar* thiz = (DBCEnvVar*)DBCAlloc(context, sizeof(struct DBCEnvVar) + nlenz + ulenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectAttr_init(context, &thiz->base, kDBCEnvVarIndex);

  char* str = (char*)thiz + sizeof(struct DBCEnvVar);
  memcpy(str, name, nlenz);
  thiz->base.name = str;

  thiz->info.id = info->id;
  thiz->info.valueType = info->valueType;
  thiz->info.accessType = info->accessType;
  thiz->info.minimum = info->minimum;
  thiz->info.maximum = info->maximum;
  thiz->info.initialValue = info->initialValue;

  str += nlenz;
  memcpy(str, info->unit, ulenz);
  thiz->info.unit = str;

  thiz->dataSize = 0;
  thiz->valTable = NULL;

  DBCArray_init(context, &thiz->accessNodes);

  return thiz;
}

static void DBCEnvVar_deinit(DBCEnvVar* thiz)
{
  DBCContext* context = thiz->base.context;

  DBCArray_finalize(context, &thiz->accessNodes);

  DBCObjectAttr_finalize(context, &thiz->base);
}

void DBCEnvVar_release(DBCEnvVar* thiz)
{
  DBCObject_release(thiz);
}

bool DBCEnvVar_setComment(DBCEnvVar* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

DBCAttribute* DBCEnvVar_findAttribute(const DBCEnvVar* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t DBCEnvVar_attributeCount(const DBCEnvVar* thiz)
{
  return thiz->base.attributes.size;
}

DBCAttribute* DBCEnvVar_attributeAt(const DBCEnvVar* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

bool DBCEnvVar_getAttrInt(const DBCEnvVar* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool DBCEnvVar_setAttrInt(DBCEnvVar* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool DBCEnvVar_getAttrFloat(const DBCEnvVar* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool DBCEnvVar_setAttrFloat(DBCEnvVar* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool DBCEnvVar_getAttrString(const DBCEnvVar* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool DBCEnvVar_setAttrString(DBCEnvVar* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

bool DBCEnvVar_addAccessNode(DBCEnvVar* thiz, DBCNetNode* node)
{
  return DBCArray_append(thiz->base.context, &thiz->accessNodes, node);
}

bool DBCEnvVar_setDataSize(DBCEnvVar* thiz, uint32_t size)
{
  thiz->dataSize = size;
  return true;
}

void DBCEnvVar_setValTable(DBCEnvVar* thiz, DBCValTable* valTable)
{
  if (thiz->valTable != NULL)
    DBCObject_release(thiz->valTable);

  if (valTable != NULL)
    DBCObject_retain(valTable);

  thiz->valTable = valTable;
}

DBCSigGroup* DBCSigGroup_create(DBCContext* context, const char* name, uint32_t repetitions)
{
  size_t lenz = strlen(name) + 1;

  DBCSigGroup* thiz = (DBCSigGroup*)DBCAlloc(context, sizeof(struct DBCSigGroup) + lenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectChild_init(context, &thiz->base, kDBCSigGroupIndex);

  char* str = (char*)thiz + sizeof(struct DBCSigGroup);
  memcpy(str, name, lenz);
  thiz->base.name = str;

  thiz->repetitions = repetitions;
  DBCArray_init(context, &thiz->signals);

  return thiz;
}

static void DBCSigGroup_deinit(DBCSigGroup* thiz)
{
  for (size_t i = 0; i < thiz->signals.size; i++) {
    DBCSignal* iter = toDBCSignal(thiz->signals.objects[i]);
    iter->sigGroup = NULL;
  }

  DBCArray_finalize(thiz->base.context, &thiz->signals);
}

void DBCSigGroup_release(DBCSigGroup* thiz)
{
  DBCObject_release(thiz);
}

const char* DBCSigGroup_name(const DBCSigGroup* thiz)
{
  return thiz->base.name;
}

size_t DBCSigGroup_signalCount(const DBCSigGroup* thiz)
{
  return thiz->signals.size;
}

DBCSignal* DBCSigGroup_signalAt(const DBCSigGroup* thiz, size_t index)
{
  return toDBCSignal(thiz->signals.objects[index]);
}

bool DBCSigGroup_addSignal(DBCSigGroup* thiz, DBCSignal* signal)
{
  if (signal == NULL || signal->base.context != thiz->base.context)
    return false;

  if (signal->base.parent != thiz->base.parent)
    return false;

  if (signal->sigGroup != NULL)
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->signals, signal))
    return false;

  signal->sigGroup = thiz;
  return true;
}

DBCMessage* DBCMessage_create(DBCContext* context, const char* name, uint32_t id, uint32_t sizeInBytes)
{
  size_t lenz = strlen(name) + 1;

  DBCMessage* thiz = (DBCMessage*)DBCAlloc(context, sizeof(struct DBCMessage) + lenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectAttr_init(context, &thiz->base, kDBCMessageIndex);

  char* str = (char*)thiz + sizeof(struct DBCMessage);
  memcpy(str, name, lenz);
  thiz->base.name = str;

  thiz->id = id;
  thiz->sizeInBytes = sizeInBytes;

  DBCArray_init(context, &thiz->groups);
  DBCArray_init(context, &thiz->signals);
  DBCArray_init(context, &thiz->transmitters);

  thiz->isPseudo = strcmp(name, "VECTOR__INDEPENDENT_SIG_MSG") == 0;

  return thiz;
}

static void DBCMessage_deinit(DBCMessage* thiz)
{
  DBCContext* context = thiz->base.context;

  DBCArray_finalize(context, &thiz->groups);
  DBCArray_finalize(context, &thiz->signals);
  DBCArray_finalize(context, &thiz->transmitters);

  DBCObjectAttr_finalize(context, &thiz->base);
}

void DBCMessage_release(DBCMessage* thiz)
{
  DBCObject_release(thiz);
}

const char* DBCMessage_name(const DBCMessage* thiz)
{
  return thiz->base.name;
}

DBCAttribute* DBCMessage_findAttribute(const DBCMessage* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t DBCMessage_attributeCount(const DBCMessage* thiz)
{
  return thiz->base.attributes.size;
}

DBCAttribute* DBCMessage_attributeAt(const DBCMessage* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

uint32_t DBCMessage_id(const DBCMessage* thiz)
{
  return thiz->id;
}

uint32_t DBCMessage_sizeInBytes(const DBCMessage* thiz)
{
  return thiz->sizeInBytes;
}

const char* DBCMessage_comment(const DBCMessage* thiz)
{
  return thiz->base.comment.characters;
}

bool DBCMessage_isPseudo(const DBCMessage* thiz)
{
  return thiz->isPseudo;
}

DBCSignal* DBCMessage_findSignal(DBCMessage* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->signals.size; i++) {
    DBCSignal* iter = toDBCSignal(thiz->signals.objects[i]);
    if (!strcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

DBCSigGroup* DBCMessage_groupAt(const DBCMessage* thiz, size_t index)
{
  DBC_ASSERT(index < thiz->groups.size);
  return toDBCSigGroup(thiz->groups.objects[index]);
}

size_t DBCMessage_groupCount(const DBCMessage* thiz)
{
  return thiz->groups.size;
}

DBCSignal* DBCMessage_signalAt(const DBCMessage* thiz, size_t index)
{
  DBC_ASSERT(index < thiz->signals.size);
  return toDBCSignal(thiz->signals.objects[index]);
}

size_t DBCMessage_signalCount(const DBCMessage* thiz)
{
  return thiz->signals.size;
}

DBCNetNode* DBCMessage_senderAt(const DBCMessage* thiz, size_t index)
{
  DBC_ASSERT(index < thiz->transmitters.size);
  return toDBCNetNode(thiz->transmitters.objects[index]);
}

size_t DBCMessage_senderCount(const DBCMessage* thiz)
{
  return thiz->transmitters.size;
}

bool DBCMessage_addSignal(DBCMessage* thiz, DBCSignal* signal)
{
  if (signal == NULL || signal->base.context != thiz->base.context)
    return false;

  if (signal->base.parent != NULL)
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->signals, signal))
    return false;

  signal->base.parent = thiz;
  return true;
}

bool DBCMessage_addSigGroup(DBCMessage* thiz, DBCSigGroup* sigGroup)
{
  if (sigGroup == NULL || sigGroup->base.context != thiz->base.context)
    return false;

  if (sigGroup->base.parent != NULL)
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->groups, sigGroup))
    return false;

  sigGroup->base.parent = thiz;
  return true;
}

bool DBCMessage_setMultiplexerSignal(DBCMessage* thiz, DBCSignal* multiplexerSignal)
{
  size_t i;
  for (i = 0; i < thiz->signals.size; i++) {
    DBCSignal* iter = toDBCSignal(thiz->signals.objects[i]);
    if (!DBCSignal_setMultiplexerSignal(iter, multiplexerSignal))
      return false;
  }
  return true;
}

DBCNetNode* DBCDocument_findNetNode(DBCDocument* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->netNodes.size; i++) {
    DBCNetNode* iter = toDBCNetNode(thiz->netNodes.objects[i]);
    if (!strcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

DBCMessage* DBCDocument_findMessage(DBCDocument* thiz, uint32_t id)
{
  size_t i;
  for (i = 0; i < thiz->messages.size; i++) {
    DBCMessage* iter = toDBCMessage(thiz->messages.objects[i]);
    if (iter->id == id)
      return iter;
  }
  return NULL;
}

DBCSignal* DBCDocument_findSignal(DBCDocument* thiz, uint32_t messageId, const char* signalName)
{
  DBCMessage* message = DBCDocument_findMessage(thiz, messageId);
  if (message == NULL)
    return NULL;
  
  return DBCMessage_findSignal(message, signalName);
}

DBCEnvVar* DBCDocument_findEnvVar(DBCDocument* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->envVars.size; i++) {
    DBCEnvVar* iter = toDBCEnvVar(thiz->envVars.objects[i]);
    if (!strcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

DBCValTable* DBCDocument_findValTable(DBCDocument* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->valTables.size; i++) {
    DBCValTable* iter = toDBCValTable(thiz->valTables.objects[i]);
    if (!strcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

DBCSigType* DBCDocument_findSigType(DBCDocument* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->sigTypes.size; i++) {
    DBCSigType* iter = toDBCSigType(thiz->sigTypes.objects[i]);
    if (!strcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

const char* DBCDocument_version(const DBCDocument* thiz)
{
  return thiz->version.characters;
}

bool DBCDocument_setVersion(DBCDocument* thiz, const char* version)
{
  return DBCString_setCharacters(&thiz->context, &thiz->version, version);
}

const char* DBCDocument_symbolAt(const DBCDocument* thiz, size_t index)
{
  return thiz->symbols.items[index];
}

size_t DBCDocument_symbolCount(const DBCDocument* thiz)
{
  return thiz->symbols.count;
}

bool DBCDocument_setSymbols(DBCDocument* thiz, const char** symbols, size_t count)
{
  return DBCSymbols_set(thiz->base.context, &thiz->symbols, symbols, count);
}

void DBCDocument_setBitTiming(DBCDocument* thiz, const DBCBitTiming* bitTiming)
{
  if (bitTiming == NULL)
    thiz->hasBitTiming = false;
  else {
    thiz->hasBitTiming = true;
    thiz->bitTiming.baudrate = bitTiming->baudrate;
    thiz->bitTiming.btr1 = bitTiming->btr1;
    thiz->bitTiming.btr2 = bitTiming->btr2;
  }
}

DBCValTable* DBCValTable_create(DBCContext* context, const char* name, const DBCValDesc* items, size_t size)
{
  char c;
  size_t i;

  size_t totalInBytes = sizeof(struct DBCValTable);

  size_t nlenz = strlen(name) + 1;
  totalInBytes += nlenz;

  for (i = 0; i < size; i++) {
    totalInBytes += sizeof(char*) + sizeof(uint32_t);
    totalInBytes += strlen(items[i].description) + 1;
  }

  DBCValTable* thiz = (DBCValTable*)DBCAlloc(context, totalInBytes);
  if (thiz == NULL)
    return false;

  DBCObjectBase_init(context, &thiz->base, kDBCValTableIndex);

  char* str = (char*)thiz + sizeof(struct DBCValTable);

  char** description = (char**)str;
  thiz->description = description;
  str += sizeof(char*) * size;

  thiz->value = (uint32_t*)str;
  str += sizeof(uint32_t) * size;

  thiz->base.name = str;
  memcpy(str, name, nlenz);
  str += nlenz;

  thiz->size = (uint32_t)size;
  for (i = 0; i < size; i++) {
    thiz->value[i] = items[i].value;
    thiz->description[i] = str;
    const char* s = items[i].description;
    do {
      c = *str++ = *s++;
    } while (c != '\0');
  }

  DBC_ASSERT(((char*)thiz + totalInBytes) == str);
  return thiz;
}

static void DBCValTable_deinit(DBCValTable* thiz)
{
}

void DBCValTable_release(DBCValTable* thiz)
{
  DBCObject_release(thiz);
}

DBCSigType* DBCSigType_create(DBCContext* context, const char* name, const DBCSigInfo* info, double defaultValue)
{
  size_t nlenz = strlen(name) + 1;
  size_t ulenz = strlen(info->unit) + 1;

  DBCSigType* thiz = (DBCSigType*)DBCAlloc(context, sizeof(struct DBCSigType) + nlenz + ulenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectBase_init(context, &thiz->base, kDBCSigTypeIndex);

  char* str = (char*)thiz + sizeof(struct DBCSignal);
  memcpy(str, name, nlenz);
  thiz->base.name = str;

  thiz->info.sizeInBits = info->sizeInBits;
  thiz->info.byteOrder = info->byteOrder;
  thiz->info.isUnsigned = info->isUnsigned;
  thiz->info.factor = info->factor;
  thiz->info.offset = info->offset;
  thiz->info.minimum = info->minimum;
  thiz->info.maximum = info->maximum;

  str += nlenz;
  memcpy(str, info->unit, ulenz);
  thiz->info.unit = str;

  thiz->defaultValue = defaultValue;
  thiz->valTable = NULL;

  return thiz;
}

static void DBCSigType_deinit(DBCSigType* thiz)
{
  DBCSigType_setValTable(thiz, NULL);
}

void DBCSigType_release(DBCSigType* thiz)
{
  DBCObject_release(thiz);
}

void DBCSigType_setValTable(DBCSigType* thiz, DBCValTable* valTable)
{
  if (thiz->valTable != NULL)
    DBCObject_release(thiz->valTable);

  if (valTable != NULL)
    DBCObject_retain(valTable);

  thiz->valTable = valTable;
}

DBCByteOrder DBCSigType_byteOrder(const DBCSigType* thiz)
{
  return thiz->info.byteOrder;
}
