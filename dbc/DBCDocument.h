/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_DOCUMENT_H
#define _DBC_DOCUMENT_H

#include <dbc/DBCTypes.h>
#include <dbc/DBCAlloc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DBCDocument DBCDocument;
typedef struct DBCAttrProto DBCAttrProto;
typedef struct DBCAttribute DBCAttribute;
typedef struct DBCNetNode DBCNetNode;
typedef struct DBCMessage DBCMessage;
typedef struct DBCSignal DBCSignal;
typedef struct DBCSigGroup DBCSigGroup;
typedef struct DBCSigType DBCSigType;
typedef struct DBCEnvVar DBCEnvVar;
typedef struct DBCValTable DBCValTable;

/* DBCAttrProto */

bool DBCAttrProto_isRelateDocument(const DBCAttrProto*);
bool DBCAttrProto_isRelateNetNode(const DBCAttrProto*);
bool DBCAttrProto_isRelateMessage(const DBCAttrProto*);
bool DBCAttrProto_isRelateSignal(const DBCAttrProto*);
bool DBCAttrProto_isRelateEnvVar(const DBCAttrProto*);

bool DBCAttrProto_hasDefault(const DBCAttrProto*);
bool DBCAttrProto_setDefaultInt(DBCAttrProto*, int64_t value);
bool DBCAttrProto_setDefaultFloat(DBCAttrProto*, double value);
bool DBCAttrProto_setDefaultString(DBCAttrProto*, const char* value);

/* DBCAttribute */

DBCAttrValueType DBCAttribute_type(const DBCAttribute*);
const char* DBCAttribute_name(const DBCAttribute*);

int64_t DBCAttribute_asIntValue(const DBCAttribute*);
double DBCAttribute_asFloatValue(const DBCAttribute*);
const char* DBCAttribute_asStringValue(const DBCAttribute*);

/* DBCDocument */

DBCDocument* DBCDocument_create(const char* name);
DBCDocument* DBCDocument_create2(const char* name, void* allocator, DBCAllocFn* alloc, DBCFreeFn* free);
void DBCDocument_release(DBCDocument*);

DBCNetNode* DBCDocument_createNetNode(DBCDocument*, const char* name);
DBCMessage* DBCDocument_createMessage(DBCDocument*, const char* name, uint32_t id, uint32_t sizeInBytes);
DBCSignal* DBCDocument_createSignal(DBCDocument*, const char* name, uint32_t startBit, const DBCSigInfo* info);
DBCEnvVar* DBCDocument_createEnvVar(DBCDocument*, const char* name, const DBCEnvInfo* info);
DBCSigGroup* DBCDocument_createSigGroup(DBCDocument*, const char* name, uint32_t repetitions);
DBCValTable* DBCDocument_createValTable(DBCDocument*, const char* name, const DBCValDesc* items, size_t size);
DBCSigType* DBCDocument_createSigType(DBCDocument*, const char* name, const DBCSigInfo* info, double defaultValue);

DBCNetNode* DBCDocument_findNetNode(DBCDocument*, const char* name);
DBCMessage* DBCDocument_findMessage(DBCDocument*, uint32_t id);
DBCSignal* DBCDocument_findSignal(DBCDocument*, uint32_t messageId, const char* signalName);
DBCEnvVar* DBCDocument_findEnvVar(DBCDocument*, const char* name);
DBCValTable* DBCDocument_findValTable(DBCDocument*, const char* name);
DBCSigType* DBCDocument_findSigType(DBCDocument*, const char* name);

const char* DBCDocument_name(const DBCDocument*);

const char* DBCDocument_comment(const DBCDocument*);
bool DBCDocument_setComment(DBCDocument*, const char* comment);

const DBCBitTiming* DBCDocument_bitTiming(const DBCDocument*);
size_t DBCDocument_messageCount(const DBCDocument*);
DBCMessage* DBCDocument_messageAt(const DBCDocument*, size_t index);

DBCAttribute* DBCDocument_findAttribute(const DBCDocument*, const char* name);
size_t DBCDocument_attributeCount(const DBCDocument*);
DBCAttribute* DBCDocument_attributeAt(const DBCDocument*, size_t index);

bool DBCDocument_getAttrInt(const DBCDocument*, const char* name, int64_t* result);
bool DBCDocument_setAttrInt(DBCDocument*, const char* name, int64_t value);
bool DBCDocument_getAttrFloat(const DBCDocument*, const char* name, double* result);
bool DBCDocument_setAttrFloat(DBCDocument*, const char* name, double value);
bool DBCDocument_getAttrString(const DBCDocument*, const char* name, const char** result);
bool DBCDocument_setAttrString(DBCDocument*, const char* name, const char* value);

