/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "DBCID.h"

uint32_t DBCGetIdBits(DBCID id)
{
  return DBCIsExtendedId(id) ? DBC_EXTENDED_ID_BITS : DBC_BASE_ID_BITS;
}
