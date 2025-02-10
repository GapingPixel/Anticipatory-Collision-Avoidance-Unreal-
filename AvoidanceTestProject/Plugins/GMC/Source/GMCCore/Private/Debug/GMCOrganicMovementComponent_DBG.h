// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#undef FLog
#undef CFLog

#if !NO_LOGGING

#define FLog(Verbosity, Format, ...)\
  GMC_LOG(LogGMCMovement, PawnOwner, Verbosity, TEXT("UGMC_OrganicMovementCmp::%s: ") TEXT(Format), *FString(__func__), ##__VA_ARGS__)
#define CFLog(Condition, Verbosity, Format, ...)\
  GMC_CLOG(Condition, LogGMCMovement, PawnOwner, Verbosity, TEXT("UGMC_OrganicMovementCmp::%s: ") TEXT(Format), *FString(__func__), ##__VA_ARGS__)

// These variables are referenced directly by some logging macros in the organic movement component so they must be preset whenever logging is enabled
// (regardless of whether ALLOW_CONSOLE is also defined).
#define DEBUG_MOVE_WITH_BASE_START_LOCATION\
  const FVector Debug_StartLocation = UpdatedComponent->GetComponentLocation();
#define DEBUG_MOVE_WITH_BASE_START_ROTATION\
  const FQuat Debug_StartRotation = UpdatedComponent->GetComponentQuat();
#define DEBUG_MOVE_WITH_BASE_MOVE_DELTA\
  const FVector Debug_MoveDelta = UpdatedComponent->GetComponentLocation() - Debug_StartLocation;
#define DEBUG_MOVE_WITH_BASE_ROTATION_DELTA\
  const FRotator Debug_RotationDelta = (UpdatedComponent->GetComponentQuat() * Debug_StartRotation.Inverse()).Rotator();

#else

#define FLog(Verbosity, Format, ...)
#define CFLog(Condition, Verbosity, Format, ...)

#define DEBUG_MOVE_WITH_BASE_START_LOCATION
#define DEBUG_MOVE_WITH_BASE_START_ROTATION
#define DEBUG_MOVE_WITH_BASE_MOVE_DELTA
#define DEBUG_MOVE_WITH_BASE_ROTATION_DELTA

#endif

#if DO_CHECK && defined GMC_DO_CHECK

#define DEBUG_VERIFY_FLOOR_NOT_DIRTY(Floor)\
  if (!IsNavMeshWalking())\
  {\
    const bool bFloorDirty = Floor.IsDirty(FloorUpdateTolerance, this);\
    gmc_ck(!Floor.bDirtyManual && FloorUpdateTolerance > 0. ? !bFloorDirty : bFloorDirty)\
  }

#else

#define DEBUG_VERIFY_FLOOR_NOT_DIRTY(Floor)

#endif

#if ALLOW_CONSOLE && !NO_LOGGING

#define DEBUG_STAT_AND_LOG_ORGANIC_MOVEMENT_VALUES\
  if (GMCCVars::StatOrganicMovementValues != 0 || GMCCVars::LogOrganicMovementValues != 0)\
  {\
    const FVector Location = GetActorLocation_GMC();\
    const FRotator Rotation = GetActorRotation_GMC();\
    const FRotator ControlRotation = GetControllerRotation_GMC();\
    if (GMCCVars::StatOrganicMovementValues != 0)\
    {\
      DEBUG_PRINT_MSG(0, "Force : %s", *GetTransientForce().ToString())\
      DEBUG_PRINT_MSG(0, "Acceleration : %s", *GetTransientAcceleration().ToString())\
      DEBUG_PRINT_MSG(0, "Velocity : %s", *Velocity.ToString())\
      DEBUG_PRINT_MSG(0, "ControlRotation : %s", *ControlRotation.ToString())\
      DEBUG_PRINT_MSG(0, "Rotation : %s", *Rotation.ToString())\
      DEBUG_PRINT_MSG(0, "Location : %s", *Location.ToString())\
      DEBUG_PRINT_MSG(0, "SpeedZ : %f", FVector(0., 0., Velocity.Z).Size())\
      DEBUG_PRINT_MSG(0, "SpeedXY : %f", FVector(Velocity.X, Velocity.Y, 0.).Size())\
      DEBUG_PRINT_MSG(0, "SpeedTotal : %f", Velocity.Size())\
      DEBUG_PRINT_MSG(0, "DeltaTime : %f", GetMoveDeltaTime())\
      DEBUG_PRINT_MSG(0, "InputVector (Processed) : %s", *GetProcessedInputVector().ToString())\
      DEBUG_PRINT_MSG(0, "InputVector (Raw) : %s", *GetRawInputVector().ToString())\
      DEBUG_PRINT_MSG(0, "MovementMode : %s", *GetMovementModeAsString(MovementMode))\
    }\
    if (GMCCVars::LogOrganicMovementValues != 0)\
    {\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("MovementMode                    : %s"), *GetMovementModeAsString(MovementMode))\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("InputVector (Raw)               : %s"), *GetRawInputVector().ToString())\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("InputVector (Processed)         : %s"), *GetProcessedInputVector().ToString())\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("DeltaTime                       : %f"), GetMoveDeltaTime())\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("Speed Total                     : %f"), Velocity.Size())\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("Speed XY                        : %f"), FVector(Velocity.X, Velocity.Y, 0.).Size())\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("Speed Z                         : %f"), FVector(0.f, 0.f, Velocity.Z).Size())\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("Location                        :           X=%15.6f, Y=%15.6f, Z=%15.6f"), Location.X, Location.Y, Location.Z)\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("Rotation                        :           P=%15.6f, Y=%15.6f, R=%15.6f"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll)\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("ControlRotation                 :           P=%15.6f, Y=%15.6f, R=%15.6f"), ControlRotation.Pitch, ControlRotation.Yaw, ControlRotation.Roll)\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("Velocity                        :           X=%15.6f, Y=%15.6f, Z=%15.6f"), Velocity.X, Velocity.Y, Velocity.Z)\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("Acceleration                    :           X=%15.6f, Y=%15.6f, Z=%15.6f"), GetTransientAcceleration().X, GetTransientAcceleration().Y, GetTransientAcceleration().Z)\
      GMC_LOG(LogGMCMovement, PawnOwner, Log, TEXT("Force                           :           X=%15.6f, Y=%15.6f, Z=%15.6f"), GetTransientForce().X, GetTransientForce().Y, GetTransientForce().Z)\
    }\
  }

