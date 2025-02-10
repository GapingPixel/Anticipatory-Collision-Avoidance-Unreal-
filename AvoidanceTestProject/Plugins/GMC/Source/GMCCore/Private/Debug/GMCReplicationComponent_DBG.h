// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

// These variables are referenced directly by some logging macros in the replication component as well so they must be preset whenever logging is enabled
// (regardless of whether ALLOW_CONSOLE is also defined).
#if !NO_LOGGING

#define DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_ACTOR_LOCATION\
  const FVector Debug_OriginalActorLocation = GetActorLocation_GMC();
#define DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_ACTOR_ROTATION\
  const FQuat Debug_OriginalActorRotation = GetActorQuat_GMC();
#define DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_ACTOR_SCALE\
  const FVector Debug_OriginalActorScale = GetActorScale_GMC();
#define DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_CONTROL_ROTATION\
  const FQuat Debug_OriginalControlRotation = GetControllerRotation_GMC().Quaternion();
#define DEBUG_NET_CORRECTION_UPDATED_CLIENT_ACTOR_LOCATION\
  const FVector Debug_UpdatedActorLocation = GetActorLocation_GMC();
#define DEBUG_NET_CORRECTION_UPDATED_CLIENT_ACTOR_ROTATION\
  const FQuat Debug_UpdatedActorRotation = GetActorQuat_GMC();
#define DEBUG_NET_CORRECTION_UPDATED_CLIENT_ACTOR_SCALE\
  const FVector Debug_UpdatedActorScale = GetActorScale_GMC();
#define DEBUG_NET_CORRECTION_UPDATED_CLIENT_CONTROL_ROTATION\
  const FQuat Debug_UpdatedControlRotation = GetControllerRotation_GMC().Quaternion();
#define DEBUG_NET_CORRECTION_REPLAYED_CLIENT_ACTOR_LOCATION\
  const FVector Debug_ReplayedActorLocation = GetActorLocation_GMC();
#define DEBUG_NET_CORRECTION_REPLAYED_CLIENT_ACTOR_ROTATION\
  const FQuat Debug_ReplayedActorRotation = GetActorQuat_GMC();
#define DEBUG_NET_CORRECTION_REPLAYED_CLIENT_ACTOR_SCALE\
  const FVector Debug_ReplayedActorScale = GetActorScale_GMC();
#define DEBUG_NET_CORRECTION_REPLAYED_CLIENT_CONTROL_ROTATION\
  const FQuat Debug_ReplayedControlRotation = GetControllerRotation_GMC().Quaternion();

#else

#define DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_ACTOR_LOCATION
#define DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_ACTOR_ROTATION
#define DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_ACTOR_SCALE
#define DEBUG_NET_CORRECTION_ORIGINAL_CLIENT_CONTROL_ROTATION
#define DEBUG_NET_CORRECTION_UPDATED_CLIENT_ACTOR_LOCATION
#define DEBUG_NET_CORRECTION_UPDATED_CLIENT_ACTOR_ROTATION
#define DEBUG_NET_CORRECTION_UPDATED_CLIENT_ACTOR_SCALE
#define DEBUG_NET_CORRECTION_UPDATED_CLIENT_CONTROL_ROTATION
#define DEBUG_NET_CORRECTION_REPLAYED_CLIENT_ACTOR_LOCATION
#define DEBUG_NET_CORRECTION_REPLAYED_CLIENT_ACTOR_ROTATION
#define DEBUG_NET_CORRECTION_REPLAYED_CLIENT_ACTOR_SCALE
#define DEBUG_NET_CORRECTION_REPLAYED_CLIENT_CONTROL_ROTATION

#endif

#if ALLOW_CONSOLE && !NO_LOGGING

#define DEBUG_PRINT_LOCAL_STATS(PawnNetRole)\
  if (PawnOwner->GetLocalRole() == PawnNetRole)\
  {\
    const FRotator ControlRotation = GetControllerRotation_GMC();\
    const FRotator ActorRotation = GetActorRotation_GMC();\
    const FVector ActorLocation = GetActorLocation_GMC();\
    const FVector AngVelocity = GetAngularVelocity_GMC();\
    const FVector LinVelocity = GetLinearVelocity_GMC();\
    const FVector LastInputVector = GetLastInputVector();\
    DEBUG_PRINT_MSG(0, "");\
    DEBUG_PRINT_MSG(0, "Z: %12.6f", ControlRotation.Yaw);\
    DEBUG_PRINT_MSG(0, "Y: %12.6f", ControlRotation.Pitch);\
    DEBUG_PRINT_MSG(0, "X: %12.6f", ControlRotation.Roll);\
    DEBUG_PRINT_MSG(0, "Control Rotation:");\
    DEBUG_PRINT_MSG(0, "Z: %12.6f", ActorRotation.Yaw);\
    DEBUG_PRINT_MSG(0, "Y: %12.6f", ActorRotation.Pitch);\
    DEBUG_PRINT_MSG(0, "X: %12.6f", ActorRotation.Roll);\
    DEBUG_PRINT_MSG(0, "Actor Rotation:");\
    DEBUG_PRINT_MSG(0, "Z: %12.6f", ActorLocation.Z);\
    DEBUG_PRINT_MSG(0, "Y: %12.6f", ActorLocation.Y);\
    DEBUG_PRINT_MSG(0, "X: %12.6f", ActorLocation.X);\
    DEBUG_PRINT_MSG(0, "Actor Location:");\
    DEBUG_PRINT_MSG(0, "Z: %12.6f", AngVelocity.Z);\
    DEBUG_PRINT_MSG(0, "Y: %12.6f", AngVelocity.Y);\
    DEBUG_PRINT_MSG(0, "X: %12.6f", AngVelocity.X);\
    DEBUG_PRINT_MSG(0, "Angular Velocity:");\
    DEBUG_PRINT_MSG(0, "Z: %12.6f", LinVelocity.Z);\
    DEBUG_PRINT_MSG(0, "Y: %12.6f", LinVelocity.Y);\
    DEBUG_PRINT_MSG(0, "X: %12.6f", LinVelocity.X);\
    DEBUG_PRINT_MSG(0, "Linear Velocity:");\
    DEBUG_PRINT_MSG(0, "Z: %12.6f", LastInputVector.Z);\
    DEBUG_PRINT_MSG(0, "Y: %12.6f", LastInputVector.Y);\
    DEBUG_PRINT_MSG(0, "X: %12.6f", LastInputVector.X);\
    DEBUG_PRINT_MSG(0, "Last Input Vector:");\
    DEBUG_PRINT_MSG(0, "OwnerNetRole: %s", *GetNetRoleAsString(PawnOwner->GetLocalRole()));\
  }
