// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#undef FLog
#undef CFLog

#if !NO_LOGGING

#define FLog(Verbosity, Format, ...)\
  GMC_LOG(LogGMCMovement, PawnOwner, Verbosity, TEXT("UGMC_GoldSrcMovementCmp::%s: ") TEXT(Format), *FString(__func__), ##__VA_ARGS__)
#define CFLog(Condition, Verbosity, Format, ...)\
  GMC_CLOG(LogGMCMovement, PawnOwner, Condition, Verbosity, TEXT("UGMC_GoldSrcMovementCmp::%s: ") TEXT(Format), *FString(__func__), ##__VA_ARGS__)

#else

#define FLog(Verbosity, Format, ...)
#define CFLog(Condition, Verbosity, Format, ...)

#endif
