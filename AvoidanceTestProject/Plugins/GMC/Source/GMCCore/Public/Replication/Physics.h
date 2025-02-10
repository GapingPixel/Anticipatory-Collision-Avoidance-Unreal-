// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Smoothing.h"
#include "Physics.generated.h"

UENUM(BlueprintType)
enum class EGMC_PhysicsInterpolationMode : uint8
{
  None,
  FixedDelay,
  AdaptiveDelay,
  MAX UMETA(Hidden),
};

UENUM(BlueprintType)
enum class EGMC_PhysicsInterpolationMethod : uint8
{
  Default,
  Custom1,
  Custom2,
  Custom3,
  Custom4,
  Custom5,
  Custom6,
  Custom7,
  Custom8,
  MAX UMETA(Hidden),
};

inline EGMC_InterpolationMode ToDefaultInterpolationMode(EGMC_PhysicsInterpolationMode Mode)
{
  switch (Mode)
  {
    case EGMC_PhysicsInterpolationMode::None:
      return EGMC_InterpolationMode::None;
    case EGMC_PhysicsInterpolationMode::FixedDelay:
      return EGMC_InterpolationMode::FixedDelay;
    case EGMC_PhysicsInterpolationMode::AdaptiveDelay:
      return EGMC_InterpolationMode::AdaptiveDelay;
    default:
      gmc_ckne();
  }
  gmc_ckne()
  return EGMC_InterpolationMode{};
}

inline EGMC_PhysicsInterpolationMode ToPhysicsInterpolationMode(EGMC_InterpolationMode Mode)
{
  switch (Mode)
  {
    case EGMC_InterpolationMode::None:
      return EGMC_PhysicsInterpolationMode::None;
    case EGMC_InterpolationMode::FixedDelay:
      return EGMC_PhysicsInterpolationMode::FixedDelay;
    case EGMC_InterpolationMode::MatchLatest:
    case EGMC_InterpolationMode::UniformSimulation:
    case EGMC_InterpolationMode::CumulativeSimulation:
    case EGMC_InterpolationMode::AdaptiveDelay:
      return EGMC_PhysicsInterpolationMode::AdaptiveDelay;
    default:
      gmc_ckne();
  }
  gmc_ckne()
  return EGMC_PhysicsInterpolationMode{};
}

USTRUCT(BlueprintType)
struct GMCCORE_API FGMC_PhysicsInterpState
{
  GENERATED_BODY()

  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  FVector LinearVelocity{0.};

  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  FVector AngularVelocity{0.};

  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  FVector ActorLocation{0.};

  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  FRotator ActorRotation{0.};

  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  double Timestamp{-1.};

  FGMC_PhysicsInterpState() = default;

  FGMC_PhysicsInterpState(
    FVector LinearVelocity,
    FVector AngularVelocity,
    FVector ActorLocation,
    FRotator ActorRotation,
    double Timestamp
  )
  : LinearVelocity(LinearVelocity),
    AngularVelocity(AngularVelocity),
    ActorLocation(ActorLocation),
    ActorRotation(ActorRotation),
    Timestamp(Timestamp)
  {}
};

USTRUCT(BlueprintType)
struct GMCCORE_API FGMC_ServerAuthPhysicsSettings
{
  GENERATED_BODY()

  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  EGMC_PhysicsInterpolationMode ClientInterpolationMode{EGMC_PhysicsInterpolationMode::AdaptiveDelay};
  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  EGMC_PhysicsInterpolationMethod ClientInterpolationMethod{EGMC_PhysicsInterpolationMethod::Default};

  // Saved internally to be restored when physics simulation is disabled again, has no other purpose and does not need to be sent to the client.
  bool bUseClientPrediction{false};
  bool bSmoothRemoteListenServerPawn{false};
  EGMC_InterpolationMode InterpolationMode{};
  EGMC_ExtrapolationMode ExtrapolationMode{};

  void SaveSettings(class UGMC_ReplicationCmp* ReplicationComponent);
  void ApplySettings(UGMC_ReplicationCmp* ReplicationComponent);

  bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

  FGMC_ServerAuthPhysicsSettings() = default;

  FGMC_ServerAuthPhysicsSettings(
    EGMC_PhysicsInterpolationMode ClientInterpolationMode,
    EGMC_PhysicsInterpolationMethod ClientInterpolationMethod
  )
    : ClientInterpolationMode(ClientInterpolationMode),
      ClientInterpolationMethod(ClientInterpolationMethod)
  {}
};

template<>
struct TStructOpsTypeTraits<FGMC_ServerAuthPhysicsSettings> : public TStructOpsTypeTraitsBase2<FGMC_ServerAuthPhysicsSettings>
{
  enum { WithNetSerializer = true };
};

USTRUCT(BlueprintType)
struct GMCCORE_API FGMC_ClientAuthPhysicsSettings
{
  GENERATED_BODY()

  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  EGMC_PhysicsInterpolationMode ServerInterpolationMode{EGMC_PhysicsInterpolationMode::AdaptiveDelay};
  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  EGMC_PhysicsInterpolationMethod ServerInterpolationMethod{EGMC_PhysicsInterpolationMethod::Default};
  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  bool bRollbackServerPawns{false};
  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  bool bRollbackClientPawns{false};
  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  bool bRollbackGenericServerActors{false};
  UPROPERTY(Transient, BlueprintReadWrite, Category = "General Movement Component")
  bool bRollbackGenericClientActors{false};

  // Saved internally to be restored when physics simulation is disabled again, has no other purpose and does not need to be sent to the client.
  bool bUseClientPrediction{true};
  bool bAssumeClientState{true};
  EGMC_InterpolationMode InterpolationMode{};
  EGMC_ExtrapolationMode ExtrapolationMode{};

  void SaveSettings(UGMC_ReplicationCmp* ReplicationComponent);
  void ApplySettings(UGMC_ReplicationCmp* ReplicationComponent);

  bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

  FGMC_ClientAuthPhysicsSettings() = default;

  FGMC_ClientAuthPhysicsSettings(
    EGMC_PhysicsInterpolationMode ServerInterpolationMode,
    EGMC_PhysicsInterpolationMethod ServerInterpolationMethod,
    bool bRollbackServerPawns,
    bool bRollbackClientPawns,
    bool bRollbackGenericServerActors,
    bool bRollbackGenericClientActors
  )
    : ServerInterpolationMode(ServerInterpolationMode),
      ServerInterpolationMethod(ServerInterpolationMethod),
      bRollbackServerPawns(bRollbackServerPawns),
      bRollbackClientPawns(bRollbackClientPawns),
      bRollbackGenericServerActors(bRollbackGenericServerActors),
      bRollbackGenericClientActors(bRollbackGenericClientActors)
  {}
};

template<>
struct TStructOpsTypeTraits<FGMC_ClientAuthPhysicsSettings> : public TStructOpsTypeTraitsBase2<FGMC_ClientAuthPhysicsSettings>
{
  enum { WithNetSerializer = true };
};