#define DEBUG_DISPLAY_LOCAL_STATS\
  if (GMCCVars::StatNetMovementValues == 1)\
  {\
    DEBUG_PRINT_LOCAL_STATS(ROLE_Authority);\
  }\
  else if (GMCCVars::StatNetMovementValues == 2)\
  {\
    DEBUG_PRINT_LOCAL_STATS(ROLE_AutonomousProxy);\
  }\
  else if (GMCCVars::StatNetMovementValues == 3)\
  {\
    DEBUG_PRINT_LOCAL_STATS(ROLE_SimulatedProxy);\
  }

#define DEBUG_PRINT_NET_STATS(PawnNetRole)\
  if (PawnOwner->GetLocalRole() == PawnNetRole)\
  {\
    DEBUG_PRINT_MSG(0, "");\
    DEBUG_PRINT_MSG(0, "OwnerNetRole: %s", *GetNetRoleAsString(PawnOwner->GetLocalRole()));\
    DEBUG_PRINT_MSG(0, "WorldTimeSeconds: %f", GetTime());\
    DEBUG_PRINT_MSG(0, "MoveHistory.Num: %d", MoveHistory.Num());\
    DEBUG_PRINT_MSG(0, "ClientPendingMoves.Num: %d", CL_MoveExecutionAux.PendingMoves.Num());\
    DEBUG_PRINT_MSG(0, "ServerPendingMoves.Num: %d", SV_RemoteMoveExecutionAux.PendingMoves.Num());\
    DEBUG_PRINT_MSG(0, "bIsExtrapolating: %s", SimulationAux.bIsExtrapolating ? TEXT("true") : TEXT("false"));\
  }
#define DEBUG_DISPLAY_NET_STATS\
  if (GMCCVars::StatNetContextValues == 1)\
  {\
    DEBUG_PRINT_NET_STATS(ROLE_Authority);\
  }\
  else if (GMCCVars::StatNetContextValues == 2)\
  {\
    DEBUG_PRINT_NET_STATS(ROLE_AutonomousProxy);\
  }\
  else if (GMCCVars::StatNetContextValues == 3)\
  {\
    DEBUG_PRINT_NET_STATS(ROLE_SimulatedProxy);\
  }

#define DEBUG_SHOW_PAWN_NET_ROLE\
  if (GMCCVars::ShowNetRole != 0 && !IsNetMode(NM_DedicatedServer))\
  {\
    const FVector TextOffsetLocalRole = {0., 0., 150.};\
    const FVector TextOffsetRemoteRole = {0., 0., 125.};\
    const FVector TextLocation = IsSmoothedListenServerPawn() && SV_RemoteServerPawnSmoothingSwapBuffer.bInitialized ?\
      SV_RemoteServerPawnSmoothingSwapBuffer.Buffer.ActorLocation.Read() : GetActorLocation_GMC();\
    DrawDebugString(\
      GetWorld(),\
      TextLocation + TextOffsetLocalRole,\
      TEXT("Local Role: ") + GetNetRoleAsString(PawnOwner->GetLocalRole()),\
      PawnOwner,\
      FColor::Red,\
      0,\
      true\
    );\
    DrawDebugString(\
      GetWorld(),\
      TextLocation + TextOffsetRemoteRole,\
      TEXT("Remote Role: ") + GetNetRoleAsString(PawnOwner->GetRemoteRole()),\
      PawnOwner,\
      FColor::Red,\
      0,\
      true\
    );\
  }

#define DEBUG_SHOW_PAWN_SIMULATION_DELAY\
  if (GMCCVars::ShowSimulationDelay != 0 && !IsNetMode(NM_DedicatedServer))\
  {\
    if (!IsLocallyControlledServerPawn() && !IsPredictedAutonomousProxy())\
    {\
      const FString Delay =\
        InterpolationMode == EGMC_InterpolationMode::FixedDelay ? FString::SanitizeFloat(FixedDelayParams.FixedDelay) :\
        InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay ? FString::SanitizeFloat(AdaptiveDelayParams.CurrentDelay) :\
        TEXT("-");\
      const FVector TextOffset = {0., 0., 175.};\
      const FVector TextLocation = IsSmoothedListenServerPawn() && SV_RemoteServerPawnSmoothingSwapBuffer.bInitialized ?\
        SV_RemoteServerPawnSmoothingSwapBuffer.Buffer.ActorLocation.Read() : GetActorLocation_GMC();\
      DrawDebugString(\
        GetWorld(),\
        TextLocation + TextOffset,\
        TEXT("Delay: ") + Delay,\
        nullptr,\
        FColor::Red,\
        0,\
        true\
      );\
    }\
  }

