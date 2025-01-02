/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "DBCAlloc.h"

#include <stdlib.h>

void* DBCAllocDefault(void* allocator, size_t size)
{
  (void)allocator;
  return malloc(size);
}

void DBCFreeDefault(void* allocator, void* ptr)
{
  (void)allocator;
  free(ptr);
}

void DBCFreeNope(void* allocator, void* ptr)
{
  (void)allocator;
  (void)ptr;
}
