/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBCID_H
#define _DBCID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t DBCID;

#define kDBCExtendedIdMask (0x80000000u)
#define kDBCIdMask (~kDBCExtendedIdMask)

#define DBCIsExtendedId(id) ((id) & kDBCExtendedIdMask)
#define DBCGetId(id) ((id) & kDBCIdMask)

#define DBC_EXTENDED_ID_BITS (29u)
#define DBC_BASE_ID_BITS (11u)

uint32_t DBCGetIdBits(DBCID id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _DBCID_H */