// Visualize net corrections of the autonomous proxy on the client. The original state is displayed in red, the corrected (replayed) state in green. Keep in
// mind that this will only display shapes if there's an error in the actor location, actor rotation or control rotation. Other deviating state values do not
// trigger the visualization.
#define DEBUG_SHOW_CLIENT_CORRECTIONS\
  if (GMCCVars::ShowClientCorrections != 0)\
  {\
    if (\
      (\
        ReplicationSettings.DefaultCompressionSettings.ActorLocation == EGMC_FloatPrecisionBlueprint::FullPrecision ?\
          Debug_OriginalActorLocation != Debug_ReplayedActorLocation :\
          !Debug_OriginalActorLocation.Equals(\
            Debug_ReplayedActorLocation,\
            1. / (int32)ToNativeEnum(ReplicationSettings.DefaultCompressionSettings.ActorLocation)\
          )\
      ) ||\
      (\
        ReplicationSettings.DefaultCompressionSettings.ActorRotation == EGMC_FloatPrecisionBlueprint::FullPrecision ?\
          Debug_OriginalActorRotation != Debug_ReplayedActorRotation :\
          !Debug_OriginalActorRotation.Equals(\
            Debug_ReplayedActorRotation,\
            1. / (int32)ToNativeEnum(ReplicationSettings.DefaultCompressionSettings.ActorRotation)\
          )\
      ) ||\
      (\
        ReplicationSettings.DefaultCompressionSettings.ActorScale == EGMC_FloatPrecisionBlueprint::FullPrecision ?\
          Debug_OriginalActorScale != Debug_ReplayedActorScale :\
          !Debug_OriginalActorScale.Equals(\
            Debug_ReplayedActorScale,\
            1. / (int32)ToNativeEnum(ReplicationSettings.DefaultCompressionSettings.ActorScale)\
          )\
      ) ||\
      (\
        ReplicationSettings.DefaultCompressionSettings.ControlRotation == EGMC_FloatPrecisionBlueprint::FullPrecision ?\
          Debug_OriginalControlRotation != Debug_ReplayedControlRotation :\
          !Debug_OriginalControlRotation.Equals(\
            Debug_ReplayedControlRotation,\
            1. / (int32)ToNativeEnum(ReplicationSettings.DefaultCompressionSettings.ControlRotation)\
          )\
      )\
    )\
    {\
      if (const auto& World = GetWorld())\
      {\
        constexpr float DebugShapeLifetime = 3.f;\
        constexpr float DebugShapeThickness = 1.f;\
        const UShapeComponent* RootCollision = Cast<UShapeComponent>(PawnOwner->GetRootComponent());\
        if (const UBoxComponent* RootBox = Cast<UBoxComponent>(RootCollision))\
        {\
          const FVector BoxScaledExtent = RootBox->GetScaledBoxExtent();\
          DrawDebugBox(\
            World,\
            Debug_OriginalActorLocation,\
            BoxScaledExtent,\
            Debug_OriginalActorRotation,\
            FColor::Red,\
            false,\
            DebugShapeLifetime,\
            0,\
            DebugShapeThickness\
          );\
          DrawDebugBox(\
            World,\
            Debug_ReplayedActorLocation,\
            BoxScaledExtent,\
            Debug_ReplayedActorRotation,\
            FColor::Green,\
            false,\
            DebugShapeLifetime,\
            0,\
            DebugShapeThickness\
          );\
        }\
        else if (const USphereComponent* RootSphere = Cast<USphereComponent>(RootCollision))\
        {\
          constexpr int32 SphereSegments = 32;\
          const float SphereScaledRadius = RootSphere->GetScaledSphereRadius();\
          DrawDebugSphere(\
            World,\
            Debug_OriginalActorLocation,\
            SphereScaledRadius,\
            SphereSegments,\
            FColor::Red,\
            false,\
            DebugShapeLifetime,\
            0,\
            DebugShapeThickness\
          );\
          DrawDebugSphere(\
            World,\
            Debug_ReplayedActorLocation,\
            SphereScaledRadius,\
            SphereSegments,\
            FColor::Green,\
            false,\
            DebugShapeLifetime,\
            0,\
            DebugShapeThickness\
          );\
        }\
        else if (const UGMC_FlatCapsuleCmp* RootHCapsule = Cast<UGMC_FlatCapsuleCmp>(RootCollision))\
        {\
          const float FlatCapsuleScaledHalfHeight = RootHCapsule->GetScaledCapsuleHalfHeight();\
          const float FlatCapsuleScaledRadius = RootHCapsule->GetScaledCapsuleRadius();\
          FQuat FlatCapsuleOriginalActorRotation = (Debug_OriginalActorRotation * FQuat(RootHCapsule->GetGMCCapsuleRotation())).GetNormalized();\
          FQuat FlatCapsuleReplayedActorRotation = (Debug_ReplayedActorRotation * FQuat(RootHCapsule->GetGMCCapsuleRotation())).GetNormalized();\
          DrawDebugCapsule(\
            World,\
            Debug_OriginalActorLocation,\
            FlatCapsuleScaledHalfHeight,\
            FlatCapsuleScaledRadius,\
            FlatCapsuleOriginalActorRotation,\
            FColor::Red,\
            false,\
            DebugShapeLifetime,\
            0,\
            DebugShapeThickness\
          );\
          DrawDebugCapsule(\
            World,\
            Debug_ReplayedActorLocation,\
            FlatCapsuleScaledHalfHeight,\
            FlatCapsuleScaledRadius,\
            FlatCapsuleReplayedActorRotation,\
            FColor::Green,\
            false,\
            DebugShapeLifetime,\
            0,\
            DebugShapeThickness\
          );\
        }\
        else if (const UCapsuleComponent* RootVCapsule = Cast<UCapsuleComponent>(RootCollision))\
        {\
          const float CapsuleScaledHalfHeight = RootVCapsule->GetScaledCapsuleHalfHeight();\
          const float CapsuleScaledRadius = RootVCapsule->GetScaledCapsuleRadius();\
          DrawDebugCapsule(\
            World,\
            Debug_OriginalActorLocation,\
            CapsuleScaledHalfHeight,\
            CapsuleScaledRadius,\
            Debug_OriginalActorRotation,\
            FColor::Red,\
            false,\
            DebugShapeLifetime,\
            0,\
            DebugShapeThickness\
          );\
          DrawDebugCapsule(\
            World,\
            Debug_ReplayedActorLocation,\
            CapsuleScaledHalfHeight,\
            CapsuleScaledRadius,\
            Debug_ReplayedActorRotation,\
            FColor::Green,\
            false,\
            DebugShapeLifetime,\
            0,\
            DebugShapeThickness\
          );\
        }\
      }\
    }\
  }