#define DEBUG_SHOW_FLOOR_SWEEP(Floor, Direction, TraceLength)\
  if (GMCCVars::ShowFloorSweep != 0 && !IsSubSteppedIteration())\
  {\
    gmc_ck(Direction.IsNormalized())\
    const FTransform& Transform = UpdatedComponent->GetComponentTransform();\
    const FVector Debug_ShapeTraceStart = Transform.GetLocation();\
    const FVector Debug_ShapeTraceEnd = Debug_ShapeTraceStart + Direction * TraceLength;\
    const FVector Debug_LineTraceStart = Transform.GetLocation() + Direction * GetRootCollisionHalfHeight(true);\
    const FVector Debug_LineTraceEnd = Debug_LineTraceStart + Direction * TraceLength;\
    const EGMC_CollisionShape CollisionShape = GetRootCollisionShape();\
    const FCollisionShape TraceShape = GetFrom(CollisionShape, GetRootCollisionExtent(true));\
    FQuat TraceRotation = AddToGMCCapsuleRotation(UpdatedComponent->GetComponentQuat());\
    /*Draw the line trace.*/\
    DrawDebugLine(\
      GetWorld(),\
      Debug_LineTraceStart,\
      Floor.LineHit().bBlockingHit ? Floor.LineHit().ImpactPoint : Debug_LineTraceEnd,\
      Floor.LineHit().bBlockingHit ? Floor.LineHit().bStartPenetrating ? FColor::Red : FColor::Green : FColor::Orange,\
      false,\
      0.f,\
      ESceneDepthPriorityGroup::SDPG_Foreground,\
      3.f\
    );\
    /*Draw the shape trace.*/\
    switch (CollisionShape)\
    {\
      case EGMC_CollisionShape::VerticalCapsule:\
      case EGMC_CollisionShape::HorizontalCapsule:\
        DrawDebugCapsule(\
          GetWorld(),\
          Floor.ShapeHit().bBlockingHit ? Floor.ShapeHit().Location : Debug_ShapeTraceEnd, \
          TraceShape.GetCapsuleHalfHeight(),\
          TraceShape.GetCapsuleRadius(),\
          TraceRotation,\
          Floor.ShapeHit().bBlockingHit ? Floor.ShapeHit().bStartPenetrating ? FColor::Red : FColor::Green : FColor::Orange,\
          false,\
          0.f,\
          ESceneDepthPriorityGroup::SDPG_Foreground\
        );\
        break;\
      case EGMC_CollisionShape::Box:\
        DrawDebugBox(\
          GetWorld(),\
          Floor.ShapeHit().bBlockingHit ? Floor.ShapeHit().Location : Debug_ShapeTraceEnd,\
          TraceShape.GetExtent(),\
          TraceRotation,\
          Floor.ShapeHit().bBlockingHit ? Floor.ShapeHit().bStartPenetrating ? FColor::Red : FColor::Green : FColor::Orange,\
          false,\
          0.f,\
          ESceneDepthPriorityGroup::SDPG_Foreground\
        );\
        break;\
      case EGMC_CollisionShape::Sphere:\
        DrawDebugSphere(\
          GetWorld(),\
          Floor.ShapeHit().bBlockingHit ? Floor.ShapeHit().Location : Debug_ShapeTraceEnd,\
          TraceShape.GetSphereRadius(),\
          64,\
          Floor.ShapeHit().bBlockingHit ? Floor.ShapeHit().bStartPenetrating ? FColor::Red : FColor::Green : FColor::Orange,\
          false,\
          0.f,\
          ESceneDepthPriorityGroup::SDPG_Foreground\
        );\
        break;\
      case EGMC_CollisionShape::Invalid:\
      case EGMC_CollisionShape::MAX:\
      default: gmc_ckne();\
    }\
    if (Floor.ShapeHit().bBlockingHit)\
    {\
      /*Shape hit impact point*/\
      DrawDebugPoint(\
        GetWorld(),\
        Floor.ShapeHit().ImpactPoint,\
        10.f,\
        Floor.ShapeHit().bStartPenetrating ? FColor::Red : FColor::Green,\
        false,\
        0.f,\
        ESceneDepthPriorityGroup::SDPG_Foreground\
      );\
    }\
    if (Floor.LineHit().bBlockingHit)\
    {\
      /*Line hit impact point*/\
      DrawDebugPoint(\
        GetWorld(),\
        Floor.LineHit().ImpactPoint,\
        10.f,\
        Floor.LineHit().bStartPenetrating ? FColor::Red : FColor::Green,\
        false,\
        0.f,\
        ESceneDepthPriorityGroup::SDPG_Foreground\
      );\
    }\
  }

#define DEBUG_LOG_NAN_DIAGNOSTIC\
  CFLog(Velocity.ContainsNaN(), Warning, "Velocity (%s) contains NAN.", *Velocity.ToString())\
  CFLog(GetTransientAcceleration().ContainsNaN(), Warning, "Acceleration (%s) contains NAN.", *GetTransientAcceleration().ToString())\
  CFLog(GetTransientForce().ContainsNaN(), Warning, "Force (%s) contains NAN.", *GetTransientForce().ToString())

#else

#define DEBUG_STAT_AND_LOG_ORGANIC_MOVEMENT_VALUES
#define DEBUG_SHOW_FLOOR_SWEEP(Floor, Direction, TraceLength)
#define DEBUG_LOG_NAN_DIAGNOSTIC

#endif
