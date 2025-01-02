/*
 * MIT License
 *
 * Copyright (c) 2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _DBC_J1939_H
#define _DBC_J1939_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define kDBCJ1939Broadcast 0xFFu
typedef uint32_t DBCJ1939Id;

uint32_t DBCJ1939Mkpdu(uint8_t p, uint32_t pgn, uint8_t da, uint8_t sa);
uint32_t DBCJ1939Mkpdu1(uint8_t p, uint32_t pgn, uint8_t da, uint8_t sa);

uint8_t DBCJ1939GetP(uint32_t id);
uint8_t DBCJ1939GetR(uint32_t id);
uint8_t DBCJ1939GetDP(uint32_t id);
uint8_t DBCJ1939GetPF(uint32_t id);
bool DBCJ1939IsPDU1(uint32_t id);
bool DBCJ1939IsPDU2(uint32_t id);
uint8_t DBCJ1939GetSA(uint32_t id);
bool DBCJ1939SetSA(uint32_t* id, uint8_t sa);
uint8_t DBCJ1939GetDA(uint32_t id);
bool DBCJ1939SetDA(uint32_t*, uint8_t da);
uint32_t DBCJ1939GetPGN(uint32_t id);

uint32_t NHMJ1939kDM1DTC(int spn, uint8_t fmi, uint8_t oc);

/*
   Protocol data unit
*/
typedef union DBCJ1939PDU {
  uint32_t id;

  struct {
    uint32_t pad : 2;
    uint32_t p : 3;   // Priority
    uint32_t r : 1;   // Reserved
    uint32_t dp : 2;  // Data Page
    uint32_t pf : 8;  // PDU Format
    uint32_t ps : 8;  // PDU Specific
    uint32_t sa : 8;  // Source Address
  } pdu;

  struct {
    uint32_t pad : 2;
    uint32_t p : 3;  // Priority
    uint32_t r : 1;  // Reserved
    uint32_t dp : 2; // Data Page
    uint32_t pf : 8; // PDU Format
    uint32_t da : 8; // Destination Address
    uint32_t sa : 8; // Source Address
  } pdu1;

  struct {
    uint32_t pad : 2;
    uint32_t p : 3;  // Priority
    uint32_t r : 1;  // Reserved
    uint32_t dp : 2; // Data Page
    uint32_t pf : 8; // PDU Format
    uint32_t ge : 8; // Group Extension
    uint32_t sa : 8; // Source Address
  } pdu2;

} DBCJ1939PDU;

void DBCJ1939PDU_init(DBCJ1939PDU*, uint32_t id);
uint8_t DBCJ1939PDU_version(const DBCJ1939PDU*);
uint8_t DBCJ1939PDU_priority(const DBCJ1939PDU*);
uint32_t DBCJ1939PDU_pgn(const DBCJ1939PDU*);
uint8_t DBCJ1939PDU_sa(const DBCJ1939PDU*);
uint8_t DBCJ1939PDU_da(const DBCJ1939PDU*);

typedef struct DBCJ1939Lamps {
  uint8_t mil : 2;
  uint8_t rsl : 2;
  uint8_t awl : 2;
  uint8_t pl : 2;
} DBCJ1939Lamps;

typedef struct DBCJ1939DTC {
  int spn;
  uint8_t fmi;
  uint8_t oc;
} DBCJ1939DTC;

#define DBCJ1939_DM1_PGN 65226u

typedef struct DBCJ1939DM1 {
  uint16_t size;
  uint8_t data[400];
} DBCJ1939DM1;

void DBCJ1939DM1_init(DBCJ1939DM1* dm1);
void DBCJ1939DM1_setMIL(DBCJ1939DM1* dm1, uint8_t state, uint8_t flash);
void DBCJ1939DM1_setRSL(DBCJ1939DM1* dm1, uint8_t state, uint8_t flash);
void DBCJ1939DM1_setAWL(DBCJ1939DM1* dm1, uint8_t state, uint8_t flash);
void DBCJ1939DM1_setPL(DBCJ1939DM1* dm1, uint8_t state, uint8_t flash);
bool DBCJ1939DM1_addDTC(DBCJ1939DM1* dm1, int spn, uint8_t fmi, uint8_t oc);
size_t DBCJ1939DM1_write(const DBCJ1939DM1* dm1, uint8_t* buffer, size_t n);

#define DBCJ1939_TP_PACKEGE_MAX 256u
#define DBCJ1939_TP_DATA_SIZE_MAX ((DBCJ1939_TP_PACKEGE_MAX - 1u) * 7u)

#define kDBCCanFrameSizeMax 8u
typedef struct DBCCanFrame {
  uint32_t id;
  uint8_t size;
  uint8_t __reserve[3];
  uint8_t data[kDBCCanFrameSizeMax];
} DBCCanFrame;

void DBCCanFrame_init(DBCCanFrame* frame, uint32_t id, const uint8_t* data, size_t size);
bool DBCCanFrame_append(DBCCanFrame* frame, const uint8_t* data, size_t size);
void DBCCanFrame_dump(DBCCanFrame* frame, char* buffer, size_t n);

typedef struct DBCJ1939TPBuilder {
  uint8_t p;
  uint8_t sa;
  uint8_t da;
  uint8_t sn;
  uint16_t nbyte;
  DBCCanFrame frames[DBCJ1939_TP_PACKEGE_MAX];
} DBCJ1939TPBuilder;

void DBCJ1939TPBuilder_init(DBCJ1939TPBuilder* builder, uint8_t p, uint32_t pgn, uint8_t da, uint8_t sa);
bool DBCJ1939TPBuilder_append(DBCJ1939TPBuilder* builder, const uint8_t* data, size_t size);
size_t DBCJ1939TPBuilder_frameCount(const DBCJ1939TPBuilder* builder);
const DBCCanFrame* DBCJ1939TPBuilder_getFrameByIndex(const DBCJ1939TPBuilder* builder, size_t index);

enum {
  kDBCJ1939CalcRuleAEBS2,
  kDBCJ1939CalcRuleXBR,
  kDBCJ1939CalcRuleSAS,
  kDBCJ1939CalcRuleCN,
};

bool DBCJ1939IsValidChecksum(int rule, const DBCCanFrame* frame);
bool DBCJ1939UpdateChecksum(int rule, DBCCanFrame* frame, uint8_t counter);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _DBC_J1939_H */
