// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCPawn.h"
#include "GMCAggregator.h"
#include "GMCLog.h"

AGMC_Pawn::AGMC_Pawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = true;
  SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
  BaseEyeHeight = 75.f;
  bReplicates = true;
  NetUpdateFrequency = 25.f;
  MinNetUpdateFrequency = 10.f;

  // Default actor movement replication must be disabled to use the replication component.
  SetReplicateMovement(false);

  // When true, this causes pawns that were already placed in the world during map load not to be destroyed when they are net culled. This is undesirable for
  // remotely controlled pawns as they will be "stuck" in their last state until they become relevant again.
  bNetLoadOnClient = false;
}

void AGMC_Pawn::BeginPlay()
{
  Super::BeginPlay();

  gmc_ck(PrimaryActorTick.IsTickFunctionRegistered())

  GMCAggregator = AGMC_Aggregator::GetGMCAggregator(this);

  if (IsValid(GMCAggregator))
  {
    GMCAggregator->RegisterPawn(this);
  }
}

void AGMC_Pawn::PostInitializeComponents()
{
  Super::PostInitializeComponents();

  ReplicationComponent = FindComponentByClass<UGMC_ReplicationCmp>();

  if (!IsValid(ReplicationComponent))
  {
    ReplicationComponent = nullptr;
    return;
  }

  constexpr EObjectFlags NoBindingFlags = RF_ClassDefaultObject | RF_ArchetypeObject | RF_NeedLoad | RF_WillBeLoaded;
  if (!ReplicationComponent->HasAnyFlags(NoBindingFlags))
  {
    CALL_NATIVE_EVENT_CONDITIONAL(ReplicationComponent->bNoBlueprintEvents, ReplicationComponent, BindReplicationData);
  }
  else
  {
    gmc_ckne()
    return;
  }

  gmc_ck(!LocalMove.HasValidTimestamp())
  gmc_ck(!APMove.HasValidTimestamp())
  gmc_ck(!SPMove.HasValidTimestamp())

  InitializeMoves();
}

void AGMC_Pawn::InitializeMoves()
{
  if (!IsValid(ReplicationComponent))
  {
    return;
  }

  using namespace GMCReplication;

  LocalMove = APMove = SPMove = FGMC_Move{};

  LocalMove.NetInfo.NetType = ESimType::LocalMove;
  APMove.NetInfo.NetType = ESimType::APMove;
  SPMove.NetInfo.NetType = ESimType::SPMove;
  LocalMove.NetInfo.OwningComponent = ReplicationComponent;
  APMove.NetInfo.OwningComponent = ReplicationComponent;
  SPMove.NetInfo.OwningComponent = ReplicationComponent;

  const auto& ReplicationSettings = ReplicationComponent->ReplicationSettings;
  const auto& SyncTagsData = ReplicationComponent->GetSyncTagsData();
  const auto& AliasData = ReplicationComponent->AliasData;
  InitializeSyncData(LocalMove.InputState, ReplicationSettings, SyncTagsData, AliasData, ESimState::Input, ESimType::LocalMove, ReplicationComponent);
  InitializeSyncData(LocalMove.OutputState, ReplicationSettings, SyncTagsData, AliasData, ESimState::Output, ESimType::LocalMove, ReplicationComponent);
  InitializeSyncData(APMove.InputState, ReplicationSettings, SyncTagsData, AliasData, ESimState::Input, ESimType::APMove, ReplicationComponent);
  InitializeSyncData(APMove.OutputState, ReplicationSettings, SyncTagsData, AliasData, ESimState::Output, ESimType::APMove, ReplicationComponent);
  InitializeSyncData(SPMove.InputState, ReplicationSettings, SyncTagsData, AliasData, ESimState::Input, ESimType::SPMove, ReplicationComponent);
  InitializeSyncData(SPMove.OutputState, ReplicationSettings, SyncTagsData, AliasData, ESimState::Output, ESimType::SPMove, ReplicationComponent);
}

