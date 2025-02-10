// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCAggregator.h"
#include "GMCPlayerController.h"
#include "GMCAIController.h"
#include "GMCReplicationComponent.h"
#include "GMCMovementUtilityComponent.h"
#include "GMCOrganicMovementComponent.h"
#include "GMCGoldSrcMovementComponent.h"
#include "GMCPawn.h"
#include "GMCRollbackActor.h"
#include "GMCRollbackPlatform.h"
#include "GMCLog.h"

DECLARE_CYCLE_STAT(TEXT("ControllerTicks"), STAT_ControllerTicks, STATGROUP_AGMC_Aggregator)
DECLARE_CYCLE_STAT(TEXT("PawnTicks"), STAT_PawnTicks, STATGROUP_AGMC_Aggregator)
DECLARE_CYCLE_STAT(TEXT("MovementComponentTicks"), STAT_MovementComponentTicks, STATGROUP_AGMC_Aggregator)
DECLARE_CYCLE_STAT(TEXT("RollbackActorTicks"), STAT_RollbackActorTicks, STATGROUP_AGMC_Aggregator)
DECLARE_CYCLE_STAT(TEXT("MeshComponentTicks"), STAT_MeshComponentTicks, STATGROUP_AGMC_Aggregator)

AGMC_Aggregator::AGMC_Aggregator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = true;
  bAllowTickBeforeBeginPlay = false;
}

