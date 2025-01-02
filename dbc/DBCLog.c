/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "DBCLog.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char levelToChar(enum DBCLogLevel level)
{
  switch (level) {
  case kDBCInfoLevel:
    return 'I';
  case kDBCWarnLevel:
    return 'W';
  case kDBCErrorLevel:
    return 'E';
  default:
    return 'X';
  }
}

static const char* getFileName(const char* file)
{
  const char* result = file;

  for (;;) {
    char ch = *file++;
    if (ch == '\0')
      break;
    if (ch == '/' || ch == '\\') {
      result = file;
    }
  }

  return result;
}

static int DBCLogVprintf(const char* file, const char* func, int line, int level, const char* fmt, va_list ap)
{
  int ret;

  ret = printf("[%s:%i] %s %c ", getFileName(file), line, func, levelToChar(level));
  ret += vprintf(fmt, ap);
  ret += printf("\n");

  return ret;
}

int DBCLogPrintf(const char* file, const char* func, int line, int level, const char* fmt, ...)
{
  int ret;
  va_list ap;

  va_start(ap, fmt);
  ret = DBCLogVprintf(file, func, line, level, fmt, ap);
  va_end(ap);

  return ret;
}