bool AGMC_Pawn::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
  CA_SUPPRESS(6011);
  if (
    bAlwaysRelevant ||
    RealViewer == Controller ||
    IsOwnedBy(ViewTarget) ||
    IsOwnedBy(RealViewer) ||
    this == ViewTarget ||
    ViewTarget == GetInstigator() ||
    IsBasedOnActor(ViewTarget) ||
    (ViewTarget && ViewTarget->IsBasedOnActor(this))
  )
  {
    return true;
  }
  else if ((IsHidden() || bOnlyRelevantToOwner) && (!GetRootComponent() || !GetRootComponent()->IsCollisionEnabled()))
  {
    return false;
  }
  else if (const auto& MovementBase = GetMovementBase())
  {
    const auto& BaseActor = MovementBase->GetOwner();
    if (IsValid(BaseActor))
    {
      return BaseActor->IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
    }
  }

  return !GetDefault<AGameNetworkManager>()->bUseDistanceBasedRelevancy || IsWithinNetRelevancyDistance(SrcLocation);
}

void AGMC_Pawn::NotifyRestarted()
{
  if (IsValid(ReplicationComponent))
  {
    ReplicationComponent->ClearTransientData(true);
  }

  Super::NotifyRestarted();
}

void AGMC_Pawn::NotifyControllerChanged()
{
  if (IsValid(ReplicationComponent))
  {
    ReplicationComponent->ClearTransientData(true);
  }

  // Must be called before the parent implementation.
  OnControllerChanged(Controller, PreviousController);

  Super::NotifyControllerChanged();
}

void AGMC_Pawn::OnControllerChanged_Implementation(AController* NewController, AController* OldController)
{
  if (NewController)
  {
    NewController->SetControlRotation(GetActorRotation());
  }
}

void AGMC_Pawn::PossessedBy(AController* NewController)
{
  // Only called on the server.

  if (IsValid(ReplicationComponent))
  {
    ReplicationComponent->ClearTransientData(true);
  }

  Super::PossessedBy(NewController);
}

void AGMC_Pawn::UnPossessed()
{
  // Only called on the server.

  if (IsValid(ReplicationComponent))
  {
    ReplicationComponent->ClearTransientData(true);
  }

  Super::UnPossessed();
}

void AGMC_Pawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
  Super::SetupPlayerInputComponent(PlayerInputComponent);

  if (!IsValid(ReplicationComponent))
  {
    return;
  }

  if (!PlayerInputComponent)
  {
    gmc_ckne()
    return;
  }

  CALL_NATIVE_EVENT_CONDITIONAL(ReplicationComponent->bNoBlueprintEvents, ReplicationComponent, SetupPlayerInputComponent, PlayerInputComponent);
}

UPawnMovementComponent* AGMC_Pawn::GetMovementComponent() const
{
  return IsValid(ReplicationComponent) ? ReplicationComponent : nullptr;
}

UGMC_ReplicationCmp* AGMC_Pawn::GetReplicationComponent() const
{
  return IsValid(ReplicationComponent) ? ReplicationComponent : nullptr;
}

FVector AGMC_Pawn::GetVelocity() const
{
  if (IsValid(ReplicationComponent))
  {
    return ReplicationComponent->GetLinearVelocity_GMC();
  }

  const auto& Root = GetRootComponent();
  if (Root && Root->IsSimulatingPhysics())
  {
    return Root->GetComponentVelocity();
  }

  return FVector::ZeroVector;
}

UPrimitiveComponent* AGMC_Pawn::GetMovementBase() const
{
  if (IsValid(ReplicationComponent))
  {
    return Cast<UPrimitiveComponent>(ReplicationComponent->GetActorBase());
  }

  return nullptr;
}

FRotator AGMC_Pawn::GetBaseAimRotation() const
{
  if (!IsValid(ReplicationComponent))
  {
    return Super::GetBaseAimRotation();
  }

  return ReplicationComponent->GetControlRotationLocalSpace();
}

void AGMC_Pawn::SV_MarkForNetReserialization()
{
  SV_bNeedsNetReserialization = true;
}

bool AGMC_Pawn::SV_NeedsNetReserialization() const
{
  return SV_bNeedsNetReserialization;
}

void AGMC_Pawn::SV_ReceiveClientSendStatus_Implementation(bool bIsUsingUnreliableMoves)
{
  if (!IsValid(ReplicationComponent))
  {
    GMC_LOG(LogGMCPawn, this, Error, TEXT("No movement component of type \"%s\" found."), TO_STR(UGMC_ReplicationCmp))
    gmc_ckne()
    return;
  }

  ReplicationComponent->SV_ReceiveClientSendStatus_Implementation(bIsUsingUnreliableMoves);
}