void AGMC_Aggregator::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  if (bIsFirstUpdate || bEnableAggregateTick != bWasEnabledLastFrame)
  {
    bWasEnabledLastFrame = bEnableAggregateTick;
    bIsFirstUpdate = false;
    CALL_NATIVE_EVENT_ALWAYS(this, OnAggregateTickToggled, bEnableAggregateTick);
  }

  if (!bEnableAggregateTick)
  {
    return;
  }

  if (bAggregateControllers)
  {
    SCOPE_CYCLE_COUNTER(STAT_ControllerTicks)

    bool bNeedsReordering = false;
    int32 PreviousOrderNumber = -1;
    for (int32 Index = 0; Index < Controllers.Num(); ++Index)
    {
      const auto& Controller = Controllers[Index];

      if (!IsValid(Controller))
      {
        Controllers.RemoveAt(Index);
        --Index;
        continue;
      }

      if (!bNeedsReordering)
      {
        bNeedsReordering = !VerifyOrder(GetControllerOrderNumber(Controller), PreviousOrderNumber);
      }

      if (Controller->PrimaryActorTick.IsTickFunctionEnabled())
      {
        UE_LOG(LogGMCReplication, VeryVerbose, TEXT("Possible double tick of \"%s\""), *Controller->GetName())
        Controller->PrimaryActorTick.SetTickFunctionEnable(false);
      }

      if (Controller->PrimaryActorTick.bCanEverTick)
      {
        Controller->TickActor(DeltaTime * Controller->CustomTimeDilation, ELevelTick::LEVELTICK_All, Controller->PrimaryActorTick);
      }
    }

    if (bNeedsReordering)
    {
      SortControllers();
    }
  }

  if (bAggregatePawns)
  {
    SCOPE_CYCLE_COUNTER(STAT_PawnTicks)

    bool bNeedsReordering = false;
    int32 PreviousOrderNumber = -1;
    for (int32 Index = 0; Index < Pawns.Num(); ++Index)
    {
      const auto& Pawn = Pawns[Index];

      if (!IsValid(Pawn))
      {
        Pawns.RemoveAt(Index);
        --Index;
        continue;
      }

      if (!bNeedsReordering)
      {
        bNeedsReordering = !VerifyOrder(GetPawnOrderNumber(Pawn), PreviousOrderNumber);
      }

      if (Pawn->PrimaryActorTick.IsTickFunctionEnabled())
      {
        UE_LOG(LogGMCReplication, VeryVerbose, TEXT("Possible double tick of \"%s\""), *Pawn->GetName())
        Pawn->PrimaryActorTick.SetTickFunctionEnable(false);
      }

      if (Pawn->PrimaryActorTick.bCanEverTick)
      {
        Pawn->TickActor(DeltaTime * Pawn->CustomTimeDilation, ELevelTick::LEVELTICK_All, Pawn->PrimaryActorTick);
      }
    }

    if (bNeedsReordering)
    {
      SortPawns();
    }
  }

  if (bAggregateMovementComponents)
  {
    SCOPE_CYCLE_COUNTER(STAT_MovementComponentTicks)

    bool bNeedsReordering = false;
    int32 PreviousOrderNumber = -1;
    for (int32 Index = 0; Index < MovementComponents.Num(); ++Index)
    {
      const auto& MovementComponent = MovementComponents[Index];

      if (!IsValid(MovementComponent))
      {
        MovementComponents.RemoveAt(Index);
        --Index;
        continue;
      }

      if (!bNeedsReordering)
      {
        bNeedsReordering = !VerifyOrder(GetMovementComponentOrderNumber(MovementComponent), PreviousOrderNumber);
      }

      if (MovementComponent->PrimaryComponentTick.IsTickFunctionEnabled())
      {
        UE_LOG(LogGMCReplication, VeryVerbose, TEXT("Possible double tick of \"%s\""), *MovementComponent->GetName())
        MovementComponent->PrimaryComponentTick.SetTickFunctionEnable(false);
      }

      if (MovementComponent->PrimaryComponentTick.bCanEverTick)
      {
        const auto& ComponentOwner = MovementComponent->GetOwner();
        const float ComponentTimeDilation = ComponentOwner ? ComponentOwner->CustomTimeDilation : 1.f;
        MovementComponent->TickComponent(DeltaTime * ComponentTimeDilation, ELevelTick::LEVELTICK_All, &MovementComponent->PrimaryComponentTick);
      }
    }

    if (bNeedsReordering)
    {
      SortMovementComponents();
    }
  }

  if (bAggregateRollbackActors)
  {
    SCOPE_CYCLE_COUNTER(STAT_RollbackActorTicks)

    bool bNeedsReordering = false;
    int32 PreviousOrderNumber = -1;
    for (int32 Index = 0; Index < RollbackActors.Num(); ++Index)
    {
      const auto& RollbackActor = RollbackActors[Index];

      if (!IsValid(RollbackActor))
      {
        RollbackActors.RemoveAt(Index);
        --Index;
        continue;
      }

      if (!bNeedsReordering)
      {
        bNeedsReordering = !VerifyOrder(GetRollbackActorOrderNumber(RollbackActor), PreviousOrderNumber);
      }

      if (RollbackActor->PrimaryActorTick.IsTickFunctionEnabled())
      {
        UE_LOG(LogGMCReplication, VeryVerbose, TEXT("Possible double tick of \"%s\""), *RollbackActor->GetName())
        RollbackActor->PrimaryActorTick.SetTickFunctionEnable(false);
      }

      if (RollbackActor->PrimaryActorTick.bCanEverTick)
      {
        RollbackActor->TickActor(DeltaTime * RollbackActor->CustomTimeDilation, ELevelTick::LEVELTICK_All, RollbackActor->PrimaryActorTick);
      }
    }

    if (bNeedsReordering)
    {
      SortRollbackActors();
    }
  }

  if (bAggregateMeshComponents)
  {
    SCOPE_CYCLE_COUNTER(STAT_MeshComponentTicks)

    bool bNeedsReordering = false;
    int32 PreviousOrderNumber = -1;
    for (int32 Index = 0; Index < MeshComponents.Num(); ++Index)
    {
      const auto& MeshComponent = MeshComponents[Index];

      if (!IsValid(MeshComponent))
      {
        MeshComponents.RemoveAt(Index);
        --Index;
        continue;
      }

      if (!bNeedsReordering)
      {
        bNeedsReordering = !VerifyOrder(GetMeshComponentOrderNumber(MeshComponent), PreviousOrderNumber);
      }

      if (MeshComponent->PrimaryComponentTick.IsTickFunctionEnabled())
      {
        UE_LOG(LogGMCReplication, VeryVerbose, TEXT("Possible double tick of \"%s\""), *MeshComponent->GetName())
        MeshComponent->PrimaryComponentTick.SetTickFunctionEnable(false);
      }

      if (MeshComponent->PrimaryComponentTick.bCanEverTick)
      {
        const auto& ComponentOwner = MeshComponent->GetOwner();
        const float ComponentTimeDilation = ComponentOwner ? ComponentOwner->CustomTimeDilation : 1.f;
        MeshComponent->TickComponent(DeltaTime * ComponentTimeDilation, ELevelTick::LEVELTICK_All, &MeshComponent->PrimaryComponentTick);
      }
    }

    if (bNeedsReordering)
    {
      SortMeshComponents();
    }
  }
}

