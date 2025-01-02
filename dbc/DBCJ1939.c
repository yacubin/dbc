/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "DBCJ1939.h"

#include <string.h>

#include "dbc/DBCAssert.h"

#define DBC_DS_BAM 32
#define DBC_PGN_TPCM 60416
#define DBC_PGN_TPDT 60160

#define DBC_DM1_MIL_INDEX (0u)
#define DBC_DM1_RSL_INDEX (1u)
#define DBC_DM1_AWL_INDEX (2u)
#define DBC_DM1_PL_INDEX  (3u)

void DBCJ1939DM1_init(DBCJ1939DM1* dm1)
{
  memset(dm1, 0, sizeof(*dm1));
  dm1->size = 2;
}

static inline void DBCJ1939DM1_setLamp(DBCJ1939DM1* dm1, uint8_t state, bool flash, int n)
{
  uint8_t* data = dm1->data + (flash ? 1 : 0);
  n *= 2;
  *data &= ~(3 << n);
  *data |= (state & 3) << n;
}

void DBCJ1939DM1_setMIL(DBCJ1939DM1* dm1, uint8_t state, uint8_t flash)
{
  DBCJ1939DM1_setLamp(dm1, state, false, DBC_DM1_MIL_INDEX);
  DBCJ1939DM1_setLamp(dm1, flash, true, DBC_DM1_MIL_INDEX);
}

void DBCJ1939DM1_setRSL(DBCJ1939DM1* dm1, uint8_t state, uint8_t flash)
{
  DBCJ1939DM1_setLamp(dm1, state, false, DBC_DM1_RSL_INDEX);
  DBCJ1939DM1_setLamp(dm1, flash, true, DBC_DM1_RSL_INDEX);
}

void DBCJ1939DM1_setAWL(DBCJ1939DM1* dm1, uint8_t state, uint8_t flash)
{
  DBCJ1939DM1_setLamp(dm1, state, false, DBC_DM1_AWL_INDEX);
  DBCJ1939DM1_setLamp(dm1, flash, true, DBC_DM1_AWL_INDEX);
}

void DBCJ1939DM1_setPL(DBCJ1939DM1* dm1, uint8_t state, uint8_t flash)
{
  DBCJ1939DM1_setLamp(dm1, state, false, DBC_DM1_PL_INDEX);
  DBCJ1939DM1_setLamp(dm1, flash, true, DBC_DM1_PL_INDEX);
}

bool DBCJ1939DM1_addDTC(DBCJ1939DM1* dm1, int spn, uint8_t fmi, uint8_t oc)
{
  if (sizeof(dm1->data) <= (dm1->size + 4))
    return false;

  uint8_t* data = &dm1->data[dm1->size];
  *data++ = (spn >> 0) & 0xFF;
  *data++ = (spn >> 8) & 0xFF;
  *data++ = ((spn >> 11) & 0xE0) | (fmi & 0x1F);
  *data++ = oc & 0x7F;
  dm1->size += 4;
  return true;
}

size_t DBCJ1939DM1_write(const DBCJ1939DM1* dm1, uint8_t* buffer, size_t n)
{
  size_t size = (n < dm1->size) ? n : dm1->size;
  memcpy(buffer, dm1->data, size);
  return dm1->size;
}

static uint32_t mkpdu(uint8_t p, int edp, int dp, uint8_t pf, uint8_t ps, uint8_t sa)
{
  int result = sa;

  result |= (ps << 8); // sa
  result |= (pf << 16);

  if (dp)
    result |= (1 << 24);

  if (edp)
    result |= (1 << 25);

  result |= (p & 7) << 26;

  return (uint32_t)result;
}

uint32_t DBCJ1939Mkpdu(uint8_t p, uint32_t pgn, uint8_t da, uint8_t sa)
{
  DBC_ASSERT(p <= 7);
  DBC_ASSERT(0xF000 <= pgn);
  DBC_ASSERT(da == 255);
  return ((uint32_t)p << 26) | (pgn << 8) | sa;
}