const char* DBCDocument_version(const DBCDocument* thiz);
bool DBCDocument_setVersion(DBCDocument* thiz, const char* version);

size_t DBCDocument_symbolCount(const DBCDocument*);
const char* DBCDocument_symbolAt(const DBCDocument*, size_t index);
bool DBCDocument_setSymbols(DBCDocument*, const char** symbols, size_t count);

void DBCDocument_setBitTiming(DBCDocument*, const DBCBitTiming* bitTiming);

bool DBCDocument_addNetNode(DBCDocument*, DBCNetNode* netNode);
bool DBCDocument_addMessage(DBCDocument*, DBCMessage* message);
bool DBCDocument_addEnvVar(DBCDocument*, DBCEnvVar* envVar);
bool DBCDocument_addValTable(DBCDocument*, DBCValTable* valTable);
bool DBCDocument_addSigType(DBCDocument*, DBCSigType* sigType);

bool DBCDocument_defineDocumentAttr(DBCDocument*, const char* name, const struct DBCAttrValueInfo* params);
bool DBCDocument_defineNetNodeAttr(DBCDocument*, const char* name, const struct DBCAttrValueInfo* params);
bool DBCDocument_defineMessageAttr(DBCDocument*, const char* name, const struct DBCAttrValueInfo* params);
bool DBCDocument_defineSignalAttr(DBCDocument*, const char* name, const struct DBCAttrValueInfo* params);
bool DBCDocument_defineEnvVarAttr(DBCDocument*, const char* name, const struct DBCAttrValueInfo* params);

DBCAttrProto* DBCDocument_findAttrProto(const DBCDocument*, const char* name);
size_t DBCDocument_attrProtoCount(const DBCDocument*);
DBCAttrProto* DBCDocument_attrProtoAt(const DBCDocument*, size_t index);

/* DBCNetNode */

void DBCNetNode_release(DBCNetNode*);
const char* DBCNetNode_name(const DBCNetNode*);
DBCAttribute* DBCNetNode_findAttribute(const DBCNetNode*, const char* name);
size_t DBCNetNode_attributeCount(const DBCNetNode*);
DBCAttribute* DBCNetNode_attributeAt(const DBCNetNode*, size_t index);
bool DBCNetNode_setComment(DBCNetNode*, const char* comment);

bool DBCNetNode_getAttrInt(const DBCDocument*, const char* name, int64_t* result);
bool DBCNetNode_setAttrInt(DBCNetNode*, const char* name, int64_t value);
bool DBCNetNode_getAttrFloat(const DBCNetNode*, const char* name, double* result);
bool DBCNetNode_setAttrFloat(DBCNetNode*, const char* name, double value);
bool DBCNetNode_getAttrString(const DBCNetNode*, const char* name, const char** result);
bool DBCNetNode_setAttrString(DBCNetNode*, const char* name, const char* value);

/* DBCMessage */

void DBCMessage_release(DBCMessage*);

const char* DBCMessage_name(const DBCMessage*);
DBCAttribute* DBCMessage_findAttribute(const DBCMessage*, const char* name);
size_t DBCMessage_attributeCount(const DBCMessage*);
DBCAttribute* DBCMessage_attributeAt(const DBCMessage*, size_t index);
uint32_t DBCMessage_id(const DBCMessage*);
uint32_t DBCMessage_sizeInBytes(const DBCMessage*);
const char* DBCMessage_comment(const DBCMessage*);
bool DBCMessage_isPseudo(const DBCMessage*);
DBCSignal* DBCMessage_findSignal(DBCMessage*, const char* name);
size_t DBCMessage_groupCount(const DBCMessage*);
DBCSigGroup* DBCMessage_groupAt(const DBCMessage*, size_t index);
size_t DBCMessage_signalCount(const DBCMessage*);
DBCSignal* DBCMessage_signalAt(const DBCMessage*, size_t index);
size_t DBCMessage_senderCount(const DBCMessage*);
DBCNetNode* DBCMessage_senderAt(const DBCMessage*, size_t index);
bool DBCMessage_setComment(DBCMessage*, const char* comment);

bool DBCMessage_getAttrInt(const DBCMessage*, const char* name, int64_t* result);
bool DBCMessage_setAttrInt(DBCMessage*, const char* name, int64_t value);
bool DBCMessage_getAttrFloat(const DBCMessage*, const char* name, double* result);
bool DBCMessage_setAttrFloat(DBCMessage*, const char* name, double value);
bool DBCMessage_getAttrString(const DBCMessage*, const char* name, const char** result);
bool DBCMessage_setAttrString(DBCMessage*, const char* name, const char* value);