// Visualize client errors on the server. The erroneous client state is displayed in red, the server state in green. This will always display shapes as soon
// as any state value deviates, but keep in mind that not all deviations cause an error in the actor location or rotation.
#define DEBUG_SHOW_CLIENT_ERRORS\
  if (GMCCVars::ShowClientErrors != 0)\
  {\
    const auto& World = GetWorld();\
    if (World && !bInOutValidState)\
    {\
      constexpr float DebugShapeLifetime = 3.f;\
      constexpr float DebugShapeThickness = 1.f;\
      const UShapeComponent* RootCollision = Cast<UShapeComponent>(PawnOwner->GetRootComponent());\
      if (const UBoxComponent* RootBox = Cast<UBoxComponent>(RootCollision))\
      {\
        const FVector BoxScaledExtent = RootBox->GetScaledBoxExtent();\
        DrawDebugBox(\
          World,\
          State.ActorLocation.Read(),\
          BoxScaledExtent,\
          State.ActorRotation.Read().Quaternion(),\
          FColor::Red,\
          false,\
          DebugShapeLifetime,\
          0,\
          DebugShapeThickness\
        );\
        DrawDebugBox(\
          World,\
          GetActorLocation_GMC(),\
          BoxScaledExtent,\
          GetActorQuat_GMC(),\
          FColor::Green,\
          false,\
          DebugShapeLifetime,\
          0,\
          DebugShapeThickness\
        );\
      }\
      else if (const USphereComponent* RootSphere = Cast<USphereComponent>(RootCollision))\
      {\
        constexpr int32 SphereSegments = 32;\
        const float SphereScaledRadius = RootSphere->GetScaledSphereRadius();\
        DrawDebugSphere(\
          World,\
          State.ActorLocation.Read(),\
          SphereScaledRadius,\
          SphereSegments,\
          FColor::Red,\
          false,\
          DebugShapeLifetime,\
          0,\
          DebugShapeThickness);\
        DrawDebugSphere(\
          World,\
          GetActorLocation_GMC(),\
          SphereScaledRadius,\
          SphereSegments,\
          FColor::Green,\
          false,\
          DebugShapeLifetime,\
          0,\
          DebugShapeThickness);\
      }\
      else if (const UGMC_FlatCapsuleCmp* RootHCapsule = Cast<UGMC_FlatCapsuleCmp>(RootCollision))\
      {\
        const float FlatCapsuleScaledHalfHeight = RootHCapsule->GetScaledCapsuleHalfHeight();\
        const float FlatCapsuleScaledRadius = RootHCapsule->GetScaledCapsuleRadius();\
        DrawDebugCapsule(\
          World,\
          State.ActorLocation.Read(),\
          FlatCapsuleScaledHalfHeight,\
          FlatCapsuleScaledRadius,\
          (State.ActorRotation.Read().Quaternion() * FQuat(RootHCapsule->GetGMCCapsuleRotation())).GetNormalized(),\
          FColor::Red,\
          false,\
          DebugShapeLifetime,\
          0,\
          DebugShapeThickness\
        );\
        DrawDebugCapsule(\
          World,\
          GetActorLocation_GMC(),\
          FlatCapsuleScaledHalfHeight,\
          FlatCapsuleScaledRadius,\
          (GetActorQuat_GMC() * FQuat(RootHCapsule->GetGMCCapsuleRotation())).GetNormalized(),\
          FColor::Green,\
          false,\
          DebugShapeLifetime,\
          0,\
          DebugShapeThickness\
        );\
      }\
      else if (const UCapsuleComponent* RootVCapsule = Cast<UCapsuleComponent>(RootCollision))\
      {\
        const float CapsuleScaledHalfHeight = RootVCapsule->GetScaledCapsuleHalfHeight();\
        const float CapsuleScaledRadius = RootVCapsule->GetScaledCapsuleRadius();\
        DrawDebugCapsule(\
          World,\
          State.ActorLocation.Read(),\
          CapsuleScaledHalfHeight,\
          CapsuleScaledRadius,\
          State.ActorRotation.Read().Quaternion(),\
          FColor::Red,\
          false,\
          DebugShapeLifetime,\
          0,\
          DebugShapeThickness\
        );\
        DrawDebugCapsule(\
          World,\
          GetActorLocation_GMC(),\
          CapsuleScaledHalfHeight,\
          CapsuleScaledRadius,\
          GetActorQuat_GMC(),\
          FColor::Green,\
          false,\
          DebugShapeLifetime,\
          0,\
          DebugShapeThickness\
        );\
      }\
    }\
  }

#define DEBUG_LOG_REPLAY_APMOVE\
  if (GMCCVars::LogClientReplay != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client replay ::: Logging complemented AP move."))\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client replay ::: Input state:"))\
    LogSyncData(APMove().InputState, this, TEXT("Client replay ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client replay ::: Output state:"))\
    LogSyncData(APMove().OutputState, this, TEXT("Client replay ::: "));\
  }
