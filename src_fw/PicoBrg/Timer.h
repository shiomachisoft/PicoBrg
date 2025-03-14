// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef TIMER_H
#define TIMER_H

#include "Common.h"

// [関数プロトタイプ宣言]
void TIMER_WdtClear();
bool TIMER_IsStabilizationWaitTimePassed();
void TIMER_ClearRecvTimeout();
bool TIMER_IsRecvTimeout();
bool TIMER_IsLedChangeTiming();
void TIMER_ClearLedTimer();
void TIMER_Init();

#endif