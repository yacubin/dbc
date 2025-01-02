/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_KEYWORDS_H
#define _DBC_KEYWORDS_H

#define DBC_KEYWORD_MAX (32u)
typedef char DBCKeyword[DBC_KEYWORD_MAX];

enum DBCKeywordType {
  DBC_KEYWORD_VERSION,
  DBC_KEYWORD_NS,
  DBC_KEYWORD_BS,
  DBC_KEYWORD_BU,
  DBC_KEYWORD_BO,
  DBC_KEYWORD_BO_TX_BU,
  DBC_KEYWORD_EV,
  DBC_KEYWORD_ENVVAR_DATA,
  DBC_KEYWORD_CM,
  DBC_KEYWORD_BA,
  DBC_KEYWORD_BA_REL,
  DBC_KEYWORD_BA_DEF,
  DBC_KEYWORD_BA_DEF_REL,
  DBC_KEYWORD_BA_DEF_DEF,
  DBC_KEYWORD_BA_DEF_DEF_REL,
  DBC_KEYWORD_SG,
  DBC_KEYWORD_SGTYPE,
  DBC_KEYWORD_SG_MUL_VAL,
  DBC_KEYWORD_SIG_GROUP,
  DBC_KEYWORD_SIG_VALTYPE,
  DBC_KEYWORD_SIG_TYPE_REF,
  DBC_KEYWORD_VAL,
  DBC_KEYWORD_VAL_TABLE,
};

#endif /* _DBC_KEYWORDS_H */