uint32_t DBCJ1939Mkpdu1(uint8_t p, uint32_t pgn, uint8_t da, uint8_t sa)
{
  DBC_ASSERT(p <= 7);
  DBC_ASSERT(pgn < 0xF000);
  DBC_ASSERT((pgn & 0xFF) == 0);

  // mkpdu(p, pgn & (1 << 17), pgn & (1 << 16), (pgn >> 8) & 0xFF, da, sa));
  return (uint32_t)(p << 26) | (pgn << 8) | (uint32_t)(da << 8) | sa;
}

uint8_t DBCJ1939GetP(uint32_t id)
{
  return (uint8_t)((id >> 26) & 7);
}

uint8_t DBCJ1939GetR(uint32_t id)
{
  return (uint8_t)((id >> 25) & 1);
}

uint8_t DBCJ1939GetDP(uint32_t id)
{
  return (uint8_t)((id >> 24) & 1);
}

uint8_t DBCJ1939GetPF(uint32_t id)
{
  return (uint8_t)((id >> 16) & 0xFF);
}

bool DBCJ1939IsPDU1(uint32_t id)
{
  return DBCJ1939GetPF(id) < 240;
}

bool DBCJ1939IsPDU2(uint32_t id)
{
  return !DBCJ1939IsPDU1(id);
}

uint8_t DBCJ1939GetSA(uint32_t id)
{
  return (uint8_t)(id & 0xFF);
}

bool DBCJ1939SetSA(uint32_t* id, uint8_t sa)
{
  *id = (*id & 0xFFFFFF00) | sa;
  return true;
}

uint8_t DBCJ1939GetDA(uint32_t id)
{
  if (DBCJ1939IsPDU1(id))
    return (id >> 8) & 0xFF;
  return kDBCJ1939Broadcast;
}

bool DBCJ1939SetDA(uint32_t* id, uint8_t da)
{
  uint32_t temp = *id;
  if (!DBCJ1939IsPDU1(temp))
    return false;

  *id = (temp & 0xFFFF00FF) | ((uint32_t)da << 8);
  return true;
}

uint32_t DBCJ1939GetPGN(uint32_t id)
{
  DBCJ1939PDU pdu = { .id = id };
  return DBCJ1939PDU_pgn(&pdu);
}

void DBCJ1939PDU_init(DBCJ1939PDU* thiz, uint32_t id)
{
  thiz->id = id;
}

uint8_t DBCJ1939PDU_version(const DBCJ1939PDU* thiz)
{
  return thiz->pdu.pf < 240 ? 1 : 2;
}

uint8_t DBCJ1939PDU_priority(const DBCJ1939PDU* thiz)
{
  return thiz->pdu.p;
}

uint32_t DBCJ1939PDU_pgn(const DBCJ1939PDU* thiz)
{
  uint32_t pgn = thiz->pdu1.pf << 8;

  if (DBCJ1939PDU_version(thiz) == 2)
    pgn |= thiz->pdu2.ge;

  pgn |= thiz->pdu.dp << 16;
  pgn |= thiz->pdu.r << 17;

  return pgn;
}

uint8_t DBCJ1939PDU_sa(const DBCJ1939PDU* thiz)
{
  return thiz->pdu.sa;
}

uint8_t DBCJ1939PDU_da(const DBCJ1939PDU* thiz)
{
  return (DBCJ1939PDU_version(thiz) == 1) ? thiz->pdu1.da : kDBCJ1939Broadcast;
}

uint32_t NHMJ1939kDM1DTC(int spn, uint8_t fmi, uint8_t oc)
{
  return (oc & 0x7F) | (((spn >> 11) & 0xE0) | (fmi & 0x1F)) << 16 | ((spn >> 8) & 0xFF) << 8 | ((spn >> 0) & 0xFF);
}

void DBCCanFrame_init(DBCCanFrame* frame, uint32_t id, const uint8_t* data, size_t size)
{
  memset(frame, 0, sizeof(*frame));

  frame->id = id;
  if (data == NULL)
    DBC_ASSERT(size == 0);
  else if (sizeof(frame->data) < size)
    DBC_ASSERT_NOT_REACHED();
  else {
    frame->size = (uint8_t)size;
    memcpy(frame->data, data, size);
  }
}

