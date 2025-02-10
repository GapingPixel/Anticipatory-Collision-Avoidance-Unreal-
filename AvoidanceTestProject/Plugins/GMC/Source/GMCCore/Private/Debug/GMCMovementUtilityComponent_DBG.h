// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#undef FLog
#undef CFLog

#if !NO_LOGGING

#define FLog(Verbosity, Format, ...)\
  GMC_LOG(LogGMCMovement, PawnOwner, Verbosity, TEXT("UGMC_MovementUtilityCmp::%s: ") TEXT(Format), *FString(__func__), ##__VA_ARGS__)
#define CFLog(Condition, Verbosity, Format, ...)\
  GMC_CLOG(Condition, LogGMCMovement, PawnOwner, Verbosity, TEXT("UGMC_MovementUtilityCmp::%s: ") TEXT(Format), *FString(__func__), ##__VA_ARGS__)

#else

#define FLog(Verbosity, Format, ...)
#define CFLog(Condition, Verbosity, Format, ...)

#endif

#if ALLOW_CONSOLE && !NO_LOGGING

// Visualize current velocity and acceleration of the pawn.
#define DEBUG_SHOW_MOVEMENT_VECTORS\
  if (GMCCVars::ShowMovementVectors != 0)\
  {\
    if (const auto& World = GetWorld())\
    {\
      const FVector CurrentLocation = GetActorLocation_GMC();\
      constexpr float ArrowScale = 0.06f;\
      /*Velocity*/\
      DrawDebugDirectionalArrow(\
        World,\
        CurrentLocation,\
        CurrentLocation + (Velocity * ArrowScale),\
        100.f,\
        FColor::Blue,\
        false,\
        0.f,\
        ESceneDepthPriorityGroup::SDPG_Foreground,\
        2.5f\
      );\
      /*Acceleration*/\
      DrawDebugDirectionalArrow(\
        World,\
        CurrentLocation,\
        CurrentLocation + (Acceleration * ArrowScale),\
        100.f,\
        FColor::Red,\
        false,\
        0.f,\
        ESceneDepthPriorityGroup::SDPG_Foreground,\
        2.5f\
      );\
    }\
  }

#define DEBUG_LOG_AUTO_RESOLVE_PENETRATION_UP_SWEEP\
  if (Hit.bBlockingHit)\
  {\
    FLog(\
      VeryVerbose,\
      "Hit result upward sweep: "\
      "Hit.bBlockingHit = %s, "\
      "Hit.bStartPenetrating = %s, "\
      "Hit.Actor = %s, "\
      "Hit.Component = %s, "\
      "Hit.Distance = %f, "\
      "Hit.Time = %f, "\
      "Hit.PenetrationDepth = %f, "\
      "Hit.ImpactPoint = {%f, %f, %f}, "\
      "Hit.ImpactNormal = {%f, %f, %f}, "\
      "Hit.Location = {%f, %f, %f}, "\
      "Hit.Normal = {%f, %f, %f}, "\
      "Hit.TraceStart = {%f, %f, %f}, "\
      "Hit.TraceEnd = {%f, %f, %f}",\
      BOOL_TO_STR(Hit.bBlockingHit),\
      BOOL_TO_STR(Hit.bStartPenetrating),\
      Hit.GetActor() ? *Hit.GetActor()->GetHumanReadableName() : TEXT("nullptr"),\
      Hit.GetComponent() ? *Hit.GetComponent()->GetReadableName() : TEXT("nullptr"),\
      Hit.Distance,\
      Hit.Time,\
      Hit.PenetrationDepth,\
      Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z,\
      Hit.ImpactNormal.X, Hit.ImpactNormal.Y, Hit.ImpactNormal.Z,\
      Hit.Location.X, Hit.Location.Y, Hit.Location.Z,\
      Hit.Normal.X, Hit.Normal.Y, Hit.Normal.Z,\
      Hit.TraceStart.X, Hit.TraceStart.Y, Hit.TraceStart.Z,\
      Hit.TraceEnd.X, Hit.TraceEnd.Y, Hit.TraceEnd.Z\
    )\
  }

#define DEBUG_LOG_AUTO_RESOLVE_PENETRATION_DOWN_SWEEP\
  if (Hit.bBlockingHit)\
  {\
    FLog(\
      VeryVerbose,\
      "Hit result downward sweep: "\
      "Hit.bBlockingHit = %s, "\
      "Hit.bStartPenetrating = %s, "\
      "Hit.Actor = %s, "\
      "Hit.Component = %s, "\
      "Hit.Distance = %f, "\
      "Hit.Time = %f, "\
      "Hit.PenetrationDepth = %f, "\
      "Hit.ImpactPoint = {%f, %f, %f}, "\
      "Hit.ImpactNormal = {%f, %f, %f}, "\
      "Hit.Location = {%f, %f, %f}, "\
      "Hit.Normal = {%f, %f, %f}, "\
      "Hit.TraceStart = {%f, %f, %f}, "\
      "Hit.TraceEnd = {%f, %f, %f}",\
      BOOL_TO_STR(Hit.bBlockingHit),\
      BOOL_TO_STR(Hit.bStartPenetrating),\
      Hit.GetActor() ? *Hit.GetActor()->GetHumanReadableName() : TEXT("nullptr"),\
      Hit.GetComponent() ? *Hit.GetComponent()->GetReadableName() : TEXT("nullptr"),\
      Hit.Distance,\
      Hit.Time,\
      Hit.PenetrationDepth,\
      Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z,\
      Hit.ImpactNormal.X, Hit.ImpactNormal.Y, Hit.ImpactNormal.Z,\
      Hit.Location.X, Hit.Location.Y, Hit.Location.Z,\
      Hit.Normal.X, Hit.Normal.Y, Hit.Normal.Z,\
      Hit.TraceStart.X, Hit.TraceStart.Y, Hit.TraceStart.Z,\
      Hit.TraceEnd.X, Hit.TraceEnd.Y, Hit.TraceEnd.Z\
    )\
  }

#else

#define DEBUG_SHOW_MOVEMENT_VECTORS
#define DEBUG_LOG_AUTO_RESOLVE_PENETRATION_UP_SWEEP
#define DEBUG_LOG_AUTO_RESOLVE_PENETRATION_DOWN_SWEEP

#endif
