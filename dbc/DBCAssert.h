/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_ASSERT_H
#define _DBC_ASSERT_H

#include <assert.h>

#define DBC_ASSERT assert
#define DBC_ASSERT_NOT_REACHED() assert(0)
#define DBC_STATIC_ASSERT static_assert

#endif /* _DBC_ASSERT_H */