bool AGMC_Pawn::SV_ReceiveClientSendStatus_Validate(bool bIsUsingUnreliableMoves)
{
  if (!IsValid(ReplicationComponent))
  {
    GMC_LOG(LogGMCPawn, this, Error, TEXT("No movement component of type \"%s\" found."), TO_STR(UGMC_ReplicationCmp))
    gmc_ckne()
    return false;
  }

  return ReplicationComponent->SV_ReceiveClientSendStatus_Validate(bIsUsingUnreliableMoves);
}

void AGMC_Pawn::SV_ReceiveMovesReliable_Implementation(const TArray<FGMC_Move>& RemoteMoves)
{
  if (!IsValid(ReplicationComponent))
  {
    GMC_LOG(LogGMCPawn, this, Error, TEXT("No movement component of type \"%s\" found."), TO_STR(UGMC_ReplicationCmp))
    gmc_ckne()
    return;
  }

  ReplicationComponent->SV_ReceiveMoves_Implementation();
}

bool AGMC_Pawn::SV_ReceiveMovesReliable_Validate(const TArray<FGMC_Move>& RemoteMoves)
{
  if (!IsValid(ReplicationComponent))
  {
    GMC_LOG(LogGMCPawn, this, Error, TEXT("No movement component of type \"%s\" found."), TO_STR(UGMC_ReplicationCmp))
    gmc_ckne()
    return false;
  }

  return ReplicationComponent->SV_ReceiveMoves_Validate();
}

void AGMC_Pawn::SV_ReceiveMovesUnreliable_Implementation(const TArray<FGMC_Move>& RemoteMoves)
{
  if (!IsValid(ReplicationComponent))
  {
    GMC_LOG(LogGMCPawn, this, Error, TEXT("No movement component of type \"%s\" found."), TO_STR(UGMC_ReplicationCmp))
    gmc_ckne()
    return;
  }

  ReplicationComponent->SV_ReceiveMoves_Implementation();
}

bool AGMC_Pawn::SV_ReceiveMovesUnreliable_Validate(const TArray<FGMC_Move>& RemoteMoves)
{
  if (!IsValid(ReplicationComponent))
  {
    GMC_LOG(LogGMCPawn, this, Error, TEXT("No movement component of type \"%s\" found."), TO_STR(UGMC_ReplicationCmp))
    gmc_ckne()
    return false;
  }

  return ReplicationComponent->SV_ReceiveMoves_Validate();
}

void AGMC_Pawn::CL_OnRepAPMove()
{
  if (!IsValid(ReplicationComponent))
  {
    GMC_LOG(LogGMCPawn, this, Error, TEXT("No movement component of type \"%s\" found."), TO_STR(UGMC_ReplicationCmp))
    gmc_ckne()
    return;
  }

  ReplicationComponent->CL_OnRepAPMove();
}

void AGMC_Pawn::CL_OnRepSPMove()
{
  if (!IsValid(ReplicationComponent))
  {
    GMC_LOG(LogGMCPawn, this, Error, TEXT("No movement component of type \"%s\" found."), TO_STR(UGMC_ReplicationCmp))
    gmc_ckne()
    return;
  }

  ReplicationComponent->CL_OnRepSPMove();
}

void AGMC_Pawn::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
  Super::PreReplication(ChangedPropertyTracker);

  if (!IsValid(ReplicationComponent))
  {
    return;
  }

  ReplicationComponent->SV_PreReplication();

  SV_bNeedsNetReserialization = false;
}

void AGMC_Pawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  FDoRepLifetimeParams Params;
  Params.bIsPushBased = false;
  Params.RepNotifyCondition = REPNOTIFY_Always;

  Params.Condition = COND_AutonomousOnly;
  DOREPLIFETIME_WITH_PARAMS_FAST(AGMC_Pawn, APMove, Params);

  Params.Condition = COND_SimulatedOnly;
  DOREPLIFETIME_WITH_PARAMS_FAST(AGMC_Pawn, SPMove, Params);
}
