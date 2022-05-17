/*
 *****************************************************************
 *
 * Module  : ADBG - ACP Debug Generated Services Code
 *
 * Purpose : THIS FILE IS AUTOMATICALLY GENERATED !
 *
 *****************************************************************
 *
 *  Copyright (c) 2014-2021 SEQUANS Communications.
 *  All rights reserved.
 *
 *  This is confidential and proprietary source code of SEQUANS
 *  Communications. The use of the present source code and all
 *  its derived forms is exclusively governed by the restricted
 *  terms and conditions set forth in the SEQUANS
 *  Communications' EARLY ADOPTER AGREEMENT and/or LICENCE
 *  AGREEMENT. The present source code and all its derived
 *  forms can ONLY and EXCLUSIVELY be used with SEQUANS
 *  Communications' products. The distribution/sale of the
 *  present source code and all its derived forms is EXCLUSIVELY
 *  RESERVED to regular LICENCE holder and otherwise STRICTLY
 *  PROHIBITED.
 *
 *****************************************************************
 */

#pragma once

#include "SIDL_Test.h"
#include "adbg.h"

SIDL_BEGIN_C_INTERFACE

void adbgTestHelloFromSSLogIn(acpCtx_t _ctx, size_t StrQty, const char* StrArray);

void adbgTestHelloToSSLogOut(acpCtx_t _ctx, size_t StrQty, const char* StrArray);

void adbgTestPingLogIn(acpCtx_t _ctx, uint32_t FromSS);

void adbgTestPingLogOut(acpCtx_t _ctx, uint32_t ToSS);

void adbgTestEchoLogIn(acpCtx_t _ctx, const struct EchoData* FromSS);

void adbgTestEchoLogOut(acpCtx_t _ctx, const struct EchoData* ToSS);

void adbgTestTest1LogIn(acpCtx_t _ctx, const struct Output* out);

void adbgTestTest2LogOut(acpCtx_t _ctx, const struct Output* out);

void adbgTestOtherLogIn(acpCtx_t _ctx, const struct Empty* in1, uint32_t in2, size_t in3Qty, const char* in3Array, const char* in4, bool in5, int in6, float in7, SomeEnum in8, size_t in9Qty, const struct Empty* in9Array, const struct Empty2* in10, const struct New* in11);

void adbgTestOtherLogOut(acpCtx_t _ctx, const struct Empty* out1, uint32_t out2, size_t out3Qty, const char* out3Array, const char* out4, bool out5, int out6, float out7, SomeEnum out8, size_t out9Qty, const struct Empty* out9Array, const struct Empty2* out10, const struct New* out11);

SIDL_END_C_INTERFACE