#define DEBUG_LOG_REPLAY_CLIENT_STATE_INITIAL\
  if (GMCCVars::LogClientReplay != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client replay ::: Logging client state before replay."))\
    FGMC_PawnState DebugCurrentState{};\
    InitializeSyncData(DebugCurrentState, ReplicationSettings, GetSyncTagsData(), AliasData, GMCReplication::ESimState::None, GMCReplication::ESimType::None, this);\
    ProcessSyncData(DebugCurrentState, {DataOp::Save}, AliasData, bUseRelativeValuesForPrediction, this);\
    LogSyncData(DebugCurrentState, this, TEXT("Client replay ::: "));\
  }
#define DEBUG_LOG_REPLAY_CLIENT_STATE_BEFORE_REPLAY\
  if (GMCCVars::LogClientReplay != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client replay ::: Logging corrected client state before replay."))\
    FGMC_PawnState DebugCurrentState{};\
    InitializeSyncData(DebugCurrentState, ReplicationSettings, GetSyncTagsData(), AliasData, GMCReplication::ESimState::None, GMCReplication::ESimType::None, this);\
    ProcessSyncData(DebugCurrentState, {DataOp::Save}, AliasData, bUseRelativeValuesForPrediction, this);\
    LogSyncData(DebugCurrentState, this, TEXT("Client replay ::: "));\
  }
#define DEBUG_LOG_REPLAY_CLIENT_STATE_AFTER_REPLAY\
  if (GMCCVars::LogClientReplay != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client replay ::: Logging corrected client state after replay."))\
    FGMC_PawnState DebugCurrentState{};\
    InitializeSyncData(DebugCurrentState, ReplicationSettings, GetSyncTagsData(), AliasData, GMCReplication::ESimState::None, GMCReplication::ESimType::None, this);\
    ProcessSyncData(DebugCurrentState, {DataOp::Save}, AliasData, bUseRelativeValuesForPrediction, this);\
    LogSyncData(DebugCurrentState, this, TEXT("Client replay ::: "));\
  }

#define DEBUG_LOG_SMOOTHING_PARAMS\
  if (GMCCVars::LogSmoothing != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Logging smoothing parameters (current time = %f)."), GetTime())\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: MatchLatest:"))\
    MatchLatest.Log(this, TEXT("Smoothing ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: FixedDelay:"))\
    FixedDelay.Log(this, TEXT("Smoothing ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: AdaptiveDelay:"))\
    AdaptiveDelay.Log(this, TEXT("Smoothing ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: UniformSimulation:"))\
    UniformSimulation.Log(this, TEXT("Smoothing ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: CumulativeSimulation:"))\
    CumulativeSimulation.Log(this, TEXT("Smoothing ::: "));\
  }

#define DEBUG_LOG_SMOOTHING_NONE\
  if (GMCCVars::LogSmoothing != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Logging interpolation \"None\" (current time = %f)."), GetTime())\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Target state:"))\
    LogSyncData(InterpState, this, TEXT("Smoothing ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: New state:"))\
    FGMC_PawnState DebugCurrentState{};\
    InitializeSyncData(DebugCurrentState, ReplicationSettings, GetSyncTagsData(), AliasData, GMCReplication::ESimState::None, GMCReplication::ESimType::None, this);\
    ProcessSyncData(DebugCurrentState, {DataOp::Save}, AliasData, bUseRelativeValuesForSimulation, this);\
    LogSyncData(DebugCurrentState, this, TEXT("Smoothing ::: "));\
  }

#define DEBUG_LOG_SMOOTHING_MATCH_LATEST\
  if (GMCCVars::LogSmoothing != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Logging %s \"MatchLatest\" (current time = %f)."), bInterpolating ? TEXT("interpolation") : TEXT("extrapolation"), GetTime())\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Start state:"))\
    LogSyncData(MatchLatestParamsAux.SimulationStartState, this, TEXT("Smoothing ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Target state:"))\
    LogSyncData(bUseInputStates ? LastMove.InputState : LastMove.OutputState, this, TEXT("Smoothing ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Previous state:"))\
    LogSyncData(bUseInputStates ? MoveHistory[PrevIdx].InputState : MoveHistory[PrevIdx].OutputState, this, TEXT("Smoothing ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Next state:"))\
    LogSyncData(LastMove.OutputState, this, TEXT("Smoothing ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: New state:"))\
    FGMC_PawnState DebugCurrentState{};\
    InitializeSyncData(DebugCurrentState, ReplicationSettings, GetSyncTagsData(), AliasData, GMCReplication::ESimState::None, GMCReplication::ESimType::None, this);\
    ProcessSyncData(DebugCurrentState, {DataOp::Save}, AliasData, bUseRelativeValuesForSimulation, this);\
    LogSyncData(DebugCurrentState, this, TEXT("Smoothing ::: "));\
  }

#define DEBUG_LOG_SMOOTHING_DELAY\
  if (GMCCVars::LogSmoothing != 0)\
  {\
    if (!bRollback)\
    {\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Logging %s \"Delay\" (current time = %f)."), bInterpolating ? TEXT("interpolation") : TEXT("extrapolation"), GetTime())\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Start state:"))\
      LogSyncData(bUseInputStates ? StartMove.InputState : StartMove.OutputState, this, TEXT("Smoothing ::: "));\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Target state:"))\
      LogSyncData(bUseInputStates ? TargetMove.InputState : TargetMove.OutputState, this, TEXT("Smoothing ::: "));\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Previous state:"))\
      LogSyncData(bUseInputStates ? PreviousMove.InputState : PreviousMove.OutputState, this, TEXT("Smoothing ::: "));\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Next state:"))\
      LogSyncData(bUseInputStates ? NextMove.InputState : NextMove.OutputState, this, TEXT("Smoothing ::: "));\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: New state:"))\
      FGMC_PawnState DebugCurrentState{};\
      InitializeSyncData(DebugCurrentState, ReplicationSettings, GetSyncTagsData(), AliasData, GMCReplication::ESimState::None, GMCReplication::ESimType::None, this);\
      ProcessSyncData(DebugCurrentState, {DataOp::Save}, AliasData, bUseRelativeValuesForSimulation, this);\
      LogSyncData(DebugCurrentState, this, TEXT("Smoothing ::: "));\
    }\
  }