AGMC_Aggregator* AGMC_Aggregator::GetGMCAggregator(UObject* Context)
{
  if (!IsValid(Context))
  {
    return nullptr;
  }

  TArray<AActor*> Actors{};
  UGameplayStatics::GetAllActorsOfClass(Context->GetWorld(), AGMC_Aggregator::StaticClass(), Actors);

  if (Actors.Num() == 0)
  {
    // No aggregator was placed in the world.
    return nullptr;
  }

  gmc_ck(Actors.Num() == 1)

  if (Actors.Num() > 1)
  {
    UE_LOG(
      LogGMCReplication,
      Error,
      TEXT("More than one actor of class %s found in the current world."),
      TO_STR(AGMC_Aggregator)
    )

    return nullptr;
  }

  const auto& Aggregator = Cast<AGMC_Aggregator>(Actors[0]);
  gmc_ck(IsValid(Aggregator))

  return Aggregator;
}

void AGMC_Aggregator::RegisterController(AController* Controller)
{
  if (!IsValid(Controller))
  {
    return;
  }

  Controllers.AddUnique(Controller);

  SortControllers();
}

void AGMC_Aggregator::RegisterPawn(APawn* Pawn)
{
  if (!IsValid(Pawn))
  {
    return;
  }

  Pawns.AddUnique(Pawn);

  SortPawns();
}

void AGMC_Aggregator::RegisterMovementComponent(UMovementComponent* MovementComponent)
{
  if (!IsValid(MovementComponent))
  {
    return;
  }

  MovementComponents.AddUnique(MovementComponent);

  SortMovementComponents();
}

void AGMC_Aggregator::RegisterRollbackActor(AGMC_RollbackActor* RollbackActor)
{
  if (!IsValid(RollbackActor))
  {
    return;
  }

  RollbackActors.AddUnique(RollbackActor);

  SortRollbackActors();
}

void AGMC_Aggregator::RegisterMeshComponent(UMeshComponent* Mesh)
{
  if (!IsValid(Mesh))
  {
    return;
  }

  MeshComponents.AddUnique(Mesh);

  SortMeshComponents();
}

void AGMC_Aggregator::UnregisterController(AController* Controller)
{
  const int32 NumRemoved = Controllers.Remove(Controller);
  gmc_ck(NumRemoved <= 1)
}

void AGMC_Aggregator::UnregisterPawn(APawn* Pawn)
{
  const int32 NumRemoved = Pawns.Remove(Pawn);
  gmc_ck(NumRemoved <= 1)
}

void AGMC_Aggregator::UnregisterMovementComponent(UMovementComponent* MovementComponent)
{
  const int32 NumRemoved = MovementComponents.Remove(MovementComponent);
  gmc_ck(NumRemoved <= 1)
}