bool DBCCanFrame_append(DBCCanFrame* frame, const uint8_t* data, size_t size)
{
  if ((sizeof(frame->data) - frame->size) < size)
    return false;

  memcpy(frame->data, data, size);
  frame->size += (uint8_t)size;
  return true;
}

void DBCCanFrame_dump(DBCCanFrame* frame, char* buffer, size_t n)
{
}

void DBCJ1939TPBuilder_init(DBCJ1939TPBuilder* builder, uint8_t p, uint32_t pgn, uint8_t da, uint8_t sa)
{
  memset(builder, 0, sizeof(*builder));
  builder->p = p;
  builder->sa = sa;
  builder->da = da;

  DBCCanFrame* frame = &builder->frames[0];
  frame->id = DBCJ1939Mkpdu1(builder->p, DBC_PGN_TPCM, builder->da, builder->sa);
  frame->size = 8;
  frame->data[0] = DBC_DS_BAM;
  frame->data[4] = 0xFF;
  memcpy(&frame->data[5], &pgn, 3);
}

bool DBCJ1939TPBuilder_append(DBCJ1939TPBuilder* builder, const uint8_t* data, size_t size)
{
  if (size == 0 || (DBCJ1939_TP_DATA_SIZE_MAX - builder->nbyte) < size)
    return false;

  DBCCanFrame* frame = &builder->frames[builder->sn];

  for (; size != 0; size--) {
    if (frame->size >= 8) {
      frame++;
      frame->id = DBCJ1939Mkpdu1(builder->p, DBC_PGN_TPDT, builder->da, builder->sa);
      frame->data[0] = ++builder->sn;
      frame->size = 1;
    }
    frame->data[frame->size++] = *data++;
    builder->nbyte++;
  }

  if (frame->size < 8)
    memset(&frame->data[frame->size], 0xFF, 8 - frame->size);

  frame = &builder->frames[0];
  memcpy(&frame->data[1], &builder->nbyte, sizeof(builder->nbyte));
  frame->data[3] = builder->sn;

  return true;
}

size_t DBCJ1939TPBuilder_frameCount(const DBCJ1939TPBuilder* builder)
{
  return builder->sn + 1;
}

const DBCCanFrame* DBCJ1939TPBuilder_getFrameByIndex(const DBCJ1939TPBuilder* builder, size_t index)
{
  return (index <= builder->sn) ? &builder->frames[index] : NULL;
}

static uint8_t AEBS2CalcChecksum(uint32_t id, const uint8_t* data, uint32_t counter)
{
  size_t i;

  id &= 0x1FFFFFFF;
  counter &= 0x0F;

  uint32_t checksum =  0;
  for (i = 0; i < 7; i++)
    checksum += data[i];;
  checksum += counter;

  for (i = 0; i < 32; i += 8)
    checksum += (id >> i) & 0xFF;
  checksum += (checksum >> 4);
  checksum &= 0x0F;

  return (uint8_t)(checksum << 4 | counter);
}

bool DBCJ1939IsValidChecksum(int rule, const DBCCanFrame* frame)
{
  switch (rule) {
  case kDBCJ1939CalcRuleAEBS2:
  case kDBCJ1939CalcRuleXBR:
  case kDBCJ1939CalcRuleSAS:
  case kDBCJ1939CalcRuleCN:
    if (frame->size != 8)
      return false;
    return frame->data[7] == AEBS2CalcChecksum(frame->id, frame->data, frame->data[7]);
  }

  return false;
}

bool DBCJ1939UpdateChecksum(int rule, DBCCanFrame* frame, uint8_t counter)
{
  switch (rule) {
  case kDBCJ1939CalcRuleAEBS2:
  case kDBCJ1939CalcRuleXBR:
  case kDBCJ1939CalcRuleSAS:
  case kDBCJ1939CalcRuleCN:
    if (frame->size != 8)
      return false;
    frame->data[7] = AEBS2CalcChecksum(frame->id, frame->data, counter);
    return true;
  }

  return false;
}

bool DBCJ1939UpdateChecksum2(int rule, DBCCanFrame* frame, uint8_t counter)
{
  return DBCJ1939UpdateChecksum(rule, frame, counter);
}