#define DEBUG_LOG_SMOOTHING_SIMULATED_INPUT\
  if (GMCCVars::LogSmoothing != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Logging %s \"Simulated\" (current time = %f)."), bInterpolating ? TEXT("interpolation") : TEXT("extrapolation"), GetTime())\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Input state:"))\
    LogSyncData(SimState, this, TEXT("Smoothing ::: "));\
  }

#define DEBUG_LOG_SMOOTHING_SIMULATED_OUTPUT\
  if (GMCCVars::LogSmoothing != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: Output state:"))\
    LogSyncData(SimState, this, TEXT("Smoothing ::: "));\
  }

#define DEBUG_LOG_SMOOTHING_SIMULATED_NEW_STATE\
  if (GMCCVars::LogSmoothing != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Smoothing ::: New state:"))\
    FGMC_PawnState DebugCurrentState{};\
    InitializeSyncData(DebugCurrentState, ReplicationSettings, GetSyncTagsData(), AliasData, GMCReplication::ESimState::None, GMCReplication::ESimType::None, this);\
    ProcessSyncData(DebugCurrentState, {DataOp::Save}, AliasData, bUseRelativeValuesForSimulation, this);\
    LogSyncData(DebugCurrentState, this, TEXT("Smoothing ::: "));\
  }

#define DEBUG_LOG_EXTRAPOLATION\
  if (GMCCVars::LogExtrapolation != 0)\
  {\
    if (!bDidInterpolate && !bDidExtrapolate)\
    {\
      GMC_CLOG(\
        SimulationAux.bMaxExtrapolationDistanceReached,\
        LogGMCReplication,\
        PawnOwner,\
        Log,\
        TEXT("\"%s\" (%f) reached."),\
        TO_STR(MaxExtrapolationDistance),\
        MaxExtrapolationDistance\
      )\
      GMC_LOG(\
        LogGMCReplication,\
        PawnOwner,\
        Log,\
        TEXT("No smoothing was applied this frame.")\
      )\
    }\
    else if (bDidExtrapolate)\
    {\
      GMC_LOG(\
        LogGMCReplication,\
        PawnOwner,\
        Log,\
        TEXT("Pawn state was extrapolated this frame: absolute extrapolated distance = %f | target time = %f | simulation time = %f | world time = %f"),\
        SimulationAux.AbsoluteExtrapolatedDistance,\
        OutTargetIdx != -1 ? MoveHistory[OutTargetIdx].MetaData.Timestamp : -1.,\
        OutSimTime,\
        GetTime()\
      )\
      GMC_CLOG(\
        SimulationAux.bMaxExtrapolationDistanceReached,\
        LogGMCReplication,\
        PawnOwner,\
        Log,\
        TEXT("\"%s\" (%f) reached."),\
        TO_STR(MaxExtrapolationDistance),\
        MaxExtrapolationDistance\
      )\
    }\
    else if (SimulationAux.ExtrapolationRecoveryTimer >= 0.f)\
    {\
      gmc_ck(bDidInterpolate)\
      const float ExtrapolationRecoveryTime =\
        InterpolationMode == EGMC_InterpolationMode::FixedDelay ? FixedDelay.ExtrapolationRecoveryTime :\
        InterpolationMode == EGMC_InterpolationMode::AdaptiveDelay ? AdaptiveDelay.ExtrapolationRecoveryTime :\
        -1.f;\
      GMC_LOG(\
        LogGMCReplication,\
        PawnOwner,\
        Log,\
        TEXT("Resuming interpolation: extrapolation recovery = %f s / %f s | target time = %f | simulation time = %f | world time = %f"),\
        SimulationAux.ExtrapolationRecoveryTimer,\
        ExtrapolationRecoveryTime,\
        OutTargetIdx != -1 ? MoveHistory[OutTargetIdx].MetaData.Timestamp : -1.,\
        OutSimTime,\
        GetTime()\
      )\
    }\
  }

#define DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_EXECUTED_MOVE\
  if (GMCCVars::LogClientMoveTrace != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Client executed local move with timestamp %f s and delta time %f s."), Move.MetaData.Timestamp, Move.MetaData.DeltaTime)\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Input state:"))\
    LogSyncData(Move.InputState, this, TEXT("Client move trace ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Output state:"))\
    LogSyncData(Move.OutputState, this, TEXT("Client move trace ::: "));\
  }
#define DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_SENT_MOVES\
  if (GMCCVars::LogClientMoveTrace != 0)\
  {\
    int32 NumMove = 0;\
    int32 NumMovesTotal = CL_MoveExecutionAux.PendingMoves.Num();\
    for (const auto& SentMove : CL_MoveExecutionAux.PendingMoves)\
    {\
      ++NumMove;\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Client sent move %d/%d with timestamp %f s and delta time %f s."), NumMove, NumMovesTotal, SentMove.MetaData.Timestamp, SentMove.MetaData.DeltaTime)\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Input state:"))\
      LogSyncData(SentMove.InputState, this, TEXT("Client move trace ::: "));\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Output state:"))\
      LogSyncData(SentMove.OutputState, this, TEXT("Client move trace ::: "));\
    }\
  }