void AGMC_Aggregator::UnregisterRollbackActor(AGMC_RollbackActor* RollbackActor)
{
  const int32 NumRemoved = RollbackActors.Remove(RollbackActor);
  gmc_ck(NumRemoved <= 1)
}

void AGMC_Aggregator::UnregisterMeshComponent(UMeshComponent* Mesh)
{
  const int32 NumRemoved = MeshComponents.Remove(Mesh);
  gmc_ck(NumRemoved <= 1)
}

void AGMC_Aggregator::SortControllers()
{
  Algo::Sort(
    Controllers,
    [&](const AController* A, const AController* B) { return GetControllerOrderNumber(A) < GetControllerOrderNumber(B);
  });
}

void AGMC_Aggregator::SortPawns()
{
  Algo::Sort(
    Pawns,
    [&](const APawn* A, const APawn* B) { return GetPawnOrderNumber(A) < GetPawnOrderNumber(B);
  });
}

void AGMC_Aggregator::SortMovementComponents()
{
  Algo::Sort(
    MovementComponents,
    [&](const UMovementComponent* A, const UMovementComponent* B) { return GetMovementComponentOrderNumber(A) < GetMovementComponentOrderNumber(B); }
  );
}

void AGMC_Aggregator::SortRollbackActors()
{
  Algo::Sort(
    RollbackActors,
    [&](const AGMC_RollbackActor* A, const AGMC_RollbackActor* B) { return GetRollbackActorOrderNumber(A) < GetRollbackActorOrderNumber(B); }
  );
}

void AGMC_Aggregator::SortMeshComponents()
{
  Algo::Sort(
    MeshComponents,
    [&](const UMeshComponent* A, const UMeshComponent* B) { return GetMeshComponentOrderNumber(A) < GetMeshComponentOrderNumber(B); }
  );
}

const TArray<AController*>& AGMC_Aggregator::GetControllers() const
{
  return Controllers;
}

const TArray<APawn*>& AGMC_Aggregator::GetPawns() const
{
  return Pawns;
}

const TArray<UMovementComponent*>& AGMC_Aggregator::GetMovementComponents() const
{
  return MovementComponents;
}

const TArray<AGMC_RollbackActor*>& AGMC_Aggregator::GetRollbackActors() const
{
  return RollbackActors;
}

const TArray<UMeshComponent*>& AGMC_Aggregator::GetMeshComponents() const
{
  return MeshComponents;
}

void AGMC_Aggregator::SetTickFunctionsEnabled(bool bInEnable)
{
  for (const auto& Controller : Controllers)
  {
    Controller->PrimaryActorTick.SetTickFunctionEnable(bInEnable);
  }

  for (const auto& Pawn : Pawns)
  {
    Pawn->PrimaryActorTick.SetTickFunctionEnable(bInEnable);
  }

  for (const auto& MovementComponent : MovementComponents)
  {
    MovementComponent->PrimaryComponentTick.SetTickFunctionEnable(bInEnable);
  }

  for (const auto& RollbackActor : RollbackActors)
  {
    RollbackActor->PrimaryActorTick.SetTickFunctionEnable(bInEnable);
  }

  for (const auto& MeshComponent : MeshComponents)
  {
    MeshComponent->PrimaryComponentTick.SetTickFunctionEnable(bInEnable);
  }
}

void AGMC_Aggregator::OnAggregateTickToggled_Implementation(bool bEnabled)
{
  if (!bEnabled)
  {
    // Enable all tick functions again by default when the aggregate tick is disabled.
    SetTickFunctionsEnabled(true);
  }
}