bool DBCMessage_addTransmitter(DBCMessage*, DBCNetNode* transmitter);
bool DBCMessage_addSignal(DBCMessage*, DBCSignal* signal);
bool DBCMessage_addSigGroup(DBCMessage*, DBCSigGroup* sigGroup);
bool DBCMessage_setMultiplexerSignal(DBCMessage*, DBCSignal* multiplexorSwitch);

/* DBCSignal */

void DBCSignal_release(DBCSignal*);
const char* DBCSignal_name(const DBCSignal*);
const char* DBCSignal_comment(const DBCSignal*);
DBCAttribute* DBCSignal_findAttribute(const DBCSignal*, const char* name);
size_t DBCSignal_attributeCount(const DBCSignal*);
DBCAttribute* DBCSignal_attributeAt(const DBCSignal*, size_t index);
DBCSigGroup* DBCSignal_sigGroup(const DBCSignal*);
DBCSigValueType DBCSignal_valueType(const DBCSignal*);
uint32_t DBCSignal_startBit(const DBCSignal*);
uint32_t DBCSignal_sizeInBits(const DBCSignal*);
DBCByteOrder DBCSignal_byteOrder(const DBCSignal*);
bool DBCSignal_isUnsigned(const DBCSignal*);
double DBCSignal_factor(const DBCSignal*);
double DBCSignal_offset(const DBCSignal*);
double DBCSignal_minimum(const DBCSignal*);
double DBCSignal_maximum(const DBCSignal*);
const char* DBCSignal_unit(const DBCSignal*);
size_t DBCSignal_receiverCount(const DBCSignal*);
DBCNetNode* DBCSignal_receiverAt(const DBCSignal*, size_t index);

bool DBCSignal_setComment(DBCSignal*, const char* comment);

bool DBCSignal_getAttrInt(const DBCSignal*, const char* name, int64_t* result);
bool DBCSignal_setAttrInt(DBCSignal*, const char* name, int64_t value);
bool DBCSignal_getAttrFloat(const DBCSignal*, const char* name, double* result);
bool DBCSignal_setAttrFloat(DBCSignal*, const char* name, double value);
bool DBCSignal_getAttrString(const DBCSignal*, const char* name, const char** result);
bool DBCSignal_setAttrString(DBCSignal*, const char* name, const char* value);

bool DBCSignal_addReceiver(DBCSignal*, DBCNetNode* receiver);
void DBCSignal_setValTable(DBCSignal*, DBCValTable* valTable);
void DBCSignal_setValueType(DBCSignal*, DBCSigValueType type);
void DBCSignal_setSignalType(DBCSignal*, DBCSigType* sigType);
bool DBCSignal_setMultiplexerSignal(DBCSignal*, DBCSignal* multiplexerSignal);
bool DBCSignal_addMultiplexerRange(DBCSignal*, uint32_t first, uint32_t second);

/* DBCEnvVar */

void DBCEnvVar_release(DBCEnvVar*);
bool DBCEnvVar_setComment(DBCEnvVar*, const char* comment);
DBCAttribute* DBCEnvVar_findAttribute(const DBCEnvVar*, const char* name);
size_t DBCEnvVar_attributeCount(const DBCEnvVar*);
DBCAttribute* DBCEnvVar_attributeAt(const DBCEnvVar*, size_t index);

bool DBCEnvVar_getAttrInt(const DBCEnvVar*, const char* name, int64_t* result);
bool DBCEnvVar_setAttrInt(DBCEnvVar*, const char* name, int64_t value);
bool DBCEnvVar_getAttrFloat(const DBCEnvVar*, const char* name, double* result);
bool DBCEnvVar_setAttrFloat(DBCEnvVar*, const char* name, double value);
bool DBCEnvVar_getAttrString(const DBCEnvVar*, const char* name, const char** result);
bool DBCEnvVar_setAttrString(DBCEnvVar*, const char* name, const char* value);

bool DBCEnvVar_addAccessNode(DBCEnvVar*, DBCNetNode* node);
bool DBCEnvVar_setDataSize(DBCEnvVar*, uint32_t size);
void DBCEnvVar_setValTable(DBCEnvVar*, DBCValTable* valTable);

/* DBCSigGroup */

void DBCSigGroup_release(DBCSigGroup*);
const char* DBCSigGroup_name(const DBCSigGroup*);
size_t DBCSigGroup_signalCount(const DBCSigGroup*);
DBCSignal* DBCSigGroup_signalAt(const DBCSigGroup*, size_t index);
bool DBCSigGroup_addSignal(DBCSigGroup*, DBCSignal* signal);

/* DBCValTable */

void DBCValTable_release(DBCValTable*);

/* DBCSigType */

void DBCSigType_release(DBCSigType*);
void DBCSigType_setValTable(DBCSigType*, DBCValTable* valTable);
DBCByteOrder DBCSigType_byteOrder(const DBCSigType*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _DBC_DOCUMENT_H */
