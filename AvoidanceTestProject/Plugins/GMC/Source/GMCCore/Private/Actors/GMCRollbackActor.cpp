// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCRollbackActor.h"
#include "GMCPlayerController.h"
#include "GMCPawn.h"
#include "GMCAggregator.h"
#include "GMCLog.h"

AGMC_RollbackActor::AGMC_RollbackActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = true;
  bAllowTickBeforeBeginPlay = false;
}

void AGMC_RollbackActor::BeginPlay()
{
  Super::BeginPlay();

  gmc_ck(PrimaryActorTick.IsTickFunctionRegistered())

  GMCAggregator = AGMC_Aggregator::GetGMCAggregator(this);

  if (IsValid(GMCAggregator))
  {
    GMCAggregator->RegisterRollbackActor(this);
  }

  BaseLinearVelocity = LinearVelocity;
  BaseAngularVelocity = AngularVelocity;
  BaseTransform = GetActorTransform();
}

void AGMC_RollbackActor::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  ManagePrerequisiteTicks();

  if (!HasTicked())
  {
    CALL_NATIVE_EVENT_CONDITIONAL(bNoBlueprintEvents, this, UpdateState, GetTime(), DeltaTime, FGMC_Move{}, EGMC_NetContext::RegularTickUpdate);
  }

  // Reset the flag. It will be set by the replication component of the local pawn during the next frame if applicable.
  SetTicked(false);
}

FVector AGMC_RollbackActor::GetVelocity() const
{
  return GetLinearVelocity();
}

FVector AGMC_RollbackActor::GetLinearVelocity() const
{
  return LinearVelocity;
}

FVector AGMC_RollbackActor::GetAngularVelocityInDegrees() const
{
  return AngularVelocity;
}

FVector AGMC_RollbackActor::GetAngularVelocityInRadians() const
{
  return FMath::DegreesToRadians(AngularVelocity);
}

void AGMC_RollbackActor::SetLinearVelocity(const FVector& NewVelocity)
{
  LinearVelocity = NewVelocity;
}

void AGMC_RollbackActor::SetAngularVelocityInDegrees(const FVector& NewVelocity)
{
  AngularVelocity = NewVelocity;
}

void AGMC_RollbackActor::SetAngularVelocityInRadians(const FVector& NewVelocity)
{
  AngularVelocity = FMath::RadiansToDegrees(NewVelocity);
}

FVector AGMC_RollbackActor::ComputeTangentialVelocity(const FVector& WorldLocation) const
{
  const FVector RadialDistanceToActor = WorldLocation - GetActorLocation();
  return GetAngularVelocityInRadians() ^ RadialDistanceToActor;
}

double AGMC_RollbackActor::GetTime() const
{
  if (IsNetMode(NM_Client))
  {
    if (const auto& GameInstance = GetGameInstance())
    {
      const auto& LocalController = Cast<AGMC_PlayerController>(GameInstance->GetFirstLocalPlayerController());
      if (LocalController)
      {
        return LocalController->CL_GetSyncedWorldTimeSeconds();
      }
    }

    GMC_LOG(LogGMCRollbackActor, this, Warning, TEXT("Client was unable to retrieve the server world time."))
    return 0.;
  }

  if (const auto& World = GetWorld())
  {
    return World->GetRealTimeSeconds();
  }

  GMC_LOG(LogGMCRollbackActor, this, Warning, TEXT("Server world time could not be retrieved."))
  return 0.;
}

void AGMC_RollbackActor::SaveState_Implementation(EGMC_NetContext Context)
{
  SavedRollbackState.Transform.SetComponents(GetActorQuat(), GetActorLocation(), GetActorScale3D());
  SavedRollbackState.LinearVelocity = LinearVelocity;
  SavedRollbackState.AngularVelocity = AngularVelocity;
}

void AGMC_RollbackActor::LoadState_Implementation(EGMC_NetContext Context)
{
  SetActorTransform(SavedRollbackState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
  LinearVelocity = SavedRollbackState.LinearVelocity;
  AngularVelocity = SavedRollbackState.AngularVelocity;
}

void AGMC_RollbackActor::SetTicked(bool bNewValue)
{
  bTicked = bNewValue;
}

bool AGMC_RollbackActor::HasTicked() const
{
  return bTicked;
}

void AGMC_RollbackActor::ManagePrerequisiteTicks()
{
  const bool bAggregates = IsValid(GMCAggregator) && GMCAggregator->bEnableAggregateTick;

  const auto& LocalPlayerController = GEngine ? Cast<AGMC_PlayerController>(GEngine->GetFirstLocalPlayerController(GetWorld())) : nullptr;
  if (LocalPlayerController)
  {
    if (!bAggregates || !GMCAggregator->bAggregateControllers)
    {
      // The local controller must tick first to update the world time.
      AddTickPrerequisiteActor(LocalPlayerController);
    }

    if (!bAggregates || !GMCAggregator->bAggregateMovementComponents)
    {
      // The replication component of the player-controlled actor should update all rollback actors (meaning the replication component needs to tick before the
      // rollback actor) so we can take player input into account. If the replication component did not call UpdateState on the actor during this frame (e.g.
      // because it doesn't exist) the state will be updated in the actor's regular tick function.
      if (const auto& LocalPawn = Cast<AGMC_Pawn>(LocalPlayerController->GetPawn()))
      {
        if (const auto& ReplicationComponent = Cast<UGMC_ReplicationCmp>(LocalPawn->GetMovementComponent()))
        {
          AddTickPrerequisiteComponent(ReplicationComponent);
        }
      }
    }
  }
}