#define DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_RECEIVED_MOVES\
  if (GMCCVars::LogClientMoveTrace != 0)\
  {\
    int32 NumMove = 0;\
    int32 NumMovesTotal = SV_RemoteMoveExecutionAux.DeserializedMoves.Num();\
    for (const auto& ReceivedMove : SV_RemoteMoveExecutionAux.DeserializedMoves)\
    {\
      ++NumMove;\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Server received remote move %d/%d with timestamp %f s and delta time %f s."), NumMove, NumMovesTotal, ReceivedMove.MetaData.Timestamp, ReceivedMove.MetaData.DeltaTime)\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Input state:"))\
      LogSyncData(ReceivedMove.InputState, this, TEXT("Client move trace ::: "));\
      GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Output state:"))\
      LogSyncData(ReceivedMove.OutputState, this, TEXT("Client move trace ::: "));\
    }\
  }
#define DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_EXECUTED_MOVE\
  if (GMCCVars::LogClientMoveTrace != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Server executed remote move with timestamp %f s and delta time %f s."), ClientMove.MetaData.Timestamp, ClientMove.MetaData.DeltaTime)\
    FGMC_PawnState CurrentServerState{};\
    InitializeSyncData(CurrentServerState, ReplicationSettings, GetSyncTagsData(), AliasData, GMCReplication::ESimState::None, GMCReplication::ESimType::None, this);\
    ProcessSyncData(CurrentServerState, {DataOp::Save}, AliasData, bUseRelative, this);\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Input state:"))\
    LogSyncData(ClientMove.InputState, this, TEXT("Client move trace ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Output state:"))\
    LogSyncData(CurrentServerState, this, TEXT("Client move trace ::: "));\
  }
#define DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_VALIDATED_MOVE\
  if (GMCCVars::LogClientMoveTrace != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Server validated remote move with timestamp %f s and delta time %f s."), ClientMove.MetaData.Timestamp, ClientMove.MetaData.DeltaTime)\
    FGMC_PawnState CurrentServerState{};\
    InitializeSyncData(CurrentServerState, ReplicationSettings, GetSyncTagsData(), AliasData, GMCReplication::ESimState::None, GMCReplication::ESimType::None, this);\
    ProcessSyncData(CurrentServerState, {DataOp::Save}, AliasData, bUseRelative, this);\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Input state:"))\
    LogSyncData(ClientMove.InputState, this, TEXT("Client move trace ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Output state:"))\
    LogSyncData(CurrentServerState, this, TEXT("Client move trace ::: "));\
  }
#define DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_SAVED_APMOVE\
  if (GMCCVars::LogClientMoveTrace != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Server saved autonomous proxy move with timestamp %f s and delta time %f s."), APMove().MetaData.Timestamp, APMove().MetaData.DeltaTime)\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Input state:"))\
    LogSyncData(APMove().InputState, this, TEXT("Client move trace ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Output state:"))\
    LogSyncData(APMove().OutputState, this, TEXT("Client move trace ::: "));\
  }
#define DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_RECEIVED_APMOVE\
  if (GMCCVars::LogClientMoveTrace != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Client received autonomous proxy move with timestamp %f s and delta time %f s."), APMove().MetaData.Timestamp, APMove().MetaData.DeltaTime)\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Input state:"))\
    LogSyncData(APMove().InputState, this, TEXT("Client move trace ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Output state:"))\
    LogSyncData(APMove().OutputState, this, TEXT("Client move trace ::: "));\
  }
#define DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_COMPLEMENTED_APMOVE\
  if (GMCCVars::LogClientMoveTrace != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Client complemented autonomous proxy move with timestamp %f s and delta time %f s."), APMove().MetaData.Timestamp, APMove().MetaData.DeltaTime)\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Input state:"))\
    LogSyncData(APMove().InputState, this, TEXT("Client move trace ::: "));\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Client move trace ::: Output state:"))\
    LogSyncData(APMove().OutputState, this, TEXT("Client move trace ::: "));\
  }

#define DEBUG_LOG_NUM_EXECUTED_REMOTE_PACKETS\
  if (GMCCVars::LogNumExecutedRemoteMoves != 0)\
  {\
    GMC_LOG(\
      LogGMCReplication,\
      PawnOwner,\
      Log,\
      TEXT("Num executed remote moves ::: Executed %3d moves: total moves remaining = %3d (RemoteMoveProcessingChunkSize = %3d) | num exceeding moves this frame = %3d (MaxNumBufferedRemoteMoves = %3d)"),\
      NumMovesToExecute,\
      SV_RemoteMoveExecutionAux.PendingMoves.Num(),\
      RemoteMoveProcessingChunkSize,\
      NumExceedingMoves,\
      MaxNumBufferedRemoteMoves\
    )\
  }
#define DEBUG_LOG_NUM_EXECUTED_REMOTE_PACKETS_PROXY_MOVE\
  if (GMCCVars::LogNumExecutedRemoteMoves != 0)\
  {\
    GMC_LOG(\
      LogGMCReplication,\
      PawnOwner,\
      Log,\
      TEXT("Num executed remote moves ::: Executed proxy move:  total moves remaining = %3d (RemoteMoveProcessingChunkSize = %3d) | num exceeding moves this frame = %3d (MaxNumBufferedRemoteMoves = %3d)"),\
      SV_RemoteMoveExecutionAux.PendingMoves.Num(),\
      RemoteMoveProcessingChunkSize,\
      FMath::Clamp(SV_RemoteMoveExecutionAux.PendingMoves.Num() - MaxNumBufferedRemoteMoves, 0, MAX_int32),\
      MaxNumBufferedRemoteMoves\
    )\
  }