int32 AGMC_Aggregator::GetControllerOrderNumber(const AController* Controller)
{
  // Tick order:
  int32 LocalGMCPlayerController = 0;
  int32 GMCPlayerController = 1;
  int32 PlayerController = 2;
  int32 GMCAIController = 3;
  int32 AIController = 4;
  int32 OtherController = 5;

  if (Cast<AGMC_PlayerController>(Controller) && Controller->IsLocalPlayerController())
  {
    return LocalGMCPlayerController;
  }

  if (Cast<AGMC_PlayerController>(Controller))
  {
    return GMCPlayerController;
  }

  if (Cast<APlayerController>(Controller))
  {
    return PlayerController;
  }

  if (Cast<AGMC_AIController>(Controller))
  {
    return GMCAIController;
  }

  if (Cast<AAIController>(Controller))
  {
    return AIController;
  }

  return OtherController;
}

int32 AGMC_Aggregator::GetPawnOrderNumber(const APawn* Pawn)
{
  // Tick order:
  int32 GMCPawn = 0;
  int32 OtherPawn = 1;

  if (Cast<AGMC_Pawn>(Pawn))
  {
    return GMCPawn;
  }

  return OtherPawn;
}

int32 AGMC_Aggregator::GetMovementComponentOrderNumber(const UMovementComponent* MovementComponent)
{
  // Tick order:
  int32 GMCReplicationComponent_Simulated = 0;
  int32 GMCMovementUtilityComponent_Simulated = 1;
  int32 GMCOrganicMovementComponent_Simulated = 2;
  int32 GMCGoldSrcMovementComponent_Simulated = 3;
  int32 GMCReplicationComponent = 4;
  int32 GMCMovementUtilityComponent = 5;
  int32 GMCOrganicMovementComponent = 6;
  int32 GMCGoldSrcMovementComponent = 7;
  int32 OtherMovementComponent = 8;

  if (const auto& MovementComponentCast = Cast<UGMC_GoldSrcMovementCmp>(MovementComponent))
  {
    return MovementComponentCast->IsSimulatedPawn() ? GMCGoldSrcMovementComponent_Simulated : GMCGoldSrcMovementComponent;
  }

  if (const auto& MovementComponentCast = Cast<UGMC_OrganicMovementCmp>(MovementComponent))
  {
    return MovementComponentCast->IsSimulatedPawn() ? GMCOrganicMovementComponent_Simulated : GMCOrganicMovementComponent;
  }

  if (const auto& MovementComponentCast = Cast<UGMC_MovementUtilityCmp>(MovementComponent))
  {
    return MovementComponentCast->IsSimulatedPawn() ? GMCMovementUtilityComponent_Simulated : GMCMovementUtilityComponent;
  }

  if (const auto& MovementComponentCast = Cast<UGMC_ReplicationCmp>(MovementComponent))
  {
    return MovementComponentCast->IsSimulatedPawn() ? GMCReplicationComponent_Simulated : GMCReplicationComponent;
  }

  return OtherMovementComponent;
}

int32 AGMC_Aggregator::GetRollbackActorOrderNumber(const AGMC_RollbackActor* RollbackActor)
{
  // Tick order:
  int32 GMCRollbackActor = 0;
  int32 GMCRollbackPlatform = 1;
  int32 OtherRollbackActor = 2;

  if (Cast<AGMC_RollbackPlatform>(RollbackActor))
  {
    return GMCRollbackPlatform;
  }

  if (Cast<AGMC_RollbackActor>(RollbackActor))
  {
    return GMCRollbackActor;
  }

  return OtherRollbackActor;
}

int32 AGMC_Aggregator::GetMeshComponentOrderNumber(const UMeshComponent* MeshComponent)
{
  // Tick order:
  int32 SkeletalMeshComponent = 0;
  int32 OtherMeshComponent = 1;

  if (Cast<USkeletalMeshComponent>(MeshComponent))
  {
    return SkeletalMeshComponent;
  }

  return OtherMeshComponent;
}

bool AGMC_Aggregator::VerifyOrder(int32 CurrentOrderNumber, int32& InOutPreviousOrderNumber) const
{
  if (CurrentOrderNumber < InOutPreviousOrderNumber)
  {
    return false;
  }

  InOutPreviousOrderNumber = CurrentOrderNumber;

  return true;
}
