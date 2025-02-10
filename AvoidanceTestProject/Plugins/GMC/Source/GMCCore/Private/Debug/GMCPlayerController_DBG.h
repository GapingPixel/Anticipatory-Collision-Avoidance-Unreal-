// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#if ALLOW_CONSOLE && !NO_LOGGING

#define DEBUG_STAT_PING\
  if (GMCCVars::StatPing != 0)\
  {\
    if (GetLocalRole() == ROLE_AutonomousProxy)\
    {\
      const UNetConnection* Connection = GetNetConnection();\
      float Ping = 0.f;\
      if (CL_WorldTimeAux.USE_PLAYER_STATE_PING)\
      {\
        Ping = GetPingInMilliseconds();\
      }\
      else if (Connection)\
      {\
        Ping = Connection->AvgLag * 1000.f;\
      }\
      DEBUG_PRINT_MSG(0, "ping: %.0f ms", Ping)\
      DEBUG_PRINT_MSG(0, "jitter: %.0f ms", Connection ? Connection->GetAverageJitterInMS() : 0.f)\
    }\
  }

#define DEBUG_LOG_NET_WORLD_TIME\
  if (GMCCVars::LogNetWorldTime != 0)\
  {\
    if (GetLocalRole() == ROLE_Authority)\
    {\
      if (const auto& World = GetWorld())\
      {\
        GMC_LOG(\
          LogGMCController,\
          GetPawn(),\
          Log,\
          TEXT("UtcNow: %s | ServerWorldTime = %12.6f s"),\
          *FDateTime::UtcNow().GetTimeOfDay().ToString(),\
          World->GetRealTimeSeconds()\
        )\
      }\
    }\
    else if (GetLocalRole() == ROLE_AutonomousProxy)\
    {\
      if (const auto& World = GetWorld())\
      {\
        GMC_LOG(\
          LogGMCController,\
          GetPawn(),\
          Log,\
          TEXT("UtcNow: %s | ClientWorldTime = %12.6f s"),\
          *FDateTime::UtcNow().GetTimeOfDay().ToString(),\
          CL_WorldTimeAux.SyncedWorldTime\
        )\
      }\
    }\
  }

#else

#define DEBUG_STAT_PING
#define DEBUG_LOG_NET_WORLD_TIME

#endif
