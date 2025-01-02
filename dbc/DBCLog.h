/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_LOG_H
#define _DBC_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

enum DBCLogLevel {
  kDBCInfoLevel,
  kDBCWarnLevel,
  kDBCErrorLevel,
};

int DBCLogPrintf(const char* file, const char* func, int line, int level, const char* fmt, ...);

#ifdef DBC_LOG_ENABLE
#define DBC_LOGI(...) DBCLogPrintf(__FILE__, __FUNCTION__, __LINE__, kDBCInfoLevel, __VA_ARGS__)
#define DBC_LOGW(...) DBCLogPrintf(__FILE__, __FUNCTION__, __LINE__, kDBCWarnLevel, __VA_ARGS__)
#define DBC_LOGE(...) DBCLogPrintf(__FILE__, __FUNCTION__, __LINE__, kDBCErrorLevel, __VA_ARGS__)
#else
#define DBC_LOGI(...) ((void)0)
#define DBC_LOGW(...) ((void)0)
#define DBC_LOGE(...) ((void)0)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _DBC_LOG_H */
