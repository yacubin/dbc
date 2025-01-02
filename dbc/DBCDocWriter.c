/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "DBCDocWriter.h"

#include <string.h>

#include "dbc/DBCAssert.h"
#include "dbc/DBCParser.h"
#include "dbc/DBCDocument.h"
#include "dbc/DBCLog.h"

struct DBCDocWriter {
  void* allocator;
  DBCAllocFn* alloc;
  DBCFreeFn* free;
  DBCParser* parser;
  DBCDocument* document;
  DBCMessage* lastMessage;
  DBCSignal* lastMultiplexerSignal;
  bool hasError;
};

static inline void* DBCAlloc(DBCDocWriter* context, size_t size)
{
  return context->alloc(context->allocator, size);
}

static inline void DBCFree(DBCDocWriter* context, void* ptr)
{
  context->free(context->allocator, ptr);
}

static void resetLastMessage(DBCDocWriter* thiz)
{
  if (thiz->lastMultiplexerSignal != NULL) {
    DBC_ASSERT(thiz->lastMessage != NULL);
    DBCMessage_setMultiplexerSignal(thiz->lastMessage, thiz->lastMultiplexerSignal);
    DBCSignal_release(thiz->lastMultiplexerSignal);
    thiz->lastMultiplexerSignal = NULL;
  }

  if (thiz->lastMessage != NULL) {
    DBCMessage_release(thiz->lastMessage);
    thiz->lastMessage = NULL;
  }
}

DBCNetNode* DBCDocument_findOrCreateNetNode(DBCDocument* thiz, const char* name)
{
  DBCNetNode* netNode = DBCDocument_findNetNode(thiz, name);
  if (netNode != NULL) {
    return netNode;
  }

  // TODO: Undeclared state
  netNode = DBCDocument_createNetNode(thiz, name);
  if (netNode == NULL) {
    DBC_LOGE("Can't create '%s' NetNode", name);
    return NULL;
  }

  if (!DBCDocument_addNetNode(thiz, netNode)) {
    DBC_LOGE("Can't add '%s' NetNode to document", name);
    DBCNetNode_release(netNode);
    return NULL;
  }

  DBC_LOGW("Created '%s' NetNode", name);

  DBCNetNode_release(netNode);
  return netNode;
}

