// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef TMR_H
#define TMR_H

#include "Common.h"

// Function prototypes / [関数プロトタイプ宣言]
void TMR_WdtClear();
bool TMR_IsStabilizationWaitTimePassed();
void TMR_ClearRecvTimeout();
bool TMR_IsRecvTimeout();
bool TMR_IsLedChangeTiming();
void TMR_ClearLedTimer();
void TMR_Init();

#endif