#define DEBUG_LOG_DYNAMIC_BUFFER_TIME_VALUATION\
  if (GMCCVars::LogDynamicBufferTime != 0)\
  {\
    GMC_LOG(\
      LogGMCReplication,\
      PawnOwner,\
      Log,\
      TEXT("Dynamic buffer time ::: Updated buffer time valuation: avg dist = %+.3f (target: %.3f <~ %.3f ~> %.3f) | avg low dist = %+.3f | avg diff = %+.3f | extrap = %d | current buffer time = %.2f | current total delay = %.2f"),\
      DynamicBufferTimeAux.AllBuffer.GetTrimmedMean(DynamicBufferTimeAux.TRIM_PERCENTAGE),\
      DynamicBufferTimeAux.TargetDist - DynamicBufferTimeAux.MIN_TARGET_DIST_TOLERANCE,\
      DynamicBufferTimeAux.TargetDist,\
      DynamicBufferTimeAux.TargetDist + DynamicBufferTimeAux.TargetDistTolerance,\
      DynamicBufferTimeAux.LowBuffer.GetMean(),\
      DynamicBufferTimeAux.DiffBuffer.GetMean(),\
      SimulationAux.bIsExtrapolating,\
      AdaptiveDelayParams.BufferTime,\
      AdaptiveDelayParams.CurrentDelay\
    )\
  }
#define DEBUG_LOG_DYNAMIC_BUFFER_TIME_REQUEST\
  if (GMCCVars::LogDynamicBufferTime != 0)\
  {\
    GMC_LOG(\
      LogGMCReplication,\
      PawnOwner,\
      Log,\
      TEXT("Dynamic buffer time ::: Requested buffer time of %.2f seconds."),\
      DynamicBufferTimeAux.RequestedBufferTime\
    )\
  }
#define DEBUG_LOG_DYNAMIC_BUFFER_TIME_APPLY\
  if (GMCCVars::LogDynamicBufferTime != 0)\
  {\
    GMC_LOG(\
      LogGMCReplication,\
      PawnOwner,\
      Log,\
      TEXT("Dynamic buffer time ::: Applied buffer time of %.2f seconds."),\
      AdaptiveDelayParams.BufferTime\
    )\
  }

#define DEBUG_LOG_FORCED_NET_UPDATES_CLIENT_CORRECTION\
  if (GMCCVars::LogForcedNetUpdates != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Log forced net updates ::: Forced update due to client correction (actor net update frequency = %f)"), PawnOwner->NetUpdateFrequency)\
  }
#define DEBUG_LOG_FORCED_NET_UPDATES_SIM_LOCATION_THRESHOLD\
  if (GMCCVars::LogForcedNetUpdates != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Log forced net updates ::: Forced update due to exceeding location diff threshold (diff = %f > threshold = %f)"), RepLocationDelta, SimLocationUpdateThreshold)\
  }
#define DEBUG_LOG_FORCED_NET_UPDATES_SYNC_DATA\
  if (GMCCVars::LogForcedNetUpdates != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Log forced net updates ::: Forced update due to bound simulation setting."))\
  }
#define DEBUG_LOG_FORCED_NET_UPDATES_CUSTOM\
  if (GMCCVars::LogForcedNetUpdates != 0)\
  {\
    GMC_LOG(LogGMCReplication, PawnOwner, Log, TEXT("Log forced net updates ::: Forced update due to custom condition."))\
  }

#else

#define DEBUG_PRINT_LOCAL_STATS(PawnNetRole)
#define DEBUG_DISPLAY_LOCAL_STATS
#define DEBUG_PRINT_NET_STATS(PawnNetRole)
#define DEBUG_DISPLAY_NET_STATS
#define DEBUG_SHOW_PAWN_NET_ROLE
#define DEBUG_SHOW_PAWN_SIMULATION_DELAY
#define DEBUG_SHOW_CLIENT_CORRECTIONS
#define DEBUG_SHOW_CLIENT_ERRORS
#define DEBUG_LOG_REPLAY_APMOVE
#define DEBUG_LOG_REPLAY_CLIENT_STATE_INITIAL
#define DEBUG_LOG_REPLAY_CLIENT_STATE_BEFORE_REPLAY
#define DEBUG_LOG_REPLAY_CLIENT_STATE_AFTER_REPLAY
#define DEBUG_LOG_SMOOTHING_PARAMS
#define DEBUG_LOG_SMOOTHING_NONE
#define DEBUG_LOG_SMOOTHING_MATCH_LATEST
#define DEBUG_LOG_SMOOTHING_DELAY
#define DEBUG_LOG_SMOOTHING_SIMULATED_INPUT
#define DEBUG_LOG_SMOOTHING_SIMULATED_OUTPUT
#define DEBUG_LOG_SMOOTHING_SIMULATED_NEW_STATE
#define DEBUG_LOG_EXTRAPOLATION
#define DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_EXECUTED_MOVE
#define DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_SENT_MOVES
#define DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_RECEIVED_MOVES
#define DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_EXECUTED_MOVE
#define DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_VALIDATED_MOVE
#define DEBUG_LOG_CLIENT_MOVE_TRACE_SERVER_SAVED_APMOVE
#define DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_RECEIVED_APMOVE
#define DEBUG_LOG_CLIENT_MOVE_TRACE_CLIENT_COMPLEMENTED_APMOVE
#define DEBUG_LOG_NUM_EXECUTED_REMOTE_PACKETS
#define DEBUG_LOG_NUM_EXECUTED_REMOTE_PACKETS_PROXY_MOVE
#define DEBUG_LOG_DYNAMIC_BUFFER_TIME_VALUATION
#define DEBUG_LOG_DYNAMIC_BUFFER_TIME_REQUEST
#define DEBUG_LOG_DYNAMIC_BUFFER_TIME_APPLY
#define DEBUG_LOG_FORCED_NET_UPDATES_CLIENT_CORRECTION
#define DEBUG_LOG_FORCED_NET_UPDATES_SIM_LOCATION_THRESHOLD
#define DEBUG_LOG_FORCED_NET_UPDATES_SYNC_DATA
#define DEBUG_LOG_FORCED_NET_UPDATES_CUSTOM

#endif
