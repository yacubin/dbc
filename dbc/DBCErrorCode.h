/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_ERRORCODE_H
#define _DBC_ERRORCODE_H

typedef enum DBCErrorCode {
  kDBCErrorInternal = 1,
  kDBCErrorUser,
  kDBCErrorBufferOverflow,
  kDBCErrorStackOverflow,

  kDBCErrorSectionKeyword,
  kDBCErrorSectionVersion,
  kDBCErrorSectionNewSymbols,
  kDBCErrorSectionBitTiming,
  kDBCErrorSectionNetNodes,
  kDBCErrorSectionValTable,
  kDBCErrorSectionMessage,
  kDBCErrorSectionSignal,
  kDBCErrorSectionSignalType,
  kDBCErrorSectionSignalValue,
  kDBCErrorSectionSignalValueType,
  kDBCErrorSectionSignalTypeRef,
  kDBCErrorSectionSignalGroup,
  kDBCErrorSectionMessageTransmitter,
  kDBCErrorSectionEnvVar,
  kDBCErrorSectionEnvVarData,
  kDBCErrorSectionEnvVarValue,
  kDBCErrorSectionComment,
  kDBCErrorSectionValueDescription,
  kDBCErrorSectionAttrDefinition,
  kDBCErrorSectionAttrRelDefinition,
  kDBCErrorSectionAttrDefault,
  kDBCErrorSectionAttrRelDefault,
  kDBCErrorSectionAttrValue,
  kDBCErrorSectionAttrRelValue,
  kDBCErrorSectionMultiplexedSignal,

  kDBCErrorTokenNumber,
  kDBCErrorTokenString,
} DBCErrorCode;

#endif /* _DBC_ERRORCODE_H */