static bool onParserSection(void* userdata, const DBCSection* section) {
  DBCDocWriter* thiz = (DBCDocWriter*)userdata;

  if (section->type != kDBCSectionSignal)
    resetLastMessage(thiz);

  switch (section->type) {
  case kDBCSectionVersion:
    if (!DBCDocument_setVersion(thiz->document, section->version.data))
      return false;
    return true;

  case kDBCSectionNewSymbols:
    if (!DBCDocument_setSymbols(thiz->document, section->newSymbols.data, section->newSymbols.count))
      return false;
    return true;

  case kDBCSectionBitTiming:
    DBCDocument_setBitTiming(thiz->document, &section->bitTiming.value);
    return true;

  case kDBCSectionNetNodes: {
    size_t i;
    for (i = 0; i < section->netNodes.count; i++) {
      DBCNetNode* netNode = DBCDocument_createNetNode(thiz->document, section->netNodes.names[i]);
      if (!DBCDocument_addNetNode(thiz->document, netNode)) {
        DBCNetNode_release(netNode);
        return false;
      }
      DBCNetNode_release(netNode);
    }
    return true;
  }

  case kDBCSectionValTable: {
    DBCValTable* valTable = DBCDocument_createValTable(thiz->document, section->valTable.name, section->valTable.valDesc, section->valTable.count);
    if (valTable == NULL)
      return false;

    bool success = DBCDocument_addValTable(thiz->document, valTable);
    DBCValTable_release(valTable);
    return success;
  }

  case kDBCSectionMessage: {
    const char* messageName = section->message.name;
    DBCMessage* message = DBCDocument_createMessage(thiz->document, messageName, section->message.id, section->message.sizeInBytes);
    if (message == NULL)
      return false;

    if (strcmp("Vector__XXX", section->message.transmitter) != 0) {
      const char* transmitterName = section->message.transmitter;
      DBCNetNode* transmitter = DBCDocument_findOrCreateNetNode(thiz->document, transmitterName);
      if (transmitter == NULL) {
        DBC_LOGE("Can't get transmitter '%s' for message '%s'", transmitterName, messageName);
        return false;
      }
      if (!DBCMessage_addTransmitter(message, transmitter)) {
        DBC_LOGE("Can't add transmitter '%s' to message '%s'", transmitterName, messageName);
        DBCMessage_release(message);
        return false;
      }
    }

    if (!DBCDocument_addMessage(thiz->document, message)) {
      DBCMessage_release(message);
      return false;
    }

    // DBCObject_release(message);
    DBC_ASSERT(thiz->lastMessage == NULL);
    thiz->lastMessage = message;

    return true;
  }

  case kDBCSectionMessageTransmitter: {
    DBCMessage* message = DBCDocument_findMessage(thiz->document, section->messageTransmitter.messageId);
    if (message == NULL)
      return false;

    size_t i;
    for (i = 0; i < section->messageTransmitter.count; i++) {
      DBCNetNode* transmitter = DBCDocument_findNetNode(thiz->document, section->messageTransmitter.transmitters[i]);
      if (!DBCMessage_addTransmitter(message, transmitter))
        return false;
    }

    return true;
  }

  case kDBCSectionSignal: {
    DBC_ASSERT(thiz->lastMessage != NULL);
    DBCSignal* signal = DBCDocument_createSignal(thiz->document, section->signal.name, section->signal.startBit, &section->signal.info);
    if (signal == NULL)
      return false;

    if (section->signal.count != 1 || strcmp("Vector__XXX", section->signal.receivers[0]) != 0) {
      size_t i;
      for (i = 0; i < section->signal.count; i++) {
        const char* receiverName = section->signal.receivers[i];
        DBCNetNode* receiver = DBCDocument_findOrCreateNetNode(thiz->document, receiverName);
        if (receiver == NULL) {
          DBC_LOGE("Can't get receiver '%s' for signal '%s'", receiverName, section->signal.name);
          return false;
        }
        if (!DBCSignal_addReceiver(signal, receiver)) {
          DBC_LOGE("Can't add receiver '%s' for signal '%s'", receiverName, section->signal.name);
          DBCSignal_release(signal);
          return false;
        }
      }
    }

    if (section->signal.hasMultiplexorValue) {
      if (!DBCSignal_addMultiplexerRange(signal, section->signal.multiplexorValue, section->signal.multiplexorValue)) {
        DBCSignal_release(signal);
        return false;;
      }
    }

    if (!DBCMessage_addSignal(thiz->lastMessage, signal)) {
      DBCSignal_release(signal);
      return false;
    }

    if (section->signal.isMultiplexorSwitch && thiz->lastMultiplexerSignal == NULL)
      thiz->lastMultiplexerSignal = signal;
    else
      DBCSignal_release(signal);

    return true;
  }

  case kDBCSectionSignalGroup: {
    DBCMessage* message = DBCDocument_findMessage(thiz->document, section->signalGroup.messageId);
    if (message == NULL)
      return false;

    DBCSigGroup* sigGroup = DBCDocument_createSigGroup(
      thiz->document, section->signalGroup.name,
      section->signalGroup.repetitions
      );
    if (sigGroup == NULL)
      return false;

    if (!DBCMessage_addSigGroup(message, sigGroup)) {
      DBCSigGroup_release(sigGroup);
      return false;
    }

    size_t i;
    for (i = 0; i < section->signalGroup.count; i++) {
      DBCSignal* signal = DBCMessage_findSignal(message, section->signalGroup.signals[i]);
      if (!DBCSigGroup_addSignal(sigGroup, signal)) {
        DBCSigGroup_release(sigGroup);
        return false;
      }
    }

    DBCSigGroup_release(sigGroup);
    return true;
  }

  case kDBCSectionSignalValue: {
    DBCSignal* signal = DBCDocument_findSignal(thiz->document, section->signalValue.messageId, section->signalValue.signalName);
    if (signal == NULL) {
      DBC_LOGW("Can't find '%s' signal", section->signalValue.signalName);
      // TODO: Strict
      return true;
    }

    DBCValTable* valTable = DBCDocument_createValTable(thiz->document, "", section->signalValue.valDesc, section->signalValue.count);
    if (valTable == NULL)
      return false;

    DBCSignal_setValTable(signal, valTable);
    DBCValTable_release(valTable);
    return true;
  }
  
  case kDBCSectionSignalValueType: {
    DBCSignal* signal = DBCDocument_findSignal(thiz->document, section->signalValueType.messageId, section->signalValueType.signalName);
    if (signal == NULL)
      return false;

    DBCSignal_setValueType(signal, section->signalValueType.value);
    return true;
  }

  case kDBCSectionEnvVar: {
    DBCEnvVar* envVar = DBCDocument_createEnvVar(thiz->document, section->envVar.name, &section->envVar.info);
    if (envVar == NULL)
      return false;

    if (section->envVar.count != 1 || strcmp(section->envVar.accessNode[0], "Vector__XXX") != 0) {
      size_t i;
      for (i = 0; i < section->envVar.count; i++) {
        DBCNetNode* netNode = DBCDocument_findNetNode(thiz->document, section->envVar.accessNode[i]);
        if (!DBCEnvVar_addAccessNode(envVar, netNode)) {
          DBCEnvVar_release(envVar);
          return false;
        }
      }
    }

    if (!DBCDocument_addEnvVar(thiz->document, envVar)) {
      DBCEnvVar_release(envVar);
      return false;
    }

    DBCEnvVar_release(envVar);
    return true;
  }
  
  case kDBCSectionSignalType: {
    DBCValTable* valTable = DBCDocument_findValTable(thiz->document, section->signalType.valTable);
    if (valTable == NULL)
      return false;

    DBCSigType* sigType = DBCDocument_createSigType(thiz->document, section->signalType.name, &section->signalType.info, section->signalType.defaultValue);
    if (sigType == NULL)
      return false;

    if (!DBCDocument_addSigType(thiz->document, sigType)) {
      DBCSigType_release(sigType);
      return false;
    }

    DBCSigType_setValTable(sigType, valTable);
    DBCSigType_release(sigType);
    return true;
  }
  
  case kDBCSectionSignalTypeRef: {
    DBCSigType* sigType = DBCDocument_findSigType(thiz->document, section->signalTypeRef.signalTypeName);
    if (sigType == NULL)
      return false;

    DBCSignal* signal = DBCDocument_findSignal(thiz->document, section->signalTypeRef.messageId, section->signalTypeRef.signalName);
    if (signal == NULL)
      return false;

    DBCSignal_setSignalType(signal, sigType);
    return true;
  }

  case kDBCSectionEnvVarData: {
    DBCEnvVar* envVar = DBCDocument_findEnvVar(thiz->document, section->envVarData.name);
    if (envVar == NULL)
      return false;

    if (!DBCEnvVar_setDataSize(envVar, section->envVarData.dataSize))
      return false;

    return true;
  }
   
  case kDBCSectionEnvVarValue: {
    DBCEnvVar* envVar = DBCDocument_findEnvVar(thiz->document, section->envVarData.name);
    if (envVar == NULL)
      return false;

    DBCValTable* valTable = DBCDocument_createValTable(thiz->document, "", section->envVarValue.valDesc, section->envVarValue.count);
    if (valTable == NULL)
      return false;

    DBCEnvVar_setValTable(envVar, valTable);
    DBCValTable_release(valTable);
    return true;
  }

  case kDBCSectionSignalMultiplexed: {
    DBCSignal* multiplexedSignal = DBCDocument_findSignal(thiz->document, section->signalMultiplexed.messageId, section->signalMultiplexed.signalName);
    if (multiplexedSignal == NULL)
      return false;

    DBCSignal* multiplexorSwitch = DBCDocument_findSignal(thiz->document, section->signalMultiplexed.messageId, section->signalMultiplexed.switchName);
    if (multiplexorSwitch == NULL)
      return false;

    if (!DBCSignal_setMultiplexerSignal(multiplexedSignal, multiplexorSwitch))
      return false;

    size_t i;
    for (i = 0; i < section->signalMultiplexed.count; i++) {
      if (!DBCSignal_addMultiplexerRange(multiplexedSignal, section->signalMultiplexed.range[i].first, section->signalMultiplexed.range[i].second)) {
        return false;
      }
    }

    return true;
  }

  case kDBCSectionComment: {
    switch (section->comment.target.objectType) {
    case kDBCObjectDocument: {
      return DBCDocument_setComment(thiz->document, section->comment.value);
    }
    case kDBCObjectNetNode: {
      const char* netNodeName = section->comment.target.netNode.name;
      DBCNetNode* netNode = DBCDocument_findNetNode(thiz->document, netNodeName);
      if (netNode == NULL) {
        DBC_LOGW("Can't find '%s' NetNode", netNodeName);
        return true;
      }
      return DBCNetNode_setComment(netNode, section->comment.value);
    }
    case kDBCObjectMessage: {
      uint32_t messageId = section->comment.target.message.id;
      DBCMessage* message = DBCDocument_findMessage(thiz->document, messageId);
      if (message == NULL) {
        DBC_LOGW("Can't find %u Message", messageId);
        return true;
      }
      return DBCMessage_setComment(message, section->comment.value);
    }
    case kDBCObjectSignal: {
      uint32_t messageId = section->comment.target.signal.messageId;
      DBCMessage* message = DBCDocument_findMessage(thiz->document, messageId);
      if (message == NULL) {
        DBC_LOGW("Can't find %u Message", messageId);
        return true;
      }
      const char* signalName = section->comment.target.signal.name;
      DBCSignal* signal = DBCMessage_findSignal(message, signalName);
      if (message == NULL) {
        DBC_LOGW("Can't find '%s' Signal of %u message", signalName, messageId);
        return true;
      }
      return DBCSignal_setComment(signal, section->comment.value);
    }
    case kDBCObjectEnvVar: {
      const char* envVarName = section->comment.target.envVar.name;
      DBCEnvVar* envVar = DBCDocument_findEnvVar(thiz->document, envVarName);
      if (envVar == NULL) {
        DBC_LOGW("Can't find '%s' EnvVar", envVarName);
        return true;
      }
      return DBCEnvVar_setComment(envVar, section->comment.value);
    }
    default:
      return false;
    }
  }

  case kDBCSectionAttr: {
    struct DBCAttrValueInfo attrValueInfo;
    switch (section->attr.target.objectType) {
    case kDBCObjectDocument: {
      switch (section->attr.value.type) {
      case kDBCVariantInt:
      case kDBCVariantUint:
        if (DBCDocument_setAttrInt(thiz->document, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!DBCDocument_defineDocumentAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' INT attribute for Document", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' INT attribute for Document", section->attr.name);
        return DBCDocument_setAttrInt(thiz->document, section->attr.name, section->attr.value.vInt);

      case kDBCVariantFloat:
        if (DBCDocument_setAttrFloat(thiz->document, section->attr.name, section->attr.value.vFloat))
          return true;

        attrValueInfo.type = kDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!DBCDocument_defineDocumentAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' FLOAT attribute for Document", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' FLOAT attribute for Document", section->attr.name);
        return DBCDocument_setAttrFloat(thiz->document, section->attr.name, section->attr.value.vFloat);

      case kDBCVariantString:
        if (!DBCDocument_setAttrString(thiz->document, section->attr.name, section->attr.value.vString)) {
          DBC_LOGW("Can't set '%s' value to attribute '%s'", section->attr.value.vString, section->attr.name);
        }
        break;

      default:
        DBC_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    case kDBCObjectNetNode: {
      DBCNetNode* netNode = DBCDocument_findNetNode(thiz->document, section->attr.target.netNode.name);
      if (netNode == NULL) {
        DBC_LOGW("Can't find '%s' network node", section->attr.target.netNode.name);
        return true;
      }

      switch (section->attr.value.type) {
      case kDBCVariantInt:
      case kDBCVariantUint:
        if (DBCNetNode_setAttrInt(netNode, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!DBCDocument_defineNetNodeAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' INT attribute for NetNode", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' INT attribute for NetNode", section->attr.name);
        return DBCNetNode_setAttrInt(netNode, section->attr.name, section->attr.value.vInt);

      case kDBCVariantFloat:
        if (DBCNetNode_setAttrFloat(netNode, section->attr.name, section->attr.value.vFloat))
          return true;

        attrValueInfo.type = kDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!DBCDocument_defineNetNodeAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' FLOAT attribute for NetNode", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' FLOAT attribute for NetNode", section->attr.name);
        return DBCNetNode_setAttrFloat(netNode, section->attr.name, section->attr.value.vFloat);

      case kDBCVariantString:
        if (!DBCNetNode_setAttrString(netNode, section->attr.name, section->attr.value.vString)) {
          DBC_LOGW("Can't set '%s' value to attribute '%s'", section->attr.value.vString, section->attr.name);
        }
        break;

      default:
        DBC_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    case kDBCObjectMessage: {
      DBCMessage* message = DBCDocument_findMessage(thiz->document, section->attr.target.message.id);
      if (message == NULL) {
        DBC_LOGW("Can't find %u message", section->attr.target.message.id);
        return true;
      }

      switch (section->attr.value.type) {
      case kDBCVariantInt:
      case kDBCVariantUint:
        if (DBCMessage_setAttrInt(message, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!DBCDocument_defineMessageAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' INT attribute for Message", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' INT attribute for Message", section->attr.name);
        return DBCMessage_setAttrInt(message, section->attr.name, section->attr.value.vInt);

      case kDBCVariantFloat:
        if (DBCMessage_setAttrFloat(message, section->attr.name, section->attr.value.vFloat))
          return true;

        attrValueInfo.type = kDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!DBCDocument_defineMessageAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' FLOAT attribute for Message", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' FLOAT attribute for Message", section->attr.name);
        return DBCMessage_setAttrFloat(message, section->attr.name, section->attr.value.vFloat);

      case kDBCVariantString:
        if (!DBCMessage_setAttrString(message, section->attr.name, section->attr.value.vString)) {
          DBC_LOGW("Can't set '%s' value to attribute '%s'", section->attr.value.vString, section->attr.name);
        }
        break;

      default:
        DBC_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    case kDBCObjectSignal: {
      DBCMessage* message = DBCDocument_findMessage(thiz->document, section->attr.target.signal.messageId);
      if (message == NULL) {
        DBC_LOGW("Can't find %u message", section->attr.target.signal.messageId);
        return true;
      }

      DBCSignal* signal = DBCMessage_findSignal(message, section->attr.target.signal.name);
      if (signal == NULL) {
        DBC_LOGW("Can't find '%s' signal", section->attr.target.signal.name);
        return true;
      }

      switch (section->attr.value.type) {
      case kDBCVariantInt:
      case kDBCVariantUint:
        if (DBCSignal_setAttrInt(signal, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!DBCDocument_defineSignalAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' INT attribute for Signal", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' INT attribute for Signal", section->attr.name);
        return DBCSignal_setAttrInt(signal, section->attr.name, section->attr.value.vInt);

      case kDBCVariantFloat:
        if (DBCSignal_setAttrFloat(signal, section->attr.name, section->attr.value.vFloat))
          return true;

        attrValueInfo.type = kDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!DBCDocument_defineSignalAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' FLOAT attribute for Signal", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' FLOAT attribute for Signal", section->attr.name);
        return DBCSignal_setAttrFloat(signal, section->attr.name, section->attr.value.vFloat);

      case kDBCVariantString:
        if (!DBCSignal_setAttrString(signal, section->attr.name, section->attr.value.vString)) {
          DBC_LOGW("Can't set '%s' value to attribute '%s'", section->attr.value.vString, section->attr.name);
        }
        break;

      default:
        DBC_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    case kDBCObjectEnvVar: {
      DBCEnvVar* envVar = DBCDocument_findEnvVar(thiz->document, section->attr.target.envVar.name);
      if (envVar == NULL) {
        DBC_LOGW("Can't find '%s' environment variable", section->attr.target.envVar.name);
        return true;
      }

      switch (section->attr.value.type) {
      case kDBCVariantInt:
      case kDBCVariantUint:
        if (DBCEnvVar_setAttrInt(envVar, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!DBCDocument_defineEnvVarAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' INT attribute for EnvVar", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' INT attribute for EnvVar", section->attr.name);
        return DBCEnvVar_setAttrInt(envVar, section->attr.name, section->attr.value.vInt);

      case kDBCVariantFloat:
        if (!DBCEnvVar_setAttrFloat(envVar, section->attr.name, section->attr.value.vFloat)) {
          DBC_LOGW("Can't set %f value to attribute '%s'", section->attr.value.vFloat, section->attr.name);
        }
        break;

      case kDBCVariantString:
        if (!DBCEnvVar_setAttrString(envVar, section->attr.name, section->attr.value.vString))
          return true;

        attrValueInfo.type = kDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!DBCDocument_defineEnvVarAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          DBC_LOGE("Can't create '%s' FLOAT attribute for EnvVar", section->attr.name);
          return false;
        }

        DBC_LOGW("Created '%s' FLOAT attribute for EnvVar", section->attr.name);
        return DBCEnvVar_setAttrString(envVar, section->attr.name, section->attr.value.vString);

      default:
        DBC_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    }

    return true;
  }

  case kDBCSectionAttrRel: {
    return true;
  }

  case kDBCSectionAttrDefinition: {
    switch (section->attrDefinition.objectType) {
    case kDBCObjectDocument:
      return DBCDocument_defineDocumentAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    case kDBCObjectNetNode:
      return DBCDocument_defineNetNodeAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    case kDBCObjectMessage:
      return DBCDocument_defineMessageAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    case kDBCObjectSignal:
      return DBCDocument_defineSignalAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    case kDBCObjectEnvVar:
      return DBCDocument_defineEnvVarAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    default:
      DBC_ASSERT_NOT_REACHED();
      break;
    }

    return false;
  }

  case kDBCSectionAttrRelDefinition: {
    return true;
  }

  case kDBCSectionAttrDefault: {
    DBCAttrProto* attrProto = DBCDocument_findAttrProto(thiz->document, section->attrDefault.name);
    if (attrProto == NULL) {
      DBC_LOGW("Can't find propotype '%s'", section->attrDefault.name);
      return true;
    }

    bool success = false;
    switch (section->attrDefault.value.type) {
    case kDBCVariantInt:
    case kDBCVariantUint:
      if (!DBCAttrProto_setDefaultInt(attrProto, section->attrDefault.value.vInt)) {
        DBC_LOGW("Can't set %lli as default for attribute '%s'", (long long)section->attrDefault.value.vInt, section->attrDefault.name);
      }
      break;

    case kDBCVariantFloat:
      if (!DBCAttrProto_setDefaultFloat(attrProto, section->attrDefault.value.vFloat)) {
        DBC_LOGW("Can't set %f as default for attribute '%s'", (double)section->attrDefault.value.vFloat, section->attrDefault.name);
      }
      break;

    case kDBCVariantString:
      if (!DBCAttrProto_setDefaultString(attrProto, section->attrDefault.value.vString)) {
        DBC_LOGW("Can't set '%s' as default for attribute '%s'", section->attrDefault.value.vString, section->attrDefault.name);
      }
      break;

    default:
      DBC_ASSERT_NOT_REACHED();
      break;
    }

    return true;
  }

  case kDBCSectionAttrRelDefault: {
    return true;
  }

  }

  return false;
}

DBCDocWriter* DBCDocWriter_create(const char* name)
{
  return DBCDocWriter_create2(name, NULL, &DBCAllocDefault, &DBCFreeDefault);
}

DBCDocWriter* DBCDocWriter_create2(const char* name, void* allocator, DBCAllocFn* alloc, DBCFreeFn* free)
{
  if (alloc == NULL) {
    return NULL;
  }
  if (free == NULL) {
    free = &DBCFreeNope;
  }

  DBCDocWriter* thiz = (DBCDocWriter*)alloc(allocator, sizeof(struct DBCDocWriter));
  if (thiz == NULL)
    return NULL;

  thiz->document = DBCDocument_create2(name, allocator, alloc, free);
  if (thiz->document == NULL) {
    free(allocator, thiz);
    return NULL;
  }

  thiz->parser = DBCParser_create2(thiz, &onParserSection, allocator, alloc, free);
  if (thiz->parser == NULL) {
    DBCDocument_release(thiz->document);
    free(allocator, thiz);
    return NULL;
  }

  thiz->allocator = allocator;
  thiz->alloc = alloc;
  thiz->free = free;

  thiz->lastMessage = NULL;
  thiz->lastMultiplexerSignal = NULL;
  thiz->hasError = false;

  return thiz;
}

void DBCDocWriter_destroy(DBCDocWriter* thiz)
{
  void* allocator = thiz->allocator;
  DBCFreeFn* freeFn = thiz->free;

  if (thiz->parser != NULL)
    DBCParser_destroy(thiz->parser);

  if (thiz->document != NULL)
    DBCDocument_release(thiz->document);

  freeFn(allocator, thiz);
}

bool DBCDocWriter_write(DBCDocWriter* thiz, const void* data, size_t size)
{
  if (thiz->document == NULL)
    return false;

  if (!DBCParser_append(thiz->parser, (const char*)data, size))
    return false;

  return true;
}

DBCDocument* DBCDocWriter_toDocument(DBCDocWriter* thiz)
{
  if (thiz->hasError)
    return NULL;

  DBCDocument* result = thiz->document;
  if (result != NULL) {
    if (!DBCParser_finish(thiz->parser)) {
      thiz->hasError = true;
      return NULL;
    }
    resetLastMessage(thiz);
    DBCParser_destroy(thiz->parser);
    thiz->parser = NULL;
    thiz->document = NULL;
  }

  return result;
}
