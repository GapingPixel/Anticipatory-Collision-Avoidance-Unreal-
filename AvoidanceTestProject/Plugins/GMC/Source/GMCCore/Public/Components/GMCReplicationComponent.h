// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Physics.h"
#include "Numerics.h"
#include "FloatPrecision.h"
#include "CircularContainer.h"
#include "SyncInterface.h"
#include "GMCPlayerController.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GMCReplicationComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("UGMC_ReplicationCmp"), STATGROUP_UGMC_ReplicationCmp, STATCAT_Advanced);

USTRUCT(BlueprintType)
struct GMCCORE_API FGMC_Move
{
  GENERATED_BODY()

  FGMC_NetInfo NetInfo{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  FGMC_MetaData MetaData{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  FGMC_PawnState InputState{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  FGMC_PawnState OutputState{};

  bool HasValidTimestamp() const;
  bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
  bool SV_ShouldReplicate();
  bool SV_IsNetRelevantFor(const AActor* const TargetConnection);
  bool SV_BecameNetRelevant(bool bIsNetRelevant, const TGMC_CircularArray<FGMC_Move>& MoveHistory, const AActor* const TargetConnection);

  void NetSerializeClientMove(FArchive& Ar, UPackageMap* Map, const AActor* const TargetConnection);
  void NetSerializeAutonomousProxyState(FArchive& Ar, UPackageMap* Map, const AActor* const TargetConnection);
  void NetSerializeSimulatedProxyState(FArchive& Ar, UPackageMap* Map, const AActor* const TargetConnection);
  void CopyDataValuesTo(FGMC_Move& Other, UGMC_ReplicationCmp* const Component) const;
  void CopyLastSerializedValuesTo(FGMC_Move& Other, UGMC_ReplicationCmp* const Component) const;
  void CopySuspendedFlagsTo(FGMC_Move& Other, UGMC_ReplicationCmp* const Component) const;
  void CopyMetaDataTo(FGMC_Move& Other, UGMC_ReplicationCmp* const Component) const;

  static void SerializeLinearVelocity(FVector& LinearVelocity, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData);
  static void SerializeAngularVelocity(FVector& AngularVelocity, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData);
  static void SerializeActorLocation(FVector& ActorLocation, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData);
  static void SerializeActorRotation(FRotator& ActorRotation, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData);
  static void SerializeActorScale(FVector& ActorScale, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData);
  static void SerializeControlRotation(FRotator& ControlRotation, FArchive& Ar, const FGMC_NetInfo& NetInfo, const FGMC_MetaData& MetaData);
};

template<>
struct TStructOpsTypeTraits<FGMC_Move> : public TStructOpsTypeTraitsBase2<FGMC_Move>
{
  enum { WithNetSerializer = true };
};

/// Synchronises the transform, velocity and any user-defined data for server and client pawns across the network.
UCLASS(ClassGroup = "Movement", HideCategories = ("Velocity", "Hidden"), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GMCCORE_API UGMC_ReplicationCmp : public UPawnMovementComponent
{
  GENERATED_BODY()

  friend class AGMC_PlayerController;
  friend class AGMC_Pawn;

  friend FGMC_Move;
  friend FGMC_ServerAuthPhysicsSettings;
  friend FGMC_ClientAuthPhysicsSettings;

public:

  UGMC_ReplicationCmp(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  void InitializeComponent() override;
  void SetUpdatedComponent(class USceneComponent* NewUpdatedComponent);
  void BeginPlay() override;
  void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
  void EndPlay(EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITORONLY_DATA && WITH_EDITOR

  UPROPERTY(VisibleAnywhere, Category = "Hidden")
  bool bHideProperties{false};

  void HideProperty(UClass* Class, const FName& PropertyName);

#endif

///=============================================================================================================================================================
/// Subclass interface

protected:

  /// Bind any generic member variables that should be replicated here by using the binding-function for the respective type (e.g. BindBool, BindInt, etc.). All
  /// bindings must be fully set up before any net serialization happens and bindings must always match across server and client, meaning you should not bind
  /// data anywhere else but within this function. Once bound, variables cannot be unbound.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void BindReplicationData();
  virtual void BindReplicationData_Implementation() {}

  /// Replicated movement tick meant to be overridden by derived classes. This is supposed to contain the logic for moving the pawn and calculating a new
  /// velocity every frame. The implementation must update the Velocity member of the parent movement component and apply all desired transformations to the
  /// updated component. Keep the following in mind when implementing your movement:
  ///
  /// 1) For server-authoritative movement: try to carry over as little data as possible between prediction ticks. If it cannot be avoided, the variable that
  /// holds the persistent data must be bound and replicated. Example: We have a bool bIsGrounded that tells us whether the pawn is currently on the ground or
  /// not. We could determine the value of bIsGrounded at the beginning of every tick from the pawn's location by sweeping downwards, in which case we would not
  /// have to replicate bIsGrounded because the value will not depend on any (non-replicated) information from the previous tick (as long as you do not do any
  /// operations that involve bIsGrounded before the sweep). If you are still unsure what data needs to be replicated a good strategy is to use local variables
  /// whenever possible. Chances are the remaining data members that cannot be local variables have to be bound. Keep in mind that binding usually increases net
  /// traffic.
  ///
  /// 2) For Blueprint: latent nodes (delays, timelines, etc.) or any of the built-in nodes that save their state (flip-flop, do once, etc.) cannot be used. You
  /// will have to implement your own versions of them that bind the appropriate variables to carry over their state to the next tick.
  ///
  /// @param        InputState              The input for the move.
  /// @param        DeltaTime               The delta time of the move (not necessarily the same as the current frame delta time).
  /// @param        Timestamp               The timestamp of the move.
  /// @param        Iteration               The iteration we are currently executing. Can be between 1 and MaxIterations (inclusive).
  /// @param        bSubSteppedIteration    Whether this is a sub-stepped iteration (i.e. not the final one for the current move).
  /// @param        bLocal                  Whether the prediction tick was called for a locally controlled pawn.
  /// @param        bSimulated              Whether the prediction tick was called for simulation only.
  /// @param        bCombined               Whether a combined client move is being executed. Only relevant for the client (always false on the server).
  /// @returns      void
  virtual void PredictionTick(
    const FGMC_PawnState& InputState,
    float DeltaTime,
    double Timestamp,
    int32 Iteration,
    bool bSubSteppedIteration,
    bool bLocal,
    bool bSimulated,
    bool bCombined
  ) {}

  /// Tick function to update simulated pawns manually on the local machine. Simulated pawns are pawns that do not run the full prediction logic locally for
  /// each frame (e.g. simulated proxies). They just receive discrete states at a certain frequency from the machine that actually calculated the movement (the
  /// server) and usually use some form of interpolation strategy to display smooth visuals. The simulation tick may also be called for remotely controlled
  /// server pawns when running a listen server. It is intended for visual updates only (animations, effects, etc.) and should not be used for anything that can
  /// cause gameplay-critical side effects. This function will never be called on a dedicated server. Animations that are tied to bound members will usually
  /// play automatically without implementing any additional logic within the simulation tick (provided they are replicated) as the animation blueprint will
  /// pick up on them.
  ///
  /// @param        DeltaTime     The current frame delta time.
  /// @param        SimTime       The current simulation time (i.e. the time at which the pawn is being displayed). Can be -1 (meaning no smoothing was applied
  ///                             this frame).
  /// @param        TargetIdx     The index of the target move in the move history. Can be -1 (meaning no smoothing was applied this frame).
  /// @param        SkippedIdx    All indices (if any) of states that have been skipped since the last simulation tick (newest to oldest). Only available if
  ///                             bDetermineSkippedSmoothingStates is set to true.
  /// @returns      void
  virtual void SimulationTick(float DeltaTime, double SimTime, int32 TargetIdx, const TArray<int32>& SkippedIdx) {}

  /// Called after the regular movement execution has completed. Can be used for (predicted) non-movement related tasks that should not be affected by client
  /// replays or sub-stepping.
  ///
  /// @param        Move                              The previously executed move.
  /// @param        DeltaTime                         The delta time of the move (not necessarily the same as the current frame delta time).
  /// @param        Timestamp                         The timestamp of the move.
  /// @param        bLocalMove                        Whether the passed move is a local move.
  /// @param        bCombinedClientMove               Whether the passed move is a combined client move (i.e. a move that was not newly enqueued this frame).
  ///                                                 Always false on the server.
  /// @param        bOutDoNotCombineNextClientMove    Whether the next client move should be prevented from being combined with the passed one. Only relevant
  ///                                                 for autonomous proxies.
  /// @returns      void
  virtual void AncillaryTick(
    const FGMC_Move& Move,
    float DeltaTime,
    double Timestamp,
    bool bLocalMove,
    bool bCombinedClientMove,
    bool& bOutDoNotCombineNextClientMove
  ) {}

  /// Called at the beginning of the world tick.
  ///
  /// @param        DeltaTime    The local frame delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void WorldTickStart(float DeltaTime);
  virtual void WorldTickStart_Implementation(float DeltaTime) {}

  /// Called at the end of the world tick.
  ///
  /// @param        DeltaTime     The local frame delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void WorldTickEnd(float DeltaTime);
  virtual void WorldTickEnd_Implementation(float DeltaTime) {}

  /// Overridable validation function for data received from the client. The return value is passed to the built-in RPC validation function and will cause a
  /// disconnect for the client if false is returned. Timestamp verification will still run afterwards if enabled and is completely independent of the built-in
  /// validation.
  ///
  /// @param        RemoteMoves    The remote moves received from the client.
  /// @returns      bool           If true, the remote moves are considered valid. If false, the received moves are considered bad/invalid and the client will
  ///                              be disconnected from the server.
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component")
  bool SV_ValidateRemoteMoves(const TArray<FGMC_Move>& RemoteMoves);
  virtual bool SV_ValidateRemoteMoves_Implementation(const TArray<FGMC_Move>& RemoteMoves) { return true; }

  /// Overridable function that gets called on the server every time the client fails the timestamp verification. Allows the user to handle the situation in an
  /// application specific way (e.g. disconnecting the client). Only called if bVerifyClientTimestamps is true.
  ///
  /// @param        NumInfractions    How many times the client has failed timestamp verification within the current verification period.
  /// @returns      bool              True to mark the client as credible and execute the remote moves regardless of the failed verification, false to mark the
  ///                                 client as not credible (moves won't be executed). The client's status will be cleared automatically when the next
  ///                                 verification period starts.
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component")
  bool SV_HandleConspicuousClient(int32 NumInfractions);
  virtual bool SV_HandleConspicuousClient_Implementation(int32 NumInfractions) { return false; }

  /// Overridable function that gets called before a client move is executed on the server.
  ///
  /// @param        RemoteMove    The client move about to be executed on the server.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_PreRemoteMoveExecution(const FGMC_Move& RemoteMove);
  virtual void SV_PreRemoteMoveExecution_Implementation(const FGMC_Move& RemoteMove) {}

  /// Overridable function that gets called after a client move was executed on the server.
  ///
  /// @param        RemoteMove    The client move that was just executed on the server.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_PostRemoteMoveExecution(const FGMC_Move& RemoteMove);
  virtual void SV_PostRemoteMoveExecution_Implementation(const FGMC_Move& RemoteMove) {}

  /// Overridable function that gets called before any client moves are executed on the server. Can be used to buffer the server world state if you plan to
  /// change it for remote move execution. This is only called once per client move batch.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_PreRemoteMovesProcessing();
  virtual void SV_PreRemoteMovesProcessing_Implementation() {}

  /// Overridable function that gets called after all client moves in a batch were executed on the server. Can be used to undo changes that were made to the
  /// server world for move execution.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component", DisplayName = "SV On Remote Moves Processed")
  void SV_OnRemoteMovesProcessed();
  virtual void SV_OnRemoteMovesProcessed_Implementation() {}

  /// Overridable function that gets called after a proxy move was created on the server. Can be used to add custom initialization to the move.
  ///
  /// @param        Move         The created proxy move.
  /// @param        DeltaTime    The delta time of the proxy move.
  /// @param        Timestamp    The timestamp of the proxy move.
  /// @returns      bool          True to indicate success, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component", DisplayName = "SV On Proxy Move Initialized")
  bool SV_OnProxyMoveInitialized(UPARAM(Ref) FGMC_Move& Move, float DeltaTime, double Timestamp);
  virtual bool SV_OnProxyMoveInitialized_Implementation(UPARAM(Ref) FGMC_Move& Move, float DeltaTime, double Timestamp) { return true; }

  /// Overridable function that gets called after a move was created for cumulative simulation during smoothing. Can be used to add custom initialization to the
  /// input state. Data that is replicated for simulation will be applied automatically afterwards.
  ///
  /// @param        InputState    The input state for the cumulative move.
  /// @param        SimStates     Whether input or output states are being used for simulation.
  /// @param        DeltaTime     The simulation delta time.
  /// @param        Timestamp     The simulation time.
  /// @returns      bool          True to indicate success, false otherwise.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  bool OnCumulativeMoveInitialized(
    UPARAM(Ref) FGMC_PawnState& InputState,
    EGMC_InterpolationStates SimStates,
    float DeltaTime,
    double Timestamp
  );
  virtual bool OnCumulativeMoveInitialized_Implementation(
    UPARAM(Ref) FGMC_PawnState& InputState,
    EGMC_InterpolationStates SimStates,
    float DeltaTime,
    double Timestamp
  ) { return true; }

  /// Called whenever a new move is added that will be used to simulate a pawn locally.
  ///
  /// @param        Move    The enqueued move.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnSimulationMoveEnqueued(FGMC_Move& Move);
  virtual void OnSimulationMoveEnqueued_Implementation(FGMC_Move& Move) {}

  /// Overridable function that gets called before a move is executed for simulation.
  ///
  /// @param        InputState           The input state of the move about to be executed for simulation.
  /// @param        bCumulativeUpdate    Whether this is going to be a cumulative update.
  /// @param        DeltaTime            The simulation delta time.
  /// @param        Timestamp            The simulation time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PreSimulatedMoveExecution(const FGMC_PawnState& InputState, bool bCumulativeUpdate, float DeltaTime, double Timestamp);
  virtual void PreSimulatedMoveExecution_Implementation(const FGMC_PawnState& InputState, bool bCumulativeUpdate, float DeltaTime, double Timestamp) {}

  /// Overridable function that gets called after a move was executed for simulation.
  ///
  /// @param        OutputState          The output state of the move that was just executed for simulation.
  /// @param        bCumulativeUpdate    Whether this was a cumulative update.
  /// @param        DeltaTime            The simulation delta time.
  /// @param        Timestamp            The simulation time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PostSimulatedMoveExecution(const FGMC_PawnState& OutputState, bool bCumulativeUpdate, float DeltaTime, double Timestamp);
  virtual void PostSimulatedMoveExecution_Implementation(const FGMC_PawnState& OutputState, bool bCumulativeUpdate, float DeltaTime, double Timestamp) {}

  /// Called when the AP move was saved on the server.
  ///
  /// @param        APMove    The saved AP move.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_OnAPMoveSaved(UPARAM(Ref) FGMC_Move& APMove);
  virtual void SV_OnAPMoveSaved_Implementation(UPARAM(Ref) FGMC_Move& APMove) {}

  /// Called when the SP move was saved on the server.
  ///
  /// @param        SPMove    The saved SP move.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_OnSPMoveSaved(UPARAM(Ref) FGMC_Move& SPMove);
  virtual void SV_OnSPMoveSaved_Implementation(UPARAM(Ref) FGMC_Move& SPMove) {}

  /// Called whenever a new AP move was received.
  ///
  /// @param        Move    The received AP move.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component", DisplayName = "CL On AP Move Received")
  void CL_OnAPMoveReceived(UPARAM(Ref) FGMC_Move& APMove);
  virtual void CL_OnAPMoveReceived_Implementation(UPARAM(Ref) FGMC_Move& APMove) {}

  /// Called whenever a new SP move was received.
  ///
  /// @param        Move    The received SP move.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component", DisplayName = "CL On SP Move Received")
  void CL_OnSPMoveReceived(UPARAM(Ref) FGMC_Move& SPMove);
  virtual void CL_OnSPMoveReceived_Implementation(UPARAM(Ref) FGMC_Move& SPMove) {}

  /// Overridable function that gets called before a move is executed during a client replay.
  ///
  /// @param        ReplayMove    The move about to be replayed.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void CL_PreReplayMoveExecution(const FGMC_Move& ReplayMove);
  virtual void CL_PreReplayMoveExecution_Implementation(const FGMC_Move& ReplayMove) {}

  /// Overridable function that gets called after a move was executed during a client replay.
  ///
  /// @param        ReplayMove    The move that was just replayed.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void CL_PostReplayMoveExecution(const FGMC_Move& ReplayMove);
  virtual void CL_PostReplayMoveExecution_Implementation(const FGMC_Move& ReplayMove) {}

  /// Overridable function that gets called before any client moves are replayed. Can be used to buffer the client world state if you plan to change it for the
  /// replay. This is only called once per replay.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void CL_PreReplay();
  virtual void CL_PreReplay_Implementation() {}

  /// Overridable function that gets called at the end of a client replay after all moves were executed. Can be used to undo changes that were made to the
  /// client world for the replay.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void CL_PostReplay();
  virtual void CL_PostReplay_Implementation();

  /// Determines whether the client is allowed to perform the replay when the validation of server data failed.
  ///
  /// @param        DeviatingSyncType         The deviating sync type that triggered the replay. This is only the first deviating type found, there may be more.
  /// @param        DeviatingSyncTypeIndex    The index of the deviating value if it is a generic sync type (-1 otherwise).
  /// @param        DeviatingClientState      The validated state containing the deviating data.
  /// @param        ServerState               The server state that the client state was validated against.
  /// @returns      bool                      True if the client is allowed to perform the replay, false otherwise.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  bool CL_IsAllowedToReplay(
    EGMC_SyncType DeviatingSyncType,
    int32 DeviatingSyncTypeIndex,
    const FGMC_PawnState& DeviatingState,
    const FGMC_PawnState& ServerState
  );
  virtual bool CL_IsAllowedToReplay_Implementation(
    EGMC_SyncType DeviatingSyncType,
    int32 DeviatingSyncTypeIndex,
    const FGMC_PawnState& DeviatingClientState,
    const FGMC_PawnState& ServerState
  ) { return true; }

  /// Called after a smoothed client correction has finished.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component", DisplayName = "CL On Smooth Correction Completed")
  void CL_OnSmoothCorrectionCompleted();
  virtual void CL_OnSmoothCorrectionCompleted_Implementation() {}

  /// Called for locally controlled server and client pawns before the input for local move execution is saved.
  ///
  /// @param        LocalMove    The current local move.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PreLocalMoveExecution(const FGMC_Move& LocalMove);
  virtual void PreLocalMoveExecution_Implementation(const FGMC_Move& LocalMove);

  /// Called for locally controlled server and client pawns after the output for local move execution was saved.
  ///
  /// @param        ExecutedMove    The move that was just executed locally.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PostLocalMoveExecution(const FGMC_Move& ExecutedMove);
  virtual void PostLocalMoveExecution_Implementation(const FGMC_Move& ExecutedMove);

  /// Called for locally controlled server and client pawns before the player input is processed by the player controller.
  ///
  /// @param        DeltaTime    The local frame delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PrePlayerTick(float DeltaTime);
  virtual void PrePlayerTick_Implementation(float DeltaTime);

  /// Called for locally controlled server and client pawns after the player input was processed by the player controller.
  ///
  /// @param        DeltaTime    The local frame delta time.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PostPlayerTick(float DeltaTime);
  virtual void PostPlayerTick_Implementation(float DeltaTime);

  /// Called before the internal smoothing function for a simulated pawn is executed.
  ///
  /// @param        TargetIndex       The smoothing target index in the move history.
  /// @param        bExtrapolating    Whether we are going to extrapolate.
  /// @param        bRollback         Whether we are currently in the context of a rollback.
  /// @param        InterpStates      The type of interpolation states that are going to be used.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void PreSmoothing(int32 TargetIndex, bool bExtrapolating, bool bRollback, EGMC_InterpolationStates InterpStates);
  virtual void PreSmoothing_Implementation(int32 TargetIndex, bool bExtrapolating, bool bRollback, EGMC_InterpolationStates InterpStates) {}

  /// Called each time the pawn was set to a different state internally. If you have bound variables that have gameplay-critical side effects when their value
  /// changes you should put the logic for that here.
  ///
  /// @param        State      The state that was applied.
  /// @param        Context    The context in which the state was applied.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnSyncDataApplied(const FGMC_PawnState& State, EGMC_NetContext Context);
  virtual void OnSyncDataApplied_Implementation(const FGMC_PawnState& State, EGMC_NetContext Context) {}

  /// Called when the pawn was rolled back to a previous state for the move execution of another pawn. Only supported for delay-based interpolation modes.
  ///
  /// @param        SimTime      The time to which the pawn was rolled back.
  /// @param        StartIdx     The start index of the interpolation.
  /// @param        TargetIdx    The target index of the interpolation.
  /// @param        Context      The context in which the function was called.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnPawnRolledBack(double SimTime, int32 StartIdx, int32 TargetIdx, EGMC_NetContext Context);
  virtual void OnPawnRolledBack_Implementation(double SimTime, int32 StartIdx, int32 TargetIdx, EGMC_NetContext Context) {}

  /// Called when the original state of a pawn was restored after it was rolled back to a previous state for the move execution of another pawn.
  ///
  /// @param        Context    The context in which the function was called.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnPawnRestoredAfterRollback(EGMC_NetContext Context);
  virtual void OnPawnRestoredAfterRollback_Implementation(EGMC_NetContext Context) {}

  /// Called when the pawn was set back to a previous state by a call to SV_RewindPawn. Only supported for delay-based interpolation modes.
  ///
  /// @param        SimTime      The time to which the pawn was rewound.
  /// @param        StartIdx     The start index of the interpolation.
  /// @param        TargetIdx    The target index of the interpolation.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component", DisplayName = "SV On Pawn Rewound")
  void SV_OnPawnRewound(double SimTime, int32 StartIdx, int32 TargetIdx);
  virtual void SV_OnPawnRewound_Implementation(double SimTime, int32 StartIdx, int32 TargetIdx) {}

  /// Called when the original state of the pawn was restored (after it was rewound) by a call to SV_RestorePawnAfterRewind.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "General Movement Component", DisplayName = "SV On Pawn Restored After Rewind")
  void SV_OnPawnRestoredAfterRewind();
  virtual void SV_OnPawnRestoredAfterRewind_Implementation() {}

  /// Called when server-authoritative physics simulation is enabled/disabled. The actual physics simulation will happen only on the server, the client just
  /// displays the replicated transform. This function is called from SV_EnableServerAuthPhysicsSimulation but only when the physics simulation setting has
  /// actually changed. On the client, this will enable/disable prediction. Make sure the parameters for interpolation are configured appropriately beforehand
  /// for the network conditions you target.
  ///
  /// @param        bEnabled    True when server-side physics simulation has been enabled, false when it has been disabled.
  /// @param        Settings    The settings to use while physics simulation is enabled.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnServerAuthPhysicsSimulationToggled(bool bEnabled, FGMC_ServerAuthPhysicsSettings Settings);
  virtual void OnServerAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ServerAuthPhysicsSettings Settings);

  /// Called when client-authoritative physics simulation is enabled/disabled. The actual physics simulation will happen only on the client, the server just
  /// displays the received transform. This function is called from SV_EnableClientAuthPhysicsSimulation but only when the physics simulation setting has
  /// actually changed. Make sure the parameters for interpolation are configured appropriately beforehand for the network conditions you target.
  ///
  /// @param        bEnabled    True when client-side physics simulation has been enabled, false when it has been disabled.
  /// @param        Settings    The settings to use while physics simulation is enabled.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnClientAuthPhysicsSimulationToggled(bool bEnabled, FGMC_ClientAuthPhysicsSettings Settings);
  virtual void OnClientAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ClientAuthPhysicsSettings Settings);

  /// Allows for setting up custom input bindings.
  ///
  /// @param        PlayerInputComponent    The configured input component.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent);
  virtual void SetupPlayerInputComponent_Implementation(UInputComponent* PlayerInputComponent) {}

  /// Called when client-side prediction is enabled for this pawn after being disabled.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnClientPredictionEnabled();
  virtual void OnClientPredictionEnabled_Implementation() {}

  /// Called when client-side prediction is disabled for this pawn after being enabled.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnClientPredictionDisabled();
  virtual void OnClientPredictionDisabled_Implementation() {}

  /// Called on client and server when unreliable client move transmission is toggled on. This event is triggered by the client.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnUnreliableClientMovesActivated();
  virtual void OnUnreliableClientMovesActivated_Implementation() {}

  /// Called on client and server when unreliable client move transmission is toggled off. This event is triggered by the client.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnUnreliableClientMovesDeactivated();
  virtual void OnUnreliableClientMovesDeactivated_Implementation() {}

  /// Called when the GMC is enabled after being disabled.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnGMCEnabled();
  virtual void OnGMCEnabled_Implementation() {}

  /// Called when the GMC is disabled after being enabled.
  ///
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void OnGMCDisabled();
  virtual void OnGMCDisabled_Implementation() {}

///=============================================================================================================================================================
/// Public interface

public:

  /// Returns the current server world time. When called on a client, this is the synced time with the network delay already accounted for.
  ///
  /// @returns      double    The time in seconds since the server world was brought up for play.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual double GetTime() const;

  /// Get a reference to the owning pawn.
  ///
  /// @returns      AGMC_Pawn*    Reference to the owning pawn.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component", DisplayName = "Get GMC Pawn Owner")
  AGMC_Pawn* GetGMCPawnOwner() const;

  /// Checks if we are the autonomous proxy on a client.
  ///
  /// @returns      bool    True if our net role is ROLE_AutonomousProxy, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsAutonomousProxy() const;

  /// Checks if we are a simulated proxy on a client.
  ///
  /// @returns      bool    True if our net role is ROLE_SimulatedProxy, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsSimulatedProxy() const;

  /// Checks if we are a simulated proxy on a client that is controlled by a player (either another client or a listen server player).
  ///
  /// @returns      bool    True if our net role is ROLE_SimulatedProxy and we are owned by another player, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsPlayerControlledSimulatedProxy() const;

  /// Checks if we are a simulated proxy on a client that is not controlled by a player elsewhere.
  ///
  /// @returns      bool    True if our net role is ROLE_SimulatedProxy and we are not owned by another player, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsNonPlayerControlledSimulatedProxy() const;

  /// Check if we are the autonomous or a simulated proxy on a client.
  ///
  /// @returns      bool    True if this pawn is an autonomous or simulated proxy, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsClientPawn() const;

  /// Check if we are the autonomous proxy on a client and using client prediction.
  ///
  /// @returns      bool    True if this pawn is an autonomous proxy using client prediction, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsPredictedAutonomousProxy() const;

  /// Check if we are the autonomous proxy on a client and not using client prediction.
  ///
  /// @returns      bool    True if this pawn is an autonomous proxy that is not using client prediction, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsNonPredictedAutonomousProxy() const;

  /// Check if we are a pawn on the server with authority.
  ///
  /// @returns      bool    True if this pawn has authority, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsServerPawn() const;

  /// Checks if we are a locally controlled pawn on a server.
  ///
  /// @returns      bool    True if we are a locally controlled pawn on a server, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsLocallyControlledServerPawn() const;

  /// Checks if we are a remotely controlled pawn on a server.
  ///
  /// @returns      bool    True if we are a pawn on a server that is controlled by a client, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsRemotelyControlledServerPawn() const;

  /// Checks if we are a client-owned server pawn that is using prediction.
  ///
  /// @returns      bool    True if we are a pawn on a server that is controlled by a client and is using prediction, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsRemotelyControlledPredictedServerPawn() const;

  /// Checks if we are a client-owned server pawn that is not using prediction.
  ///
  /// @returns      bool    True if we are a pawn on a server that is controlled by a client and is not using prediction, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsRemotelyControlledNonPredictedServerPawn() const;

  /// Checks if we are a locally controlled pawn on a dedicated server.
  ///
  /// @returns      bool    True if we are a locally controlled pawn on a dedicated server, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsLocallyControlledDedicatedServerPawn() const;

  /// Checks if we are a remotely controlled pawn on a dedicated server.
  ///
  /// @returns      bool    True if we are a pawn on a dedicated server that is controlled by a client, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsRemotelyControlledDedicatedServerPawn() const;

  /// Checks if we are a locally controlled pawn on a listen server.
  ///
  /// @returns      bool    True if we are a locally controlled pawn on a listen server, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsLocallyControlledListenServerPawn() const;

  /// Checks if we are a remotely controlled pawn on a listen server.
  ///
  /// @returns      bool    True if we are a pawn on a listen server that is controlled by a client, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsRemotelyControlledListenServerPawn() const;

  /// Checks if we are a remotely controlled pawn on a listen server and if the option to smooth those pawns is enabled. Whether the pawn is considered smoothed
  /// only depends on the value of bSmoothRemoteServerPawn.
  ///
  /// @returns      bool    True if we are a smoothed remotely controlled pawn on a listen server, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsSmoothedListenServerPawn() const;

  /// Checks if we are controlled by a player (either another client or a listen server player).
  ///
  /// @returns      bool    True if we are controlled by a player, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsPlayerControlledPawn() const;

  /// Checks if we are a simulated pawn. Simulated pawns are all pawns that are subject to being smoothed at some point on the current machine. These can be
  /// simulated proxies, smoothed listen server pawns and non-predicted autonomous proxies.
  ///
  /// @returns      bool    True if we are a simulated pawn, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsSimulatedPawn() const;

  /// Check if we are on a network server (i.e. not a client and not running in standalone).
  ///
  /// @returns      bool    True if we are a server in a networked context, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsNetworkedServer() const;

  /// Tells us whether we are currently executing a move.
  ///
  /// @returns      bool    True if we are currently executing a move in any context, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsExecutingMove() const;

  /// Tells us whether we are currently executing a local move.
  ///
  /// @returns      bool    True if we are currently executing a local move, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsExecutingLocalMove() const;

  /// Tells us whether we are currently executing a move in a non-simulated context.
  ///
  /// @returns      bool    True if we are currently executing a move in a non-simulated context, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsExecutingNonSimulatedMove() const;

  /// Tells us whether we are currently executing a move in a simulated context.
  ///
  /// @returns      bool    True if we are currently executing a move in a simulated context, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsExecutingSimulatedMove() const;

  /// Changes a server pawn's state from smoothed to non-smoothed and vice versa. Only relevant for smoothed listen server pawns, this function does nothing for
  /// any other type of pawn. Call this function to switch to the non-smoothed state, then execute the code that changes the pawn's state, and afterwards call
  /// this function again to switch back to the smoothed state. Do not forget this second call, the function must always wrap your own logic.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_SwapServerState();

  /// Calling this function will unwind a pawn to its original state during remote move execution on the server. Only call this function in the context of
  /// server rollback and do not forget to call it a second time when you are done with your logic to rewind the pawn again (this is always necessary).
  /// Unwinding can be expensive, prefer manipulating the rollback buffer directly if possible or defer logic until pawns are restored.
  ///
  /// @param        Pawn    The pawn to unwind.
  /// @returns      bool    True if the swap was successful, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintPure = false, Category = "General Movement Component")
  bool SV_SwapRollbackState(AGMC_Pawn* Pawn) const;

  /// Tells us whether the pawn is currently being smoothed as part of a client correction.
  ///
  /// @returns      bool    True if the pawn is currently being smoothed for a client correction, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  bool CL_IsPerformingSmoothCorrection() const;

  /// Returns the time that has passed since the client last sent a move batch to the server. Only applicable for autonomous proxies, all other kinds of pawns
  /// will return 0.
  ///
  /// @returns      float    The time in seconds since the client last sent a move batch to the server.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double CL_GetTimeSinceLastMoveBatchWasSent() const;

  /// Returns the average time discrepancy of the client that was calculated during the last time sync. If the value is greater/smaller than 0 the client is
  /// estimated to be behind/ahead.
  ///
  /// @returns      double    The estimated signed average time discrepancy of the client (in seconds). Always returns 0 on the server.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual double CL_GetTimeDiscrepancy() const;

  /// Returns a move from the pawn's move history based on the passed timestamp. If no move with the exact passed timestamp exists, the move with the closest
  /// matching timestamp will be returned.
  ///
  /// @param        Timestamp    The timestamp of the move to find.
  /// @returns      FGMC_Move    A copy of the move from the move history that most closely matches the passed timestamp, or a default constructed move with
  ///                            timestamp -1 if no appropriate move could be found.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FGMC_Move FindMove(double Timestamp) const;

  /// Version of FindMove that only returns the index of the desired move in the move history. Keep in mind that the returned index will become outdated as soon
  /// as a new move is enqueued so it should be used immediately and then discarded.
  ///
  /// @param        Timestamp    The timestamp of the move to find.
  /// @returns      int32        The index of the move from the move history that most closely matches the passed timestamp, or -1 if no appropriate move could
  ///                            be found.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  int32 FindMoveByIndex(double Timestamp) const;

  /// Computes rewind parameters according to the passed arguments. This function may be used instead of SV_RewindPawn if you don't want to actually rewind the
  /// pawn but just look at the rewind data numerically. Any relevant preconditions stated in the description of SV_RewindPawn also apply to this function.
  ///
  /// @param        Connection      The client connection to target.
  /// @param        Time            The time to get the rewind parameters for (usually the client timestamp).
  /// @param        Pawn            The pawn to get the rewind parameters for.
  /// @param        OutStartIdx     The start index of the interpolation in the passed pawn's move history.
  /// @param        OutTargetIdx    The target index of the interpolation in the passed pawn's move history.
  /// @param        OutAlpha        The interpolation ratio between the start and target states.
  /// @returns      bool            True if the rewind parameters are valid, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintPure = false, BlueprintAuthorityOnly, Category = "General Movement Component")
  bool SV_GetRewindParams(
    APlayerController* Connection,
    double Time,
    AGMC_Pawn* Pawn,
    int32& OutStartIdx,
    int32& OutTargetIdx,
    float& OutAlpha
  ) const;

  /// Sets the passed server pawn back to the state it had on the client at the passed time. There are a few important things to consider when using this
  /// function:
  /// 1) Only works when called on the server.
  /// 2) The passed connection must be a client controller.
  /// 3) The passed connection must not be the owner of the passed pawn (i.e. the autonomous proxy connection for that pawn).
  /// 4) The passed pawn needs to have a replication component as well.
  /// 5) Uses the server-side smoothing settings of the passed pawn for interpolation (which must be the same settings the client uses).
  /// 6) Delay-based interpolation yields the best results, other smoothing modes are potentially a lot less accurate.
  /// 7) Only values that are replicated by default (location, rotation, etc.) and bound variables will be rewound.
  /// 8) Always call SV_RestorePawnAfterRewind when you are done with your logic to set the pawn back to its original state.
  /// 9) Only call this function on pawns that are not already rolled back.
  ///
  /// @param        Connection         The client connection to target.
  /// @param        Time               The time to set the pawn back to (usually the client timestamp).
  /// @param        Pawn               The pawn to rewind.
  /// @param        bSkipBoneUpdate    Whether to skip updating kinematic bones when the pawn has a skeletal mesh component with a physics state.
  /// @returns      bool               True if the pawn was successfully set back to the passed time, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  bool SV_RewindPawn(APlayerController* Connection, double Time, AGMC_Pawn* Pawn, bool bSkipBoneUpdate = true);

  /// Sets the passed server pawn back to the state it had before SV_RewindPawn was called on it.
  ///
  /// @param        Pawn               The pawn that should be restored.
  /// @param        bSkipBoneUpdate    Whether to skip updating kinematic bones when the pawn has a skeletal mesh component with a physics state.
  /// @returns      bool               True if the pawn's state was successfully restored, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  bool SV_RestorePawnAfterRewind(AGMC_Pawn* Pawn, bool bSkipBoneUpdate = true);

  /// Tells us whether the passed pawn is currently rolled back to a previous point in time.
  ///
  /// @param        Pawn    The pawn to check.
  /// @returns      bool    Whether the passed pawn is currently in a rolled back state.
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  bool SV_IsRewound(AGMC_Pawn* Pawn) const;

  /// Tells us whether we are currently replaying moves from the move history on a client. Always returns false on the server and for simulated proxies.
  ///
  /// @returns      bool    True if we are currently executing a client replay, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool CL_IsReplaying() const;

  /// Returns the history index of the move that is current being replayed (only valid during a client replay).
  ///
  /// @returns      int32    The index of the move that is currently being replayed (-1 if no replay is currently being performed).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  int32 CL_GetCurrentReplayMoveIndex() const;

  /// Returns the move that is current being replayed. This function must not be called outside of a client replay.
  ///
  /// @returns      const FGMC_Move&    The move that is currently being replayed.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  const FGMC_Move& CL_GetCurrentReplayMove() const;

  /// Tells us whether we are currently executing remote moves from a client on the server. Always returns false on a client.
  ///
  /// @returns      bool    True if we are currently executing client moves on the server, false otherwise.
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  bool SV_IsExecutingRemoteMoves() const;

  /// Returns the last client move that was executed without any server processing applied (meaning it only contains the raw data received from the client).
  ///
  /// @returns      const FGMC_Move&    The raw client data of the last executed remote move.
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  const FGMC_Move& SV_GetLastClientData() const;

  /// Tells us whether we are currently in the process of smoothing a simulated pawn. Simulation includes the internal portion of the smoothing logic as well
  /// as the simulation tick.
  ///
  /// @returns      bool    True if we are currently simulating the pawn, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsSimulating() const;

  /// Returns the most recent interpolation target state timestamp. Will be -1 if no interpolation happened during the last smoothing update.
  ///
  /// @returns      double    The latest interpolation target time.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double GetLastInterpTargetTime() const;

  /// Returns the most recent extrapolation target state timestamp. Will be -1 if no extrapolation happened during the last smoothing update.
  ///
  /// @returns      double    The latest extrapolation target time.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double GetLastExtrapTargetTime() const;

  /// Tells us whether the most recent smoothing iteration was a cumulative update. Only relevant for cumulative simulation, always returns false if any other
  /// type of smoothing is used.
  ///
  /// @returns      bool    True if the most recent smoothing iteration was a cumulative update, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsCumulativeSmoothingUpdate() const;

  /// Tells us whether we are currently extrapolating the state of a simulated pawn. The return value is only valid for the current frame after the internal
  /// smoothing code has run.
  ///
  /// @returns      bool    True if we are currently extrapolating the state of the pawn, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsExtrapolating() const;

  /// Tells us whether we have reached the max allowed extrapolation distance for a simulated pawn.
  ///
  /// @returns      bool    True if the max extrapolation distance was reached, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool HasReachedMaxExtrapolationDistance() const;

  /// Tells us whether we were extrapolating the state of a simulated pawn during the last update. The return value is only valid for the current frame after
  /// the component tick has begun.
  ///
  /// @returns      bool    True if we were extrapolating the state of the pawn last frame, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool WasExtrapolatingLastUpdate() const;

  /// Can be used to manually trigger a recovery to the next smoothing state. Only available available for delay based interpolation modes.
  ///
  /// @param        bInWorldSpace    Whether the recovery should be performed with absolute or relative values.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void TriggerExtrapolationRecovery(bool bInWorldSpace);

  /// Tells us if an extrapolation recovery is currently in progress.
  ///
  /// @returns      bool    True if the pawn is currently performing a recovery, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsPerformingExtrapolationRecovery() const;

  /// Sets a component to smooth after simulation. Pass nullptr to reset the smoothing parameters. The passed component must be a direct child of the actor's
  /// root component.
  ///
  /// @param        Component    The component to smooth. Must have the same outer actor as the replication component and must be directly attached to the
  ///                            actor's root component.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetComponentToSmooth(USceneComponent* Component);

  /// Returns the component that is being smoothed after simulation.
  ///
  /// @returns      USceneComponent*    The component to smooth.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  USceneComponent* GetComponentToSmooth() const;

  /// Returns the saved base offset of the component that is being smoothed after simulation.
  ///
  /// @returns      FTransform    The offset of the component to smooth. Returns an identity transform if none is set.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FTransform GetComponentToSmoothOffset() const;

  /// Sets the base offset transform of the smoothed component in addition to applying the transform. The offset will only be set if the passed component is the
  /// component that is being smoothed and the owner is a simulated pawn, but the transform will be applied for any valid component passed.
  ///
  /// @param        ComponentWorldTransform    The component for which the transform should be set.
  /// @param        bSweep                     Whether to sweep the component.
  /// @param        OutSweepHitResult          The hit result of the sweep.
  /// @param        bTeleportPhysics           Whether the physics state should be teleported.
  /// @param        SmoothComponent            The component that is being smoothed.
  /// @param        SmoothOffset               The new offset of the smoothed component (relative to the owning actor's root component).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetComponentWorldTransformWithSmoothOffset(
    const FTransform& ComponentWorldTransform,
    bool bSweep,
    FHitResult& OutSweepHitResult,
    bool bTeleportPhysics,
    USceneComponent* SmoothComponent,
    const FTransform& SmoothOffset
  );

  /// Sets the base location offset of the smoothed component in addition to applying the location. The offset will only be set if the passed component is the
  /// component that is being smoothed and the owner is a simulated pawn, but the location will be applied for any valid component passed.
  ///
  /// @param        ComponentWorldLocation    The component for which the location should be set.
  /// @param        bSweep                    Whether to sweep the component.
  /// @param        OutSweepHitResult         The hit result of the sweep.
  /// @param        bTeleportPhysics          Whether the physics state should be teleported.
  /// @param        SmoothComponent           The component that is being smoothed.
  /// @param        SmoothOffset              The new offset of the smoothed component (relative to the owning actor's root component).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetComponentWorldLocationWithSmoothOffset(
    const FVector& ComponentWorldLocation,
    bool bSweep,
    FHitResult& OutSweepHitResult,
    bool bTeleportPhysics,
    USceneComponent* SmoothComponent,
    const FVector& SmoothOffset
  );

  /// Sets the base rotation offset of the smoothed component in addition to applying the rotation. The offset will only be set if the passed component is the
  /// component that is being smoothed and the owner is a simulated pawn, but the rotation will be applied for any valid component passed.
  ///
  /// @param        ComponentWorldRotation    The component for which the rotation should be set.
  /// @param        bTeleportPhysics          Whether the physics state should be teleported.
  /// @param        SmoothComponent           The component that is being smoothed.
  /// @param        SmoothOffset              The new offset of the smoothed component (relative to the owning actor's root component).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetComponentWorldRotationWithSmoothOffset(
    const FRotator& ComponentWorldRotation,
    bool bTeleportPhysics,
    USceneComponent* SmoothComponent,
    const FRotator& SmoothOffset
  );

  /// Sets the base scale offset of the smoothed component in addition to applying the scale. The offset will only be set if the passed component is the
  /// component that is being smoothed and the owner is a simulated pawn, but the scale will be applied for any valid component passed.
  ///
  /// @param        ComponentWorldScale    The component for which the scale should be set.
  /// @param        SmoothComponent        The component that is being smoothed.
  /// @param        SmoothOffset           The new offset of the smoothed component (relative to the owning actor's root component).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetComponentWorldScaleWithSmoothOffset(const FVector& ComponentWorldScale, USceneComponent* SmoothComponent, const FVector& SmoothOffset);

  /// Calling this function will force the next client move to be enqueued as a new entry in the move history. Only relevant for autonomous proxies, this
  /// function does nothing for any other kind of pawn.
  ///
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void CL_DoNotCombineNextMove();

  /// Access a move in the move history state at the given (valid) index. Read only.
  ///
  /// @param        Index               The index at which to access the move history. Must be a valid index so check beforehand.
  /// @returns      const FGMC_Move&    The move at the specified index.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  const FGMC_Move& AccessMoveHistory(int32 Index) const;

  /// Checks if the passed index corresponds to a valid move within the move history.
  ///
  /// @param        Index    The index to check.
  /// @returns      bool     True if the passed index maps to a valid move within the move history, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsValidMoveHistoryIndex(int32 Index) const;

  /// Returns the current number of entries in the move history.
  ///
  /// @returns      int32    The current size of the move history.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  int32 GetCurrentMoveHistoryNum() const;

  /// Returns the max number of entries the move history can hold.
  ///
  /// @returns      int32    The max size of the move history.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  int32 GetMoveHistoryMaxSize() const;

  /// Returns the current number of pending client moves that are about to be sent to the server.
  ///
  /// @returns      int32    The number of moves waiting to be sent to the server.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  int32 CL_GetNumPendingClientMoves() const;

  /// Returns the replicated control rotation in the local space of the pawn. Also returns a valid control rotation if called on a simulated proxy (which have
  /// no controller).
  ///
  /// @returns      FRotator    The current control rotation in local space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FRotator GetControlRotationLocalSpace() const;

  /// Returns the replicated control rotation in world space. Also returns a valid control rotation if called on a simulated proxy (which have no controller).
  ///
  /// @returns      FRotator    The current control rotation in world space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FRotator GetControlRotationWorldSpace() const;

  /// Gets the current pawn state saved to the return value.
  ///
  /// @param        bUseRelative      Whether the pawn state should be relative to the current base.
  /// @returns      FGMC_PawnState    The current pawn state.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FGMC_PawnState GetPawnState(bool bUseRelative) const;

  /// Sets the pawn to the passed state. OnSyncDataApplied will be called with no context information.
  ///
  /// @param        State    The state to apply.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void ApplyPawnState(const FGMC_PawnState& State);

  /// Allows for accessing the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        State      The state to read from.
  /// @returns      FVector    The linear velocity saved in the passed state.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetLinearVelocityFromState(const FGMC_PawnState& State) const;

  /// Allows for accessing the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        State      The state to read from.
  /// @returns      FVector    The angular velocity saved in the passed state.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetAngularVelocityFromState(const FGMC_PawnState& State) const;

  /// Allows for accessing the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        State      The state to read from.
  /// @returns      FVector    The actor location saved in the passed state.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetActorLocationFromState(const FGMC_PawnState& State) const;

  /// Allows for accessing the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        State       The state to read from.
  /// @returns      FRotator    The actor rotation saved in the passed state.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FRotator GetActorRotationFromState(const FGMC_PawnState& State) const;

  /// Allows for accessing the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        State      The state to read from.
  /// @returns      FVector    The actor scale saved in the passed state.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetActorScaleFromState(const FGMC_PawnState& State) const;

  /// Allows for accessing the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        State       The state to read from.
  /// @returns      FRotator    The control rotation saved in the passed state.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FRotator GetControlRotationFromState(const FGMC_PawnState& State) const;

  /// Allows for setting the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        Value    The linear velocity to set in the passed state.
  /// @param        State    The state to write to.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetLinearVelocityInState(const FVector& Value, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Allows for setting the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        Value    The angular velocity to set in the passed state.
  /// @param        State    The state to write to.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetAngularVelocityInState(const FVector& Value, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Allows for setting the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        Value    The actor location to set in the passed state.
  /// @param        State    The state to write to.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetActorLocationInState(const FVector& Value, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Allows for setting the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        Value    The actor rotation to set in the passed state.
  /// @param        State    The state to write to.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetActorRotationInState(const FRotator& Value, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Allows for setting the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        Value    The actor scale to set in the passed state.
  /// @param        State    The state to write to.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetActorScaleInState(const FVector& Value, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Allows for setting the integrated sync data of a pawn state from Blueprint.
  ///
  /// @param        Value    The control rotation to set in the passed state.
  /// @param        State    The state to write to.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetControlRotationInState(const FRotator& Value, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Suspend the linear velocity from replication to the target machine.
  ///
  /// @param        bSuspend    Whether to suspend the value from replication.
  /// @param        State       The state to suspend the value for (must be a state of the local, AP or SP move).
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetLinearVelocitySuspended(bool bSuspend, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Suspend the angular velocity from replication to the target machine.
  ///
  /// @param        bSuspend    Whether to suspend the value from replication.
  /// @param        State       The state to suspend the value for (must be a state of the local, AP or SP move).
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetAngularVelocitySuspended(bool bSuspend, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Suspend the actor location from replication to the target machine.
  ///
  /// @param        bSuspend    Whether to suspend the value from replication.
  /// @param        State       The state to suspend the value for (must be a state of the local, AP or SP move).
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetActorLocationSuspended(bool bSuspend, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Suspend the actor rotation from replication to the target machine.
  ///
  /// @param        bSuspend    Whether to suspend the value from replication.
  /// @param        State       The state to suspend the value for (must be a state of the local, AP or SP move).
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetActorRotationSuspended(bool bSuspend, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Suspend the actor scale from replication to the target machine.
  ///
  /// @param        bSuspend    Whether to suspend the value from replication.
  /// @param        State       The state to suspend the value for (must be a state of the local, AP or SP move).
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetActorScaleSuspended(bool bSuspend, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Suspend the control rotation from replication to the target machine.
  ///
  /// @param        bSuspend    Whether to suspend the value from replication.
  /// @param        State       The state to suspend the value for (must be a state of the local, AP or SP move).
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetControlRotationSuspended(bool bSuspend, UPARAM(Ref) FGMC_PawnState& State) const;

  /// Sets the linear velocity of the owning pawn relative to the current base.
  ///
  /// @param        RelativeLinearVelocity    The relative linear velocity to set.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetBasedLinearVelocity(const FVector& RelativeLinearVelocity);

  /// Returns the passed relative linear velocity in world space based on the passed transform.
  ///
  /// @param        InRelativeLinearVelocity    The relative value to transform.
  /// @param        InBaseTransform             The transform to use as base.
  /// @returns      FVector                     The passed value in world space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetWorldLinearVelocityFor(const FVector& InRelativeLinearVelocity, const FTransform& InBaseTransform) const;

  /// Returns the linear velocity of the owning pawn relative to the current base.
  ///
  /// @returns      FVector    The linear velocity relative to the current base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetBasedLinearVelocity() const;

  /// Returns the passed linear velocity value relative to the passed transform.
  ///
  /// @param        InLinearVelocity    The value to transform.
  /// @param        InBaseTransform     The transform to use as base.
  /// @returns      FVector             The passed value relative to the passed base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetBasedLinearVelocityFor(const FVector& InLinearVelocity, const FTransform& InBaseTransform) const;

  /// Sets the angular velocity of the owning pawn relative to the current base.
  ///
  /// @param        RelativeAngularVelocity    The relative angular velocity to set.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetBasedAngularVelocity(const FVector& RelativeAngularVelocity);

  /// Returns the passed relative angular velocity in world space based on the passed transform.
  ///
  /// @param        InRelativeAngularVelocity    The relative value to transform.
  /// @param        InBaseTransform              The transform to use as base.
  /// @returns      FVector                      The passed value in world space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetWorldAngularVelocityFor(const FVector& InRelativeAngularVelocity, const FTransform& InBaseTransform) const;

  /// Returns the angular velocity of the owning pawn relative to the current base.
  ///
  /// @returns      FVector    The angular velocity relative to the passed base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetBasedAngularVelocity() const;

  /// Returns the passed angular velocity value relative to the passed transform.
  ///
  /// @param        InAngularVelocity    The value to transform.
  /// @param        InBaseTransform      The transform to use as base.
  /// @returns      FVector              The passed value relative to the passed base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetBasedAngularVelocityFor(const FVector& InAngularVelocity, const FTransform& InBaseTransform) const;

  /// Sets the location of the owning pawn relative to the current base.
  ///
  /// @param        RelativeActorLocation    The relative actor location to set.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetBasedActorLocation(const FVector& RelativeActorLocation);

  /// Returns the passed relative actor location in world space based on the passed transform.
  ///
  /// @param        InRelativeActorLocation    The relative value to transform.
  /// @param        InBaseTransform            The transform to use as base.
  /// @returns      FVector                    The passed value in world space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetWorldActorLocationFor(const FVector& InRelativeActorLocation, const FTransform& InBaseTransform) const;

  /// Returns the actor location of the owning pawn relative to the current base.
  ///
  /// @returns      FVector    The actor location relative to the current base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetBasedActorLocation() const;

  /// Returns the passed actor location value relative to the passed transform.
  ///
  /// @param        InActorLocation    The value to transform.
  /// @param        InBaseTransform    The transform to use as base.
  /// @returns      FVector            The passed value relative to the passed base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetBasedActorLocationFor(const FVector& InActorLocation, const FTransform& InBaseTransform) const;

  /// Sets the actor rotation of the owning pawn relative to the current base.
  ///
  /// @param        RelativeActorRotation    The relative actor rotation to set.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetBasedActorRotation(const FRotator& RelativeActorRotation);

  /// Returns the passed relative actor rotation in world space based on the passed transform.
  ///
  /// @param        InRelativeActorRotation    The relative value to transform.
  /// @param        InBaseTransform            The transform to use as base.
  /// @returns      FRotator                   The passed value in world space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FRotator GetWorldActorRotationFor(const FRotator& InRelativeActorRotation, const FTransform& InBaseTransform) const;

  /// Returns the actor rotation of the owning pawn relative to the current base.
  ///
  /// @returns      FRotator    The normalized actor rotation relative to the current base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FRotator GetBasedActorRotation() const;

  /// Returns the passed actor rotation value relative to the passed transform.
  ///
  /// @param        InActorRotation    The value to transform.
  /// @param        InBaseTransform    The transform to use as base.
  /// @returns      FRotator           The passed value relative to the passed base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FRotator GetBasedActorRotationFor(const FRotator& InActorRotation, const FTransform& InBaseTransform) const;

  /// Sets the actor scale of the owning pawn relative to the current base.
  ///
  /// @param        RelativeActorScale    The relative actor scale to set.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetBasedActorScale(const FVector& RelativeActorScale);

  /// Returns the passed relative actor scale in world space based on the passed transform.
  ///
  /// @param        InRelativeActorScale    The relative value to transform.
  /// @param        InBaseTransform         The transform to use as base.
  /// @returns      FVector                 The passed value in world space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetWorldActorScaleFor(const FVector& InRelativeActorScale, const FTransform& InBaseTransform) const;

  /// Returns the actor scale of the owning pawn relative to the current base.
  ///
  /// @returns      FVector    The actor scale relative to the current base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetBasedActorScale() const;

  /// Returns the passed actor scale value relative to the passed transform.
  ///
  /// @param        InActorScale       The value to transform.
  /// @param        InBaseTransform    The transform to use as base.
  /// @returns      FVector            The passed value relative to the passed base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FVector GetBasedActorScaleFor(const FVector& InActorScale, const FTransform& InBaseTransform) const;

  /// Sets the control rotation of the owning pawn relative to the current base.
  ///
  /// @param        RelativeControlRotation    The relative control rotation to set.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetBasedControlRotation(const FRotator& RelativeControlRotation);

  /// Returns the passed relative control rotation in world space based on the passed transform.
  ///
  /// @param        InRelativeControlRotation    The relative value to transform.
  /// @param        InBaseTransform              The transform to use as base.
  /// @returns      FRotator                     The passed value in world space.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FRotator GetWorldControlRotationFor(const FRotator& InRelativeControlRotation, const FTransform& InBaseTransform) const;

  /// Returns the control rotation of the owning pawn relative to the current base.
  ///
  /// @returns      FRotator    The control rotation relative to the current base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FRotator GetBasedControlRotation() const;

  /// Returns the passed control rotation value relative to the passed transform.
  ///
  /// @param        InControlRotation    The value to transform.
  /// @param        InBaseTransform      The transform to use as base.
  /// @returns      FRotator             The passed value relative to the passed base.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual FRotator GetBasedControlRotationFor(const FRotator& InControlRotation, const FTransform& InBaseTransform) const;

  /// Set a base component for the owning pawn. Setting the base to nullptr will use absolute coordinates.
  ///
  /// @param        Base    The component to base the owning pawn on.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void SetActorBase(USceneComponent* Base);

  /// Returns the component the owning pawn is currently based on.
  ///
  /// @returns      Base    The component the owning pawn is currently based on.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual USceneComponent* GetActorBase() const;

  /// Check if we are a server pawn that is locally controlled by AI.
  ///
  /// @returns      bool    True if this pawn is a bot on the server, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsServerBot() const;

  /// Utility function to enables/disable server-authoritative physics simulation of the pawn's root component while also handling the required networking
  /// related configuration. The client will be informed of a state change via RPC (reliable) and OnServerAuthPhysicsSimulationToggled will be called for both
  /// the server and the client. Keep the following in mind when using this function:
  ///
  /// 1) Only works when called on the server.
  /// 2) This function will do nothing at all if the internal activation state has not changed, for example when called with bEnable as true when physics
  /// simulation was already enabled via this function before.
  /// 3) Client movement will NOT be predicted while server-authoritative physics simulation is enabled.
  /// 4) Using this function will apply the passed settings when bEnable is true and will revert to the original settings when bEnable is false.
  /// 5) The transition is not seamless, depending on the situation and network conditions the client may experience a noticeable teleport or stutter when the
  /// physics simulation is toggled.
  /// 6) The client state can desync if the RPC cannot be processed properly by the client at the time it is received.
  /// 7) Only call this function externally, not from within the movement logic.
  /// 8) Do not mix the functions for enabling/disabling server-authoritative and client-authoritative physics replication, and do not toggle any of the related
  /// settings individually while the physics simulation is active.
  /// 9) Do not enable or disable physics simulation manually, only use this function to toggle it in a networked context. You can spawn the pawn with physics
  /// simulation already enabled but then you must still activate server-authoritative replication by calling this function after spawning to get the right
  /// configuration.
  ///
  /// @param        bEnable     True to enable server-side physics simulation, false to disable it.
  /// @param        Settings    The settings to use for physics simulation.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_EnableServerAuthPhysicsSimulation(bool bEnable, FGMC_ServerAuthPhysicsSettings Settings);

  /// Local version of SV_EnableServerAuthPhysicsSimulation to enable/disable server-authoritative physics simulation on the client and the server independently
  /// (i.e. without sending a client RPC). Usually this means that the function needs to be triggered by some event that is guaranteed to run on the server as
  /// well as the client around the same time. It is your responsibility to ensure that the pawn state stays in sync when using this function (keep in mind that
  /// during some early events like BeginPlay the remote role may not be assigned yet on the server). Does not need to be called for simulated proxies.
  ///
  /// @param        bEnable     True to enable server-side physics simulation, false to disable it.
  /// @param        Settings    The settings to use for physics simulation.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void EnableServerAuthPhysicsSimulationLocal(bool bEnable, FGMC_ServerAuthPhysicsSettings Settings);

  /// Utility function to enables/disable client-authoritative physics simulation of the pawn's root component while also handling the required networking
  /// related configuration. The client will be informed of a state change via RPC (reliable) and OnClientAuthPhysicsSimulationToggled will be called for both
  /// the server and the client. Keep the following in mind when using this function:
  ///
  /// 1) Only works when called on the server.
  /// 2) This function will do nothing at all if the internal activation state has not changed, for example when called with bEnable as true when physics
  /// simulation was already enabled via this function before.
  /// 3) All synchronised values that are part of the physics state should be configured to be client-authoritative output (linear velocity, angular velocity,
  /// actor location, actor rotation). Other values can be still be server-authoritative and will be replayed if necessary.
  /// 4) Using this function will apply the passed settings when bEnable is true and will revert to the original settings when bEnable is false.
  /// 5) The transition is not seamless, depending on the situation and network conditions the client may experience a noticeable teleport or stutter when the
  /// physics simulation is toggled.
  /// 6) The client state can desync if the RPC cannot be processed properly by the client at the time it is received.
  /// 7) Only call this function externally, not from within the movement logic.
  /// 8) Do not mix the functions for enabling/disabling server-authoritative and client-authoritative physics replication, and do not toggle any of the related
  /// settings individually while the physics simulation is active.
  /// 9) Do not enable or disable physics simulation manually, only use this function to toggle it in a networked context. You can spawn the pawn with physics
  /// simulation already enabled but then you must still activate client-authoritative replication by calling this function after spawning to get the right
  /// configuration.
  ///
  /// @param        bEnable     True to enable client-side physics simulation, false to disable it.
  /// @param        Settings    The settings to use for physics simulation.
  /// @returns      void
  UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "General Movement Component")
  void SV_EnableClientAuthPhysicsSimulation(bool bEnable, FGMC_ClientAuthPhysicsSettings Settings);

  /// Local version of SV_EnableClientAuthPhysicsSimulation to enable/disable client-authoritative physics simulation on the client and the server independently
  /// (i.e. without sending a client RPC). Usually this means that the function needs to be triggered by some event that is guaranteed to run on the server as
  /// well as the client around the same time. It is your responsibility to ensure that the pawn state stays in sync when using this function (keep in mind that
  /// during some early events like BeginPlay the remote role may not be assigned yet on the server). Does not need to be called for simulated proxies.
  ///
  /// @param        bEnable     True to enable client-side physics simulation, false to disable it.
  /// @param        Settings    The settings to use for physics simulation.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void EnableClientAuthPhysicsSimulationLocal(bool bEnable, FGMC_ClientAuthPhysicsSettings Settings);

  /// Whether this pawn currently uses server-authoritative physics replication.
  ///
  /// @returns      bool    True if server-authoritative physics replication is currently active, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsUsingServerAuthPhysicsReplication() const;

  /// Whether this pawn currently uses client-authoritative physics replication.
  ///
  /// @returns      bool    True if client-authoritative physics replication is currently active, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsUsingClientAuthPhysicsReplication() const;

  /// Whether this pawn is using client-side prediction.
  ///
  /// @returns      bool    True if this pawn is using client-side prediction, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsUsingClientPrediction() const;

  /// Allows for enabling/disabling client side prediction locally. Should only be called externally, not from within the component itself. Toggling prediction
  /// is not seamless.
  ///
  /// @param        bEnable    True to enable client-side prediction, false to disable it.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void EnableClientPrediction(bool bEnable);

  /// Whether we are currently using unreliable RPCs to transmit client moves to the server. This flag is synced across client and server.
  ///
  /// @returns      bool    True if client moves are sent to the server via unreliable RPCs, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsUsingUnreliableClientMoves() const;

  /// Whether the GMC system is currently enabled.
  ///
  /// @returns      bool    True if the system is currently enabled, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool IsGMCEnabled() const;

  /// Allows for enabling/disabling of the GMC system locally. Should only be called externally, not from within the component itself. Disabling will stop
  /// almost all operations of the component (some minor functionality may still continue to run in the background).
  ///
  /// @param        bEnable    True to enable the system, false to disable it.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void EnableGMC(bool bEnable);

  /// Tells us how much time has passed since this pawn has begun play. This returns the real time (not paused, not dilated).
  ///
  /// @returns      double    Time since BeginPlay was called on this pawn.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double GetElapsedTimeSinceSpawn() const;

  /// Sets the pawn's linear world velocity.
  ///
  /// @param        InLinearVelocity    The linear velocity value to set (in cm/sec).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetVelocity(const FVector& InLinearVelocity);

  /// Returns pawn's linear world velocity.
  ///
  /// @returns      FVector    The current linear velocity of the pawn (in cm/sec).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetVelocity() const;

  /// Sets the pawn's linear world velocity.
  ///
  /// @param        InLinearVelocity    The linear velocity value to set (in cm/sec).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetLinearVelocity_GMC(const FVector& InLinearVelocity);

  /// Returns pawn's linear world velocity.
  ///
  /// @returns      FVector    The current linear velocity of the pawn (in cm/sec).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetLinearVelocity_GMC() const;

  /// Sets pawn's angular world velocity.
  ///
  /// @param        InAngularVelocity    The angular velocity value to set (in rad/sec).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetAngularVelocity_GMC(const FVector& InAngularVelocity);

  /// Returns the pawn's angular world velocity.
  ///
  /// @returns      FVector    The current angular velocity of the pawn (in rad/sec).
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetAngularVelocity_GMC() const;

  /// Sets the pawn's world location.
  ///
  /// @param        Location            The location value to set.
  /// @param        bTeleportPhysics    Whether to teleport the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetActorLocation_GMC(const FVector& Location, bool bTeleportPhysics);

  /// Returns the pawn's world location.
  ///
  /// @returns      FVector    The current actor location.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetActorLocation_GMC() const;

  /// Sets the pawn's world rotation.
  ///
  /// @param        Rotation            The rotation value to set.
  /// @param        bTeleportPhysics    Whether to teleport the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetActorRotation_GMC(const FRotator& Rotation, bool bTeleportPhysics);

  /// Returns the pawn's world rotation.
  ///
  /// @returns      FRotator    The current actor rotation.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FRotator GetActorRotation_GMC() const;

  /// Sets the pawn's world rotation.
  ///
  /// @param        Quat                The rotation value to set.
  /// @param        bTeleportPhysics    Whether to teleport the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetActorQuat_GMC(const FQuat& Quat, bool bTeleportPhysics);

  /// Returns the pawn's world rotation.
  ///
  /// @returns      FQuat    The current actor rotation.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FQuat GetActorQuat_GMC() const;

  /// Sets the pawn's world location and rotation.
  ///
  /// @param        Location            The location value to set.
  /// @param        Rotation            The rotation value to set.
  /// @param        bTeleportPhysics    Whether to teleport the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetActorLocationAndRotation_GMC(const FVector& Location, const FRotator& Rotation, bool bTeleportPhysics);

  /// Sets the pawn's world location and rotation.
  ///
  /// @param        Location            The location value to set.
  /// @param        Rotation            The rotation value to set.
  /// @param        bTeleportPhysics    Whether to teleport the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetActorLocationAndQuat_GMC(const FVector& Location, const FQuat& Rotation, bool bTeleportPhysics);

  /// Sets the pawn's world scale.
  ///
  /// @param        Scale    The scale value to set.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetActorScale_GMC(const FVector& Scale);

  /// Returns the pawn's world scale.
  ///
  /// @returns      FVector    The current actor scale.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetActorScale_GMC() const;

  /// Sets the pawn's world transform.
  ///
  /// @param        Transform           The transform to set.
  /// @param        bTeleportPhysics    Whether to teleport the physics state.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetActorTransform_GMC(const FTransform& Transform, bool bTeleportPhysics);

  /// Returns the pawn's world transform.
  ///
  /// @returns      FTransform    The current actor transform.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FTransform GetActorTransform_GMC() const;

  /// Sets the control rotation of the pawn's controller in world space, or the simulated control rotation if the pawn has no controller.
  ///
  /// @param        Rotation    The control rotation value to set.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  void SetControllerRotation_GMC(const FRotator& Rotation);

  /// Returns the control rotation of the pawn's controller in world space, or the simulated control rotation if the pawn has no controller.
  ///
  /// @returns      FRotator    The current control rotation.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FRotator GetControllerRotation_GMC() const;

  /// Returns the tangential velocity at the passed world location based on the pawn's location and angular velocity.
  ///
  /// @param        WorldLocation    The world location for which to calculate the tangential velocity.
  /// @returns      FVector          The tangential velocity at the passed location exerted by the pawn.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetTangentialVelocity_GMC(const FVector& WorldLocation);

///=============================================================================================================================================================
/// Common

public:

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  UPARAM(Ref) FGMC_Move& LocalMove();
  const FGMC_Move& LocalMove() const;

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  UPARAM(Ref) FGMC_Move& APMove();
  const FGMC_Move& APMove() const;

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  UPARAM(Ref) FGMC_Move& SPMove();
  const FGMC_Move& SPMove() const;

private:

  /// Contains setup logic that runs when a networked pawn is spawned.
  ///
  /// @returns      void
  void BeginPlayNetworked();

  /// Contains any setup logic that must run before the actual tick.
  ///
  /// @returns      void
  void PreTick();

  /// Whether we should run the GMC logic for this frame.
  ///
  /// @returns      bool    True if the pawn should be updated through the GMC this tick, false otherwise.
  bool ShouldRunGMCUpdates() const;

  /// Contains the logic for updating a locally controlled server pawn. This could be the pawn of the local player (standalone or listen server) or an AI
  /// controlled pawn (AI logic only runs on the server).
  ///
  /// @returns      void
  void UpdateLocallyControlledServerPawn();

  /// Contains the logic for updating a remotely controlled server pawn. This is the server version of a pawn that is remotely controlled by a client player.
  ///
  /// @returns      void
  void UpdateRemotelyControlledServerPawn();

  /// Contains the logic for updating the locally controlled client pawn (i.e. the autonomous proxy). This is the pawn that a client player controls on their
  /// machine.
  ///
  /// @returns      void
  void UpdateAutonomousProxyPawn();

  /// Contains the logic for updating a simulated pawn. A simulated pawn does not run the full movement logic locally, it is updated by receiving data about its
  /// state at a certain frequency from the server (where the movement was calculated).
  ///
  /// @returns      void
  void SimulatePawn();

public:

  /// Minimum delta time for calculations to avoid potential divide-by-zero.
  static constexpr float MIN_DELTA_TIME = 1e-6f;

  /// How many reliable RPCs should be reserved for the owning actor at the least when attempting to send client moves to the server.
  static constexpr int32 SEND_CLIENT_MOVES_OVERFLOW_PROTECTION = 30;

  /// The client move send rate to assume for calculations when unreliable RPCs are used (should also consider CLIENT_MAX_MOVE_COMBINE_DELTA_TIME_UNRELIABLE).
  static constexpr int32 STAND_IN_CLIENT_SEND_RATE_UNRELIABLE = 50;

  /// The max move combine delta time to when unreliable RPCs are used.
  static constexpr float CLIENT_MAX_MOVE_COMBINE_DELTA_TIME_UNRELIABLE = 0.025f;

  /// How large the buffer for moves created by non-predicted autonomous proxies should be.
  static constexpr int32 NUM_SAVED_NO_PREDICTION_MOVES = 3;

  /// Absolute clamp value for the client time discrepancy during simulation.
  static constexpr double CLIENT_TIME_DISCREPANCY_CLAMP_ABS = 0.05;

  /// Contains past moves that this pawn has executed. New moves are enqueued at the end (i.e. the most recent move has the highest index).
  TGMC_CircularArray<FGMC_Move> MoveHistory{};

  /// The aliases to bind generic data members to.
  FGMC_MemberAliases AliasData{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The current angular kinematic velocity of the pawn (in radians per second).
  FVector AngularVelocity{0.};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Whether built-in sync types should be synchronised as relative to the current base for predicted clients.
  bool bUseRelativeValuesForPrediction{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// Whether built-in sync types should be synchronised as relative to the current base for simulation.
  bool bUseRelativeValuesForSimulation{false};

  /// Checks if the buffer for reliable RPCs of the passed actor is about to overflow.
  ///
  /// @param        Owner                   The actor to check.
  /// @param        ProtectionMargin        How much capacity should still be allowed within the buffer before it is considered to be about to overflow.
  /// @param        OutNumPendingPackets    The current number of pending reliable packets waiting to be acknowledged.
  /// @returns      bool                    True if there's still more capacity in the buffer than the protection margin specified, false otherwise.
  static bool CheckReliableBuffer(AActor* Owner, int32 ProtectionMargin, int32& OutNumPendingPackets);

  /// Returns all data associated with any sync tags for non-default initialization of sync types.
  ///
  /// @returns      FGMC_SyncTasgData    The settings to use for this component.
  FGMC_SyncTagsData GetSyncTagsData() const;

protected:

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The saved settings to revert to when server-authoritative physics simulation is disabled again.
  FGMC_ServerAuthPhysicsSettings SavedSettingsServerAuthPhysics{};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The saved settings to revert to when client-authoritative physics simulation is disabled again.
  FGMC_ClientAuthPhysicsSettings SavedSettingsClientAuthPhysics{};

  UPROPERTY(BlueprintReadOnly, Category = "General Movement Component")
  /// Cached reference to the GMC aggregator (if present).
  TObjectPtr<class AGMC_Aggregator> GMCAggregator{nullptr};

  /// Binds to FWorldDelegates::OnWorldTickStart.
  ///
  /// @param        World        The current world.
  /// @param        TickType     The level tick type.
  /// @param        DeltaTime    The current delta time.
  /// @returns      void
  virtual void OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime);

  /// Binds to FWorldDelegates::OnWorldTickEnd.
  ///
  /// @param        World        The current world.
  /// @param        TickType     The level tick type.
  /// @param        DeltaTime    The current delta time.
  /// @returns      void
  virtual void OnWorldTickEnd(UWorld* World, ELevelTick TickType, float DeltaTime);

  /// Assures that all actors and components that participate in the movement logic tick in the correct order.
  ///
  /// @returns      void
  virtual void ManagePrerequisiteTicks();

  /// Adjusts the client's buffer time when using an adaptive simulation delay to mitigate adverse network conditions.
  ///
  /// @returns      void
  virtual void ManageDynamicBufferTime();

  /// Sets the linear/angular velocity of the movement component to the linear/angular velocity of the root component if it is simulating physics.
  ///
  /// @returns      void
  void UpdatePhysicsVelocity();

  /// Calculates the (sub-stepped) delta time for the current iteration of a move execution.
  ///
  /// @param        Iterations         The current iteration number.
  /// @param        RemainingTime      The remaining time of the original delta time.
  /// @param        InMaxTimeStep      The max allowed duration of a single time step.
  /// @param        InMaxIterations    The max allowed amount of iterations.
  /// @returns      float              The sub-stepping delta time for this iteration.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  float CalculateSubDeltaTime(int32 Iterations, float RemainingTime, float InMaxTimeStep, int32 InMaxIterations) const;

  /// Returns the current move send rate of the client. This value is not automatically synced across client and server.
  ///
  /// @returns      float    The client move send rate.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetClientSendRate() const;

  /// Returns the current max combined delta time for a client move. This value is not automatically synced across client and server.
  ///
  /// @returns      float    The max combined move delta time.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual float GetMaxCombinedDeltaTime() const;

  /// Clears most transient data on the component. Do not call from within prediction or simulation logic.
  ///
  /// @param        bResetMoves    Whether the pawn moves should also be reset.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void ClearTransientData(bool bResetMoves = true);

  /// Checks if the owner and the component have completed their BeginPlay functions and are ready to be updated.
  ///
  /// @returns      bool    True if the pawn is ready for play, false otherwise.
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual bool IsReadyForPlay() const;

  struct FComponentStatus
  {
    bool bGMCEnabled{true};

    bool bDidGMCUpdateLastFrame{false};

    TWeakObjectPtr<AController> ControllerLastFrame{nullptr};

    ENetRole NetRoleLastFrame{};

    bool bOnWorldTickStartExecuted{false};

    bool bIsUsingServerAuthPhysicsReplication{false};

    bool bIsUsingClientAuthPhysicsReplication{false};

    bool bIsPlayerOwned{false};

    double ElapsedRealTimeSinceSpawn{0.};

    double MoveExecutionTimer{0.};

    float AccumulatedMoveDeltaTime{0.};

    bool bIsExecutingLocalMove{false};

    bool bIsExecutingNonSimulatedMove{false};

    bool bIsExecutingSimulatedMove{false};

    bool bUseUnreliableClientMoves{false};

    bool CL_bJustDeactivatedUnreliableClientMoves{false};

    double CL_UseUnreliableClientMovesActivationTime{0.};

    static constexpr double USE_UNRELIABLE_CLIENT_MOVES_TIMEOUT = 30.;
    static constexpr double UNRELIABLE_CLIENT_MOVES_SPAWN_TOLERANCE = 3.;

    void PreTick(float DeltaTime);

    bool ShouldExecuteMove(float& InOutDeltaTime, const UGMC_ReplicationCmp* const Outer);

    void CL_SetUseUnreliableClientMoves(int32 NumPendingReliablePackets, UGMC_ReplicationCmp* const Outer);

    void SaveFrameInfo(bool bDidUpdate, const UGMC_ReplicationCmp* const Outer);

    bool ShouldClearTransientData(const UGMC_ReplicationCmp* const Outer) const;
  };

  FComponentStatus ComponentStatus{};

private:

  UPROPERTY(Transient, DuplicateTransient)
  TObjectPtr<USceneComponent> ActorBase{nullptr};

  FVector ExecuteMove(
    const FGMC_PawnState& InputState,
    float DeltaTime,
    double Timestamp,
    float InMaxTimestep,
    int32 InMaxIterations,
    bool bUseRelative,
    bool bLocal,
    bool bSimulated,
    bool bCombined
  );

  bool ShouldAddToSimulationHistory(double MoveTimestamp) const;

  void AddToSimulationHistory(const FGMC_Move& Move);

///=============================================================================================================================================================
/// Server

public:

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  UPARAM(Ref) FGMC_PawnState& SV_RemoteServerPawnSmoothingBuffer();
  const FGMC_PawnState& SV_RemoteServerPawnSmoothingBuffer() const;

protected:

  /// Utility type for maintaining two states on the same pawn simultaneously. On first use the buffer must be initialized, afterwards the pawn can be toggled
  /// between two states at any time by calling the swap function.
  struct FSyncDataSwapBuffer
  {
    FGMC_PawnState Buffer{};

    bool bInitialized{false};

    uint64 NumSwaps{0};

    void Initialize(bool bUseRelative, UGMC_ReplicationCmp* const Outer);

    void Initialize(const FGMC_PawnState& InitState, UGMC_ReplicationCmp* const Outer);

    void Swap(bool bUseRelative, UGMC_ReplicationCmp* const Outer, EGMC_NetContext Context = EGMC_NetContext::NoContextInformation);

    bool HasEvenNumberOfSwaps() const;

    void Reset()
    {
      Buffer = FGMC_PawnState{};
      bInitialized = false;
      NumSwaps = 0;
    }
  };

  /// Called when the smoothing buffer for remote server pawns is initialized.
  ///
  /// @param        Buffer    The currently buffered state.
  /// @returns      void
  virtual void SV_OnInitRemoteServerPawnSmoothingBuffer(FGMC_PawnState& Buffer) {}

  /// Called when the smoothing buffer for remote server pawns is swapped.
  ///
  /// @param        Buffer     The currently buffered state.
  /// @param        Context    The context in which the function was called.
  /// @returns      void
  virtual void SV_OnSwapRemoteServerPawnSmoothingBuffer(FGMC_PawnState& Buffer, EGMC_NetContext Context) {}

  /// Verifies the timestamps of moves received from the client. Can be overridden to implement additional or different checks.
  ///
  /// @param        ClientMoves    The moves received from the client.
  /// @returns      bool           True if all the timestamps were valid, false otherwise.
  virtual bool SV_VerifyTimestamps(const TArray<FGMC_Move>& ClientMoves);

  /// Enables custom checks to force a replication update to the client.
  ///
  /// @param        CurrentMove     The client move that was just executed.
  /// @param        PreviousMove    The client move that was executed before the current one.
  /// @param        bValidMove      Whether the current move was valid. Only relevant for predicted clients.
  /// @returns      bool            True if a net update should be forced, false otherwise.
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  bool SV_CheckForcedNetUpdate_Custom(
    const FGMC_Move& CurrentMove,
    const FGMC_Move& PreviousMove,
    bool bValidMove
  ) const;
  virtual bool SV_CheckForcedNetUpdate_Custom_Implementation(
    const FGMC_Move& CurrentMove,
    const FGMC_Move& PreviousMove,
    bool bValidMove
  ) const { return false; }

private:

  FSyncDataSwapBuffer SV_RemoteServerPawnSmoothingSwapBuffer{};

  struct FRemoteMoveExecutionAux
  {
    TArray<FGMC_Move> DeserializedMoves{};

    TArray<FGMC_Move> PendingMoves{};

    FGMC_Move LastRawMove{};

    FVector LastReplicatedLocation{0.};

    double LastReceivedClientTimestamp{0.};

    double LastAcceptedClientTimestamp{0.};

    double LastRemotePawnUpdateTime{0.};

    bool bLastClientMoveWasValid{false};

    bool bIsExecutingRemoteMoves{false};

    bool bIsRolledBack{false};

    void Reset()
    {
      DeserializedMoves.Reset();
      PendingMoves.Reset();
      LastRawMove = FGMC_Move{};
      LastReplicatedLocation = FVector{0.};
      LastReceivedClientTimestamp = 0.;
      LastAcceptedClientTimestamp = 0.;
      LastRemotePawnUpdateTime = 0.;
      bLastClientMoveWasValid = false;
      bIsExecutingRemoteMoves = false;
      bIsRolledBack = false;
    }
  };

  FRemoteMoveExecutionAux SV_RemoteMoveExecutionAux{};

  void SV_PreReplication();

  void SV_ReceiveClientSendStatus_Implementation(bool bIsUsingUnreliableMoves);

  bool SV_ReceiveClientSendStatus_Validate(bool bIsUsingUnreliableMoves);

  void SV_ReceiveMoves_Implementation();

  bool SV_ReceiveMoves_Validate();

  bool SV_AuditClientMoves(TArray<FGMC_Move>& ClientMoves);

  void SV_ApplyValidatedState(FGMC_PawnState& State, bool bInAssumeClientState, bool& bInOutValidState, bool bUseRelative, EGMC_NetContext Context);

  void SV_ExecuteClientMoves(TArray<FGMC_Move>& ClientMoves, bool bIsProxyMove = false);

  bool SV_CheckForcedNetUpdate(const FGMC_Move& Move, bool bValidMove);

  FGMC_Move SV_CreateProxyMove(double Timestamp, float DeltaTime);

  bool SV_ProcessProxyMove();

  struct FNetReserializationAux
  {
    static constexpr float NET_RESERIALIZATION_LEAD_TIME = 3.f;

    // For internal use only, use AGMC_Pawn::SV_MarkForNetReserialization to trigger a net reserialization.
    bool bOwnerNeedsReserialization{false};

    FTimerHandle TimerHandle{};

    void ReserializationTimer_Impl(const UGMC_ReplicationCmp* const Outer) const;

    void SetTimer(UGMC_ReplicationCmp* const Outer);

    void ClearTimer(const UGMC_ReplicationCmp* const Outer);

    void PreReplication(const UGMC_ReplicationCmp* const Outer);

    void PreTick(const UGMC_ReplicationCmp* const Outer);

    void Reset()
    {
      bOwnerNeedsReserialization = false;
    }
  };

  FNetReserializationAux SV_NetReserializationAux{};

  void SV_NetReserializationTimer() const;

  struct FPredictedClientNetSerializationAux
  {
    int32 UpdateFrequency{0};

    double TimeSinceLastUpdate{0.};

    bool ShouldUpdate(bool bValidClientMove) const;

    void ResetTimer();

    void PreTick(const UGMC_ReplicationCmp* const Outer, float DeltaTime);

    void Reset()
    {
      UpdateFrequency = 0;
      TimeSinceLastUpdate = 0.;
    }
  };

  FPredictedClientNetSerializationAux SV_PredictedClientNetSerializationAux{};

  struct FTimestampVerificationAux
  {
    static constexpr bool VERIFY_WORLD_TIME = true;
    static constexpr int32 BUFFER_SIZE = 512;

    TGMC_CircularNumericBuffer<double, BUFFER_SIZE> TimestampBuffer{};

    TGMC_CircularNumericBuffer<double, BUFFER_SIZE> TimeDiffBuffer{};

    int32 InfractionsThisPeriod{0};

    FTimerHandle TimerHandle{};

    bool ComputeAccumulatedClientTime(double& OutClientTime) const;

    void ResetClientCredibility_Impl(const UGMC_ReplicationCmp* const Outer);

    void SetTimer(UGMC_ReplicationCmp* const Outer);

    void ClearTimer(const UGMC_ReplicationCmp* const Outer);

    void Reset()
    {
      TimestampBuffer.Reset();
      TimeDiffBuffer.Reset();
      InfractionsThisPeriod = 0;
    }
  };

  FTimestampVerificationAux SV_TimestampVerificationAux{};

  void SV_ResetClientCredibility();

///=============================================================================================================================================================
/// Client

public:

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  UPARAM(Ref) FGMC_PawnState& CL_NoPredictionBuffer();
  const FGMC_PawnState& CL_NoPredictionBuffer() const;

protected:

  /// Swaps the no-prediction buffer of a client (if initialized).
  ///
  /// @param        Context    The context in which the buffer is being swapped.
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  virtual void CL_SwapNoPredictionBuffer(EGMC_NetContext Context);

  /// Overridable function for custom checks to determine whether a move should be enqueued as a new move in the move history or combined with the previous one.
  /// This is only called when essential built-in checks have not already made the decision.
  ///
  /// @param        CurrentInputState     The local input state that was created this frame.
  /// @param        PreviousInputState    The last input state that was enqueued.
  /// @returns      int32                 = 0: Continue with the default checks to decide whether the current move should be combined or not.
  ///                                     < 0: Enqueue the current move as a new move.
  ///                                     > 0: Combine the new move with the previous one (advanced).
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  int32 CL_CheckSyncDataShouldCombineMove_Custom(const FGMC_PawnState& CurrentInputState, const FGMC_PawnState& PreviousInputState) const;
  virtual int32 CL_CheckSyncDataShouldCombineMove_Custom_Implementation(
    const FGMC_PawnState& CurrentInputState,
    const FGMC_PawnState& PreviousInputState
  ) const { return 0; }

  /// Overridable function for custom logic to combine moves.
  ///
  /// @param        ExistingInputState     The last move input already enqueued.
  /// @param        InputStateToCombine    The new move input that was combined with the existing one.
  /// @returns      void
  UFUNCTION(BlueprintNativeEvent, Category = "General Movement Component")
  void CL_SetSyncDataCombinedMove_Custom(FGMC_PawnState& ExistingInputState, const FGMC_PawnState& InputStateToCombine);
  virtual void CL_SetSyncDataCombinedMove_Custom_Implementation(FGMC_PawnState& ExistingInputState, const FGMC_PawnState& InputStateToCombine) {}

  /// Called when a server update is received for the autonomous proxy.
  ///
  /// @returns      void
  virtual void CL_OnRepAPMove();

  /// Called when a server state update is received for a simulated proxy.
  ///
  /// @returns      void
  virtual void CL_OnRepSPMove();

  /// RPC to inform the client when server-authoritative physics simulation was toggled on the server. This will also call OnServerAuthPhysicsSimulationToggled.
  ///
  /// @param        bEnabled    True if physics simulation was enabled, false if it was disabled.
  /// @param        Settings    The settings to use while physics simulation is enabled.
  /// @returns      void
  UFUNCTION(Client, Reliable)
  void CL_OnServerAuthPhysicsSimulationToggled(bool bEnabled, FGMC_ServerAuthPhysicsSettings Settings);
  virtual void CL_OnServerAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ServerAuthPhysicsSettings Settings);

  /// RPC to inform the client when client-authoritative physics simulation was toggled on the server. This will also call OnClientAuthPhysicsSimulationToggled.
  ///
  /// @param        bEnabled    True if physics simulation was enabled, false if it was disabled.
  /// @param        Settings    The settings to use while physics simulation is enabled.
  /// @returns      void
  UFUNCTION(Client, Reliable)
  void CL_OnClientAuthPhysicsSimulationToggled(bool bEnabled, FGMC_ClientAuthPhysicsSettings Settings);
  virtual void CL_OnClientAuthPhysicsSimulationToggled_Implementation(bool bEnabled, FGMC_ClientAuthPhysicsSettings Settings);

  /// Called on the client before adopting the server state before a replay.
  ///
  /// @param        APMove        The received autonomous proxy data.
  /// @param        SourceMove    The original source move with the same timestamp as the received server data.
  /// @returns      void
  virtual void CL_PreAdoptStateForReplay(const FGMC_Move& APMove, const FGMC_Move& SourceMove);

  /// Called on the client after adopting the server state before a replay.
  ///
  /// @param        APMove        The received autonomous proxy data.
  /// @param        SourceMove    The original source move with the same timestamp as the received server data.
  /// @returns      void
  virtual void CL_PostAdoptStateForReplay(const FGMC_Move& APMove, const FGMC_Move& SourceMove) {}

  /// Determines if the client should interpolate server corrections locally for smoother visuals.
  ///
  /// @returns      bool    True if the client should use smooth corrections, false otherwise.
  virtual bool CL_ShouldUseSmoothCorrections() const;

  /// Whether to force a deferred camera manager update. By default the update will always be deferred.
  ///
  /// @returns      bool    True if the camera manager update should be deferred to the end of the world tick, false otherwise.
  virtual bool ShouldDeferCameraManagerUpdate() const;

  /// Called when the buffer for non-predicted autonomous proxies is initialized.
  ///
  /// @param        Buffer    The currently buffered state.
  /// @returns      void
  virtual void CL_OnInitNoPredictionBuffer(FGMC_PawnState& Buffer) {}

  /// Called when the buffer for non-predicted autonomous proxies is swapped.
  ///
  /// @param        Buffer     The currently buffered state.
  /// @param        Context    The context in which the function was called.
  /// @returns      void
  virtual void CL_OnSwapNoPredictionBuffer(FGMC_PawnState& Buffer, EGMC_NetContext Context) {}

private:

  struct FClientMoveExecutionAux
  {
    TArray<FGMC_Move> PendingMoves{};

    TArray<FGMC_Move> NonPredictedMoves{};

    FGMC_Move DefaultMove{};

    FGMC_Move TempMove{};

    double TimeSinceLastMoveBatchWasSent{0.};

    double LastReceivedMoveTimestamp{0.};

    double LastValidRepUpdateTime{0.};

    bool bIsReplaying{false};

    int32 ReplayMoveIdx{-1};

    bool bDoNotCombineNextMove{false};

    bool bIsRolledBack{false};

    void PreTick(const UGMC_ReplicationCmp* const Outer, float DeltaTime);

    FGMC_Move& GetCurrentMoveForExecution(UGMC_ReplicationCmp* const Outer);

    FGMC_Move& GetPreviousMoveForExecution(UGMC_ReplicationCmp* const Outer);

    FGMC_Move& GetDefaultMove(const UGMC_ReplicationCmp* const Outer);

    void Reset()
    {
      PendingMoves.Reset();
      NonPredictedMoves.Reset();
      DefaultMove = FGMC_Move{};
      TempMove = FGMC_Move{};
      TimeSinceLastMoveBatchWasSent = 0.;
      LastReceivedMoveTimestamp = 0.;
      LastValidRepUpdateTime = 0.;
      bIsReplaying = false;
      ReplayMoveIdx = -1;
      bDoNotCombineNextMove = false;
      bIsRolledBack = false;
    }
  };

  FClientMoveExecutionAux CL_MoveExecutionAux{};

  FSyncDataSwapBuffer CL_NoPredictionSwapBuffer{};

  struct FSmoothClientCorrection
  {
    struct CorrectionTransform
    {
      FVector Location{0.};
      FRotator Rotation{0.};
      FVector Scale{1.};
      FRotator ControlRotation{0.};
    }
    StartTransform{},
    TargetTransform{},
    PreReplayTransform{},
    PostReplayTransform{};

    struct CorrectionBuffer
    {
      // The buffer for swapping the actual pawn transform with the visually smoothed one.
      CorrectionTransform SwapTransform{};

      // Tracks how far we are into the interpolation during a correction.
      float AccumulatedTime{-1.f};
    }
    Buffer{};

    struct CorrectionStatus
    {
      static constexpr double LOCATION_TOLERANCE = 2.;
      static constexpr double ROTATION_TOLERANCE = 5.;
      static constexpr double SCALE_TOLERANCE = 0.05;
      static constexpr double CONTROL_ROTATION_TOLERANCE = 5.;

      bool bTargetLocationReached{false};
      bool bTargetRotationReached{false};
      bool bTargetScaleReached{false};
      bool bTargetControlRotationReached{false};
    }
    Status{};

    bool HasData() const;

    void AddCorrectionTime(float TimeToAdd);

    void InitCorrection();

    bool HasBegunCorrection();

    bool HasFinishedCorrection(float TotalDuration);

    void SaveTargetTransform(const UGMC_ReplicationCmp* const Outer);

    void SavePreReplayTransform(const UGMC_ReplicationCmp* const Outer);

    void SavePostReplayTransform(const UGMC_ReplicationCmp* const Outer);

    void SetTargetTransformReached(const UGMC_ReplicationCmp* const Outer);

    void SwapTransform(UGMC_ReplicationCmp* const Outer);

    void Interpolate(const UGMC_ReplicationCmp* const Outer, float TotalDuration);

    void Reset()
    {
      StartTransform = TargetTransform = PreReplayTransform = PostReplayTransform = CorrectionTransform{};
      Buffer = CorrectionBuffer{};
      Status = CorrectionStatus{};
    }

  private:

    void SaveTransform(const UGMC_ReplicationCmp* const Outer, CorrectionTransform& Transform) const;
  };

  FSmoothClientCorrection CL_SmoothCorrection{};

  void CL_HandleSmoothCorrectionOnReplayStart();

  void CL_HandleSmoothCorrectionOnReplayEnd();

  void CL_HandleSmoothCorrectionOnWorldTickStart();

  void CL_HandleSmoothCorrectionOnWorldTickEnd(float DeltaTime, ELevelTick TickType);

  bool CL_MaintainPredictionHistory(const FGMC_Move& NewMove, bool& bOutStartedNewMove, bool& bOutPredictionHistoryFull);

  bool CL_MaintainNonPredictedMoves(const FGMC_Move& NewMove, bool& bOutStartedNewMove);

  bool CL_ShouldEnqueueMove(const FGMC_Move& CurrentMove, const FGMC_Move& PreviousMove, bool& bOutCombineMove);

  bool CL_AddToPredictionHistory(const FGMC_Move& NewMove, bool& bOutPredictionHistoryFull);

  FVector CL_ExecuteMove(FGMC_Move& Move, bool bPredicted, bool bStartedNewMove);

  void CL_AncillaryTick(FGMC_Move& Move, bool bLocalMove, bool bCombinedMove);

  bool CL_ShouldSendMoves() const;

  void CL_SendMoves();

  FGMC_Move CL_ClearAcknowledgedMoves(double ReceivedTimestamp);

  bool CL_CheckPhysicsSettingsSynced(const FGMC_Move& ReplicatedMove) const;

  bool CL_ShouldReplay(const FGMC_Move& APMove, const FGMC_Move& SourceMove);

  void CL_ReplayMoves();

///=============================================================================================================================================================
/// Lag compensation

public:

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General Movement Component")
  UPARAM(Ref) FGMC_PawnState& RollbackBuffer();
  const FGMC_PawnState& RollbackBuffer() const;

protected:

  /// Determines whether a pawn should be rolled back for move execution.
  ///
  /// @param        PawnToTest    The pawn to check.
  /// @returns      bool          True if the passed pawn should be rolled back for move execution, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool ShouldRollBackGMCPawn(const AGMC_Pawn* PawnToTest) const;
  virtual bool ShouldRollBackGMCPawn_Implementation(const AGMC_Pawn* PawnToTest) const;

  /// Determines whether a rollback actor should be rolled back for move execution.
  ///
  /// @param        ActorToTest    The actor to check.
  /// @returns      bool           True if the passed actor should be rolled back for move execution, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool ShouldRollBackGMCActor(const class AGMC_RollbackActor* ActorToTest) const;
  virtual bool ShouldRollBackGMCActor_Implementation(const AGMC_RollbackActor* ActorToTest) const;

  /// Determines whether a rollback actor should be ticked from the replication component of the local player.
  ///
  /// @param        RollbackActor    The actor to check.
  /// @returns      bool             True if the passed actor should be ticked from the replication component of the local player, false otherwise.
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  bool ShouldTickGMCActor(const class AGMC_RollbackActor* RollbackActor) const;
  virtual bool ShouldTickGMCActor_Implementation(const AGMC_RollbackActor* RollbackActor) const;

private:

  struct FRollbackRestoreBuffer
  {
    FGMC_PawnState Buffer{};

    bool bInitialized{false};

    void Initialize(UGMC_ReplicationCmp* const Outer);

    void Save(bool bUseRelative, UGMC_ReplicationCmp* const Outer);

    void Restore(bool bUseRelative, UGMC_ReplicationCmp* const Outer, EGMC_NetContext Context);

    void Reset()
    {
      Buffer = FGMC_PawnState{};
      bInitialized = false;
    }
  };

  FRollbackRestoreBuffer RollbackRestoreBuffer{};

  FSyncDataSwapBuffer RollbackSwapBuffer{};

  TArray<AGMC_Pawn*> GatherRollbackPawns() const;

  void SaveLocalStateBeforeRollback(const APawn* PawnToSave, bool bUseRelative) const;

  void RollBackPawns(double Time, const TArray<AGMC_Pawn*>& PawnsToRollBack, EGMC_NetContext Context) const;

  double ComputeRollbackSimulationTime(
    double Time,
    APlayerController* const Connection,
    AGMC_Pawn* const OtherPawn,
    UGMC_ReplicationCmp* const OtherReplicationComponent
  ) const;

  bool ComputeRollbackParams(
    APlayerController* Connection,
    double Time,
    const TGMC_CircularArray<FGMC_Move>& MoveHistoryToSearch,
    int32& OutStartIdx,
    int32& OutTargetIdx,
    float& OutAlpha
  ) const;

  void SetRollbackState(
    double Time,
    double SimulationTime,
    int32 StartIdx,
    int32 TargetIdx,
    float Alpha,
    UGMC_ReplicationCmp* const OtherReplicationComponent,
    EGMC_NetContext Context
  ) const;

  void RestoreRolledBackPawns(const TArray<AGMC_Pawn*>& PawnsToRestore, EGMC_NetContext Context) const;

  void RestoreOriginalStateAfterRollback(const APawn* PawnToRestore, bool bUseRelative, EGMC_NetContext Context) const;

  TArray<AGMC_RollbackActor*> GatherGenericRollbackActors() const;

  void TickGenericRollbackActors(double Time, float DeltaTime, const FGMC_Move& Move, EGMC_NetContext Context) const;

  void BufferGenericRollbackActorStates(const TArray<AGMC_RollbackActor*>& RollbackActors, EGMC_NetContext Context) const;

  void RollbackGenericActors(
    double Time,
    float DeltaTime,
    const FGMC_Move& Move,
    const TArray<AGMC_RollbackActor*>& ActorsToRollBack,
    EGMC_NetContext Context
  ) const;

  void RestoreRolledBackGenericActors(const TArray<AGMC_RollbackActor*>& RollbackActors, EGMC_NetContext Context) const;

///=============================================================================================================================================================
/// Smoothing

public:

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  /// The replicated control rotation in world space for pawns without a controller (simulated proxies).
  FRotator SimulatedControlRotation{0.};

  /// Used to generically interpolate a numeric value.
  ///
  /// @param        Function             The function to use for interpolation.
  /// @param        StartValue           The start value.
  /// @param        TargetValue          The target value.
  /// @param        PreviousValue        The value before the start value.
  /// @param        NextValue            The value after the target value.
  /// @param        Ratio                The interpolation ratio.
  /// @returns      FGMC_NumericValue    The interpolated value.
  FGMC_NumericValue InterpolateValue(
    uint8 Function,
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );

protected:

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateNearestNeighbour(const FGMC_NumericValue& StartValue, const FGMC_NumericValue& TargetValue, double Ratio);

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateStartValue(const FGMC_NumericValue& StartValue, const FGMC_NumericValue& TargetValue, double Ratio);

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateTargetValue(const FGMC_NumericValue& StartValue, const FGMC_NumericValue& TargetValue, double Ratio);

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateLinear(const FGMC_NumericValue& StartValue, const FGMC_NumericValue& TargetValue, double Ratio);

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateCubic(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateCustom1(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );
  virtual FGMC_NumericValue InterpolateCustom1_Implementation(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  )
  {
    return FGMC_NumericValue{EGMC_NumericType::Float, 0.};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateCustom2(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );
  virtual FGMC_NumericValue InterpolateCustom2_Implementation(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  )
  {
    return FGMC_NumericValue{EGMC_NumericType::Float, 0.};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateCustom3(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );
  virtual FGMC_NumericValue InterpolateCustom3_Implementation(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  )
  {
    return FGMC_NumericValue{EGMC_NumericType::Float, 0.};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateCustom4(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );
  virtual FGMC_NumericValue InterpolateCustom4_Implementation(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  )
  {
    return FGMC_NumericValue{EGMC_NumericType::Float, 0.};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateCustom5(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );
  virtual FGMC_NumericValue InterpolateCustom5_Implementation(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  )
  {
    return FGMC_NumericValue{EGMC_NumericType::Float, 0.};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateCustom6(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );
  virtual FGMC_NumericValue InterpolateCustom6_Implementation(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  )
  {
    return FGMC_NumericValue{EGMC_NumericType::Float, 0.};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateCustom7(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );
  virtual FGMC_NumericValue InterpolateCustom7_Implementation(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  )
  {
    return FGMC_NumericValue{EGMC_NumericType::Float, 0.};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_NumericValue InterpolateCustom8(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  );
  virtual FGMC_NumericValue InterpolateCustom8_Implementation(
    const FGMC_NumericValue& StartValue,
    const FGMC_NumericValue& TargetValue,
    const FGMC_NumericValue& PreviousValue,
    const FGMC_NumericValue& NextValue,
    double Ratio
  )
  {
    return FGMC_NumericValue{EGMC_NumericType::Float, 0.};
  }

  FGMC_PhysicsInterpState InterpolatePhysicsDefault(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  );

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_PhysicsInterpState InterpolatePhysicsCustom1(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  );
  virtual FGMC_PhysicsInterpState InterpolatePhysicsCustom1_Implementation(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  )
  {
    return FGMC_PhysicsInterpState{};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_PhysicsInterpState InterpolatePhysicsCustom2(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  );
  virtual FGMC_PhysicsInterpState InterpolatePhysicsCustom2_Implementation(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  )
  {
    return FGMC_PhysicsInterpState{};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_PhysicsInterpState InterpolatePhysicsCustom3(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  );
  virtual FGMC_PhysicsInterpState InterpolatePhysicsCustom3_Implementation(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  )
  {
    return FGMC_PhysicsInterpState{};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_PhysicsInterpState InterpolatePhysicsCustom4(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  );
  virtual FGMC_PhysicsInterpState InterpolatePhysicsCustom4_Implementation(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  )
  {
    return FGMC_PhysicsInterpState{};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_PhysicsInterpState InterpolatePhysicsCustom5(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  );
  virtual FGMC_PhysicsInterpState InterpolatePhysicsCustom5_Implementation(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  )
  {
    return FGMC_PhysicsInterpState{};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_PhysicsInterpState InterpolatePhysicsCustom6(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  );
  virtual FGMC_PhysicsInterpState InterpolatePhysicsCustom6_Implementation(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  )
  {
    return FGMC_PhysicsInterpState{};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_PhysicsInterpState InterpolatePhysicsCustom7(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  );
  virtual FGMC_PhysicsInterpState InterpolatePhysicsCustom7_Implementation(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  )
  {
    return FGMC_PhysicsInterpState{};
  }

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "General Movement Component")
  FGMC_PhysicsInterpState InterpolatePhysicsCustom8(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  );
  virtual FGMC_PhysicsInterpState InterpolatePhysicsCustom8_Implementation(
    const FGMC_PhysicsInterpState& StartState,
    const FGMC_PhysicsInterpState& TargetState,
    const FGMC_PhysicsInterpState& PreviousState,
    const FGMC_PhysicsInterpState& NextState,
    double Ratio
  )
  {
    return FGMC_PhysicsInterpState{};
  }

  virtual void SmoothComponent(
    double Time,
    float DeltaTime,
    const FTransform& AbsoluteStartTransform,
    const FTransform& AbsoluteTargetTransform,
    FGMC_SmoothComponentParams& InOutSmoothComponentParams
  );

  virtual void SaveMatchLatestSimulationStartState(FGMC_PawnState& InOutSimulationStartState);

  virtual void SaveSmoothComponentSimulationStartTransform(USceneComponent* SmoothComponent, FTransform& OutAbsoluteStartTransform);

private:

  EGMC_PhysicsInterpolationMode PhysicsInterpolationMode{EGMC_PhysicsInterpolationMode::AdaptiveDelay};

  EGMC_PhysicsInterpolationMethod PhysicsInterpolationMethod{EGMC_PhysicsInterpolationMethod::Default};

  struct FDynamicBufferTimeAux
  {
    static constexpr int32 BUFFER_SIZE = 1024;
    static constexpr int32 MIN_BUFFER_NUM_FOR_AVG = 64;
    static constexpr int32 MIN_LOW_BUFFER_NUM_FOR_AVG = 32;

    static constexpr float TRIM_PERCENTAGE = 0.25f;

    static constexpr float REQUEST_RETRY_INTERVAL = 0.2f;
    static constexpr float START_UP_TIME = 12.f;
    static constexpr float START_UP_REQUEST_HOLD_TIME = 2.f;
    static constexpr float REQUEST_HOLD_TIME = 5.f;

    static constexpr float MIN_TARGET_DIST = 0.08f;
    static constexpr float MAX_TARGET_DIST = 0.15f;
    static constexpr float MIN_TARGET_DIST_TOLERANCE = 0.02f;
    static constexpr float MAX_TARGET_DIST_TOLERANCE = 0.09f;
    static constexpr float TARGET_DIST_DECREASE_THRESHOLD = 0.03f;
    static constexpr float MIN_LOW_DIST = -0.02f;

    static constexpr float TARGET_DIST_ADJUST_PER_SEC = 0.005f;
    static constexpr float TARGET_DIST_TOLERANCE_ADJUST_PER_SEC =
      (MAX_TARGET_DIST_TOLERANCE - MIN_TARGET_DIST_TOLERANCE) / ((MAX_TARGET_DIST - MIN_TARGET_DIST) / TARGET_DIST_ADJUST_PER_SEC);
    static constexpr float LOW_DIST_INCREASE_PER_SEC = 0.04f;

    TGMC_CircularNumericBuffer<float, BUFFER_SIZE> AllBuffer{};

    TGMC_CircularNumericBuffer<float, BUFFER_SIZE> LowBuffer{};

    TGMC_CircularNumericBuffer<float, BUFFER_SIZE> DiffBuffer{};

    float TargetDist{MIN_TARGET_DIST};

    float TargetDistTolerance{MIN_TARGET_DIST_TOLERANCE};

    float LowDist{0.f};

    float SignedDiff{0.f};

    float RequestedBufferTime{0.f};

    double RequestTimer{-START_UP_REQUEST_HOLD_TIME};

    void Update(UGMC_ReplicationCmp* const Outer);

    bool Request(UGMC_ReplicationCmp* const Outer);

    void Apply(UGMC_ReplicationCmp* const Outer);

    bool WantsNewBufferTime() const;

    bool HasPendingBufferTime() const;

    bool CanRequest(UGMC_ReplicationCmp* const Outer, AGMC_PlayerController* const LocalController);

    float GetRequestHoldTime(UGMC_ReplicationCmp* const Outer) const;

    void Reset()
    {
      AllBuffer.Reset();
      LowBuffer.Reset();
      DiffBuffer.Reset();
      TargetDist = MIN_TARGET_DIST;
      TargetDistTolerance = MIN_TARGET_DIST_TOLERANCE;
      LowDist = 0.f;
      SignedDiff = 0.f;
      RequestedBufferTime = 0.f;
      RequestTimer = -START_UP_REQUEST_HOLD_TIME;
    }
  };

  FDynamicBufferTimeAux DynamicBufferTimeAux{};

  // Auxiliary struct for "Match Latest" smoothing because we can't define a new pawn state in FGMC_MatchLatestPersistentParams (circular dependencies).
  struct FMatchLatestParamsAux
  {
    // The start state for the current simulation iteration.
    FGMC_PawnState SimulationStartState{};

    // Flag to verify that the simulation start state has been initialized.
    bool bSimulationStartStateInitialized{false};

    void Reset()
    {
      SimulationStartState = FGMC_PawnState{};
      bSimulationStartStateInitialized = false;
    }
  };

  FMatchLatestParamsAux MatchLatestParamsAux{};

  struct FSimulationAux
  {
    // -1 if no interpolation happened during the last smoothing update.
    double PrevInterpTargetTime{-1.};

    // -1 if no extrapolation happened during the last smoothing update.
    double PrevExtrapTargetTime{-1.};

    // Only -1 if SmoothMovement did not run at all during the last update.
    double PrevLatestTime{-1.};

    // Only -1 if neither interpolation nor extrapolation happened during the last smoothing update.
    double PrevSmoothingTime{-1.};

    // Only true during the SmoothMovement function by default.
    bool bIsSimulating{false};

    // Only relevant for cumulative smoothing.
    bool bIsCumulativeUpdate{false};

    uint64 NumFramesSinceLastSimulation{0};

    FVector ExtrapolationStartLocation{0.};

    double AccExtrapolatedDistance{0.};

    double AbsoluteExtrapolatedDistance{0.};

    bool bMaxExtrapolationDistanceReached{false};

    bool bIsExtrapolating{false};

    bool bWasExtrapolatingLastUpdate{false};

    float ExtrapolationRecoveryTimer{-1.f};

    FGMC_PawnState ExtrapolationRecoveryStartState{};

    bool bPerformExtrapolationRecoveryInWorldSpace{false};

    bool bTriggerManualExtrapolationRecovery{false};

    bool bManualExtrapolationRecoveryInWorldSpace{false};

    uint64 NumAbortedExtrapolationRecoveries{0};

    static constexpr int32 RECOVERY_JITTER_GUARD_NUM = 10;
    static constexpr double RECOVER_ACTOR_LOCATION_THRESHOLD = 0.01;
    static constexpr double RECOVER_ACTOR_ROTATION_THRESHOLD = 0.01;
    static constexpr double RECOVER_ACTOR_SCALE_THRESHOLD = 0.01;
    static constexpr double RECOVER_ACTOR_CONTROL_ROTATION_THRESHOLD = 0.01;

    void PreTick(UGMC_ReplicationCmp* const Outer);

    void Reset()
    {
      PrevInterpTargetTime = -1.;
      PrevExtrapTargetTime = -1.;
      PrevLatestTime = -1.;
      PrevSmoothingTime = -1.;
      bIsSimulating = false;
      bIsCumulativeUpdate = false;
      NumFramesSinceLastSimulation = 0;
      ExtrapolationStartLocation = FVector::ZeroVector;
      AccExtrapolatedDistance = 0.;
      AbsoluteExtrapolatedDistance = 0.;
      bMaxExtrapolationDistanceReached = false;
      bIsExtrapolating = false;
      bWasExtrapolatingLastUpdate = false;
      ExtrapolationRecoveryTimer = -1.f;
      ExtrapolationRecoveryStartState = FGMC_PawnState{};
      bPerformExtrapolationRecoveryInWorldSpace = false;
      bTriggerManualExtrapolationRecovery = false;
      bManualExtrapolationRecoveryInWorldSpace = false;
      NumAbortedExtrapolationRecoveries = 0;
    }
  };

  FSimulationAux SimulationAux{};

  void SmoothMovement(float DeltaTime, double& OutSimTime, int32& OutTargetIdx, TArray<int32>& OutSkippedStateIndices);

  bool ShouldSimulatePawn(double MaxSmoothingDistance, double SmoothingFallOffDistance, int32 MaxSkippedSmoothingFrames) const;

  EGMC_NetContext GetSmoothingContext() const;

  void DetermineSkippedStates(TArray<int32>& OutSkippedStateIndices, int32 StartIdx, int32 TargetIdx, double PrevTargetTimestamp) const;

  void SetSimulatedPawnState(const FGMC_PawnState& State, bool bSkipBoneUpdate, bool bUseRelative, EGMC_NetContext Context);

  void ComputeSmoothingParams(
    float DeltaTime,
    FGMC_MatchLatestTransientParams& MatchLatest,
    FGMC_FixedDelayTransientParams& FixedDelay,
    FGMC_AdaptiveDelayTransientParams& AdaptiveDelay,
    FGMC_UniformSimulationTransientParams& UniformSimulation,
    FGMC_CumulativeSimulationTransientParams& CumulativeSimulation
  );

  void SmoothNone(
    int32 LastIdx,
    bool bRollback,
    EGMC_InterpolationStates InterpStates,
    bool bSkipBoneUpdate,
    EGMC_NetContext Context
  );

  void SmoothMatchLatest(
    int32 LastIdx,
    int32 PrevIdx,
    float Alpha,
    bool bInterpolating,
    bool bRollback,
    float TeleportThreshold,
    EGMC_InterpolationStates InterpStates,
    bool bSkipBoneUpdate,
    EGMC_NetContext Context
  );

  void SmoothDelay(
    int32 StartIdx,
    int32 TargetIdx,
    float Alpha,
    bool bInterpolating,
    bool bRollback,
    float ExtrapolationRecoveryTime,
    float TeleportThreshold,
    EGMC_InterpolationStates InterpStates,
    bool bSkipBoneUpdate,
    EGMC_NetContext Context
  );

  void SmoothSimulated(
    int32 LastIdx,
    bool bCumulativeUpdate,
    float DeltaTime,
    double Timestamp,
    float InMaxTimeStep,
    int32 InMaxIterations,
    bool bInterpolating,
    bool bRollback,
    EGMC_InterpolationStates SimStates,
    bool bSkipBoneUpdate,
    EGMC_NetContext Context
  );

  FGMC_PhysicsInterpState InterpolatePhysicsState(
    const FGMC_Move& StartMove,
    const FGMC_Move& TargetMove,
    const FGMC_Move& PreviousMove,
    const FGMC_Move& NextMove,
    float Alpha,
    EGMC_InterpolationStates InterpStates
  );

  void ExtrapolationRecovery(
    bool bInterpolating,
    float RecoveryTime,
    float& Alpha,
    EGMC_InterpolationStates InterpStates,
    const FGMC_PawnState& StartState,
    FGMC_PawnState& InterpolatedState
  );

  FRotator ControlRotationToLocal(const FRotator& WorldControlRotation) const;

  void ManageAdaptiveSimulationDelay();

  float GetAdaptiveDelayAt(double Time, APlayerController* ClientController);

  float GetDynamicBufferTimeAt(double Time, APlayerController* ClientController);

  void SV_UpdateLocalAdaptiveDelay();

  void SV_UpdateClientAdaptiveDelays();

  void SV_UpdateAdaptiveDelayBufferTime(APlayerController* ClientController, float NewBufferTime);

  void CL_UpdateLocalAdaptiveDelay();

  void CL_SendAdaptiveDelayParams(const FGMC_AdaptiveDelayServerPacket& NextDelay);

///=============================================================================================================================================================
/// Properties

public:

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking")
  /// Determines the replication settings for properties that are synchronised by default. These settings must always match between client and server for any
  /// actor instance that replicates and they cannot be changed anymore after the actor was spawned.
  FGMC_DefaultReplicationSettings ReplicationSettings{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// If true, all built-in smoothing logic will be skipped. Enable if you want to handle simulated pawns completely by yourself through the simulation tick.
  bool bNoSmoothing{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  EGMC_InterpolationMode InterpolationMode{EGMC_InterpolationMode::AdaptiveDelay};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  EGMC_ExtrapolationMode ExtrapolationMode{EGMC_ExtrapolationMode::CumulativeSimulation};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  FGMC_MatchLatestPersistentParams MatchLatestParams{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  FGMC_FixedDelayPersistentParams FixedDelayParams{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  FGMC_AdaptiveDelayPersistentParams AdaptiveDelayParams{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  FGMC_UniformSimulationPersistentParams UniformSimulationParams{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  FGMC_CumulativeSimulationPersistentParams CumulativeSimulationParams{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// Whether input or output states should be used when the interpolation mode is set to "None".
  EGMC_InterpolationStates NoInterpStates{EGMC_InterpolationStates::Output};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// The interpolation method to use for simulated proxies that are displaying physics.
  EGMC_PhysicsInterpolationMethod SimulatedProxyPhysicsInterp{EGMC_PhysicsInterpolationMethod::Default};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing", meta = (ClampMin = "0", UIMin = "100", UIMax = "10000"))
  /// How far the pawn is allowed to be extrapolated from the last known location.
  double MaxExtrapolationDistance{5000.};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// Whether to smooth the actor location during extrapolation recovery.
  bool bRecoverActorLocation{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// Whether to smooth the actor rotation during extrapolation recovery.
  bool bRecoverActorRotation{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// Whether to smooth the actor scale during extrapolation recovery.
  bool bRecoverActorScale{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// Whether to smooth the control rotation during extrapolation recovery.
  bool bRecoverControlRotation{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// Whether we should skip updating kinematic bones during smoothing when the pawn has a skeletal mesh component with a physics state.
  bool bSkipBoneUpdateForSmoothing{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// Whether states that were skipped during the current smoothing iteration should be passed to the simulation tick.
  bool bDetermineSkippedSmoothingStates{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// The settings for post-process smoothing of a set component after simulation.
  FGMC_SmoothComponentParams PostSmoothingParams{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Smoothing")
  /// The settings for distance-based simulation throttling.
  FGMC_SimulationThrottle SimulationThrottle{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Client")
  /// Predict the movement of the client locally to hide network latency.
  bool bUseClientPrediction{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Client", meta = (ClampMin = "0", UIMin = "0", UIMax = "2"))
  /// When the autonomous proxy receives a server update it looks for the past source move associated with the received data. If none can be found (which can
  /// happen during latency spikes) we cannot check the current client state for correctness. At this point we have the option to reset the client to the
  /// received server state (most likely causing a teleport) or to wait for another update that produces a valid source move. The configured value here tells us
  /// how long we are willing to wait for such an update (in seconds).
  float MaxClientUpdateWaitTime{1.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Client")
  /// When enabled the client will always replay upon receiving a server update. Enabling may cause a large performance hit and can sometimes introduce
  /// instabilities in the client state that would otherwise not occur. This option should generally be disabled except for testing purposes.
  bool bAlwaysReplay{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Client")
  /// When enabled the client will never combine any moves. Enabling may increase bandwidth usage significantly (especially at high frame rates). This option
  /// should generally be disabled except for testing purposes.
  bool bNoMoveCombining{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Client", meta = (ClampMin = "0.000001", UIMin = "0.01", UIMax = "0.2"))
  /// The maximum delta time allowed for a combined client move before a new move has to be enqueued. In some situations you may want to enqueue moves more
  /// frequently so the server gets updates faster (the client send rate is still limited by how quickly we enqueue new moves). As this will not clamp the delta
  /// time of a client move, it can be set to small values without affecting the movement simulation (unlike the max move delta time). E.g. if you set this to
  /// 0.02 you will enqueue every move up to 50 fps. Cannot be greater than the max move delta time (will be lowered to match the max move delta time if it is).
  float MaxCombinedDeltaTime{0.03334f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Client", meta = (ClampMin = "0", UIMin = "1", UIMax = "60"))
  /// How often per second the client will send data to the server. This is an upper bound i.e. the interval between two updates will never be lower than
  /// 1 / client send rate. However, more often than not it will be longer to save bandwidth if no important values changed on the client (moves will be
  /// combined). In this case the lower bound is determined by the max combined delta time.
  int32 ClientSendRate{30};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Client", meta = (ClampMin = "-1", UIMin = "5", UIMax = "50"))
  /// If there are more reliable RPC packets than the set threshold waiting to be acknowledged by the server, we will switch to unreliable transmission for
  /// sending client moves to the server until the network congestion subsides. Set to -1 to always use unreliable RPCs.
  int32 UseUnreliableClientMovesThreshold{10};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Client")
  /// If true, any client-auth default sync types will be restored to the values they had before the correction after a replay.
  bool bRestoreClientAuthValuesAfterReplay{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Client")
  /// The settings for smoothing corrections of predicted clients.
  FGMC_SmoothCorrection SmoothCorrection{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Server", meta = (ClampMin = "0", UIMin = "0", UIMax = "10"))
  /// How often per second the client pawn must be updated on the server at the least. If no data is received from the client for a period of
  /// 1 / server min update rate seconds the last known client input will be reused to execute proxy moves on the server to meet the targeted update rate. This
  /// value should always be significantly lower than the client send rate. Set to 0 to disable this functionality.
  int32 ServerMinUpdateRate{0};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Server")
  /// Assume the values of server validated variables sent by the client if they match the calculated server state within the associated tolerance.
  bool bAssumeClientState{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Server", meta = (ClampMin = "1", UIMin = "1", UIMax = "30"))
  /// How often per second predicted clients should receive an update from the server. Cannot be larger than the net update frequency of the actor.
  int32 PredictedClientNetUpdateFrequency{10};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Server")
  /// When enabled bandwidth usage is reduced for server to client replication. Specifically, network traffic is optimized by comparing the last serialized
  /// value to the one that is about to be replicated: if it has changed it is reserialized, otherwise only 1 bit is replicated to indicate to the client that
  /// the last received value can be used again. Should generally always be enabled except for testing purposes or when targeting network environments with
  /// very high packet loss.
  bool bUseConditionalNetSerialization{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Server", meta = (ClampMin = "0", UIMin = "0", UIMax = "5"))
  /// If greater than 0 the server will reserialize all data anew at the specified interval. This is intended as a sort of fail-safe mechanism for move data
  /// replication when there is packet loss, otherwise client pawns can get stuck in a wrong state if the server assumes a packet was received by the client
  /// when in actuality it was lost. Only relevant when using conditional net serialization.
  float NetReserializationInterval{0.5f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Server")
  /// Whether pawns on a listen server that are remotely controlled by a client should be smoothed. Setting this to false essentially allows you to see how
  /// movement would happen on a dedicated server as these will never run any smoothing logic. Disabling this is not the same same as just disabling
  /// interpolation, and some animations/effects might not play. This option should generally be enabled except for testing purposes.
  bool bSmoothRemoteListenServerPawn{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Server", meta = (ClampMin = "-1", UIMin = "10", UIMax = "1000"))
  /// Setting this to a value >= 0 will force additional net updates based on the configured location distance threshold.
  double SimLocationUpdateThreshold{-1.};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Server", meta = (ClampMin = "1", UIMin = "1", UIMax = "10"))
  /// Determines how many remote moves will be processed in one server frame at most. Lower values introduce less additional latency but can have a larger
  /// impact on the performance of individual server ticks. Higher values distribute the workload more evenly and may also produce better simulation results in
  /// bad network conditions.
  int32 RemoteMoveProcessingChunkSize{1};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Server", meta = (ClampMin = "0", UIMin = "10", UIMax = "30"))
  /// How many remote moves the server is allowed to buffer at most. The exceeding moves will all be executed immediately within the same frame until the amount
  /// of elements in the buffer is below the threshold again. Only relevant if the processing chunk size is greater than 1.
  int32 MaxNumBufferedRemoteMoves{10};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation")
  /// When enabled, all other pawns of type AGMC_Pawn are set back in time for move execution on the server, so the server world resembles the client world more
  /// closely at the point in time when the client originally executed the move. This can significantly improve the client experience, but comes at the cost of
  /// an increased server performance overhead, especially when many clients are connected. For this to produce good results, all smoothing parameters must be
  /// synchronised between server and client for any pawn that is rolled back.
  bool bRollBackServerPawns{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation")
  /// When enabled, all simulated proxies of type AGMC_Pawn are set back in time for replay on the client. This can significantly improve replay accuracy at the
  /// cost of an increased client performance overhead.
  bool bRollBackClientPawns{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation")
  /// When enabled, all actors of type AGMC_RollbackActor on the server update their state before each move execution.
  bool bRollBackGenericServerActors{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation")
  /// When enabled, all actors of type AGMC_RollbackActor on the client update their state before each move execution that happens as part of a client replay.
  bool bRollBackGenericClientActors{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation", meta = (ClampMin = "0", UIMin = "100", UIMax = "5000"))
  /// If server pawn rollback is enabled, this is the radius of a sphere around the pawn within which other pawns will be considered for rollback on the server.
  double ServerPawnRollbackRadius{2500.};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation", meta = (ClampMin = "0", UIMin = "100", UIMax = "5000"))
  /// If client pawn rollback is enabled, this is the radius of a sphere around the pawn within which other pawns will be considered for rollback on the client.
  double ClientPawnRollbackRadius{2500.};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation", meta = (ClampMin = "0", UIMin = "100", UIMax = "5000"))
  /// If generic server actor rollback is enabled, this is the radius of a sphere around the pawn within which actors will be considered for rollback on the
  /// server.
  double GenericServerActorRollbackRadius{2500.};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation", meta = (ClampMin = "0", UIMin = "100", UIMax = "5000"))
  /// If generic client actor rollback is enabled, this is the radius of a sphere around the pawn within which actors will be considered for rollback on the
  /// client.
  double GenericClientActorRollbackRadius{2500.};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation")
  /// If true, this pawn will never be rolled back by any other pawn for move execution.
  bool bExcludeFromRollback{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Lag Compensation")
  /// Whether we should skip updating kinematic bones during rollback when the pawn has a skeletal mesh component with a physics state.
  bool bSkipBoneUpdateForRollback{true};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Verification")
  /// Whether the server should verify the timestamps of the moves received from the client.
  bool bVerifyClientTimestamps{false};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Verification", meta = (ClampMin = "0", UIMin = "0.05", UIMax = "0.5"))
  /// The maximum amount in seconds that the estimated client time is allowed to differ from the server time. The appropriate value here depends on several
  /// factors (mainly the quality of the client connection), try to find the lowest value that does not fail verifications with legit input in a real network
  /// setting.
  float MaxAllowedTimeDeviation{0.1f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Verification", meta = (ClampMin = "0", UIMin = "1", UIMax = "10"))
  /// The time interval for resetting the client credibility. If the client fails a verification no moves will be executed anymore until the next verification
  /// period has started.
  float VerificationPeriod{3.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking", AdvancedDisplay, meta = (ClampMin = "0.000001", UIMin = "0.04", UIMax = "0.2"))
  /// The maximum delta time allowed for a move. This means that the movement will effectively slow down if the frame rate falls below 1 / MaxMoveDeltaTime so
  /// this value should not be too low (but also not too high because then you may get a performance hit from physics sub-stepping). Cannot be smaller than the
  /// inverse of the client send rate (if it is the value will be increased to match the client send interval i.e. 1 / client send rate).
  float MaxMoveDeltaTime{0.05f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking", AdvancedDisplay, meta = (ClampMin = "0.000001", UIMin = "0.01"))
  /// The time interval in which to subdivide the delta time of a move for move execution.
  float MaxTimeStep{0.04f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking", AdvancedDisplay, meta = (ClampMin = "1", UIMin = "1"))
  /// How many iterations of a sub-stepped move execution are allowed at most. If upon reaching the last iteration there is still more time than the max allowed
  /// timestep remaining, all the remaining time will be used in the last iteration.
  int32 MaxIterations{10};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking", AdvancedDisplay, meta = (ClampMin = "0", UIMin = "0"))
  /// How often moves should be executed per second for non-player controlled server pawns (0 means execute every frame). The configured max move delta time is
  /// still enforced.
  int32 MoveExecutionFrequency{0};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking", AdvancedDisplay, meta = (ClampMin = "8", UIMin = "128", UIMax = "512"))
  /// The max size of the array containing the saved moves.
  int32 MoveHistoryMaxSize{256};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking", AdvancedDisplay)
  /// Disables all Blueprint events called for this component.
  bool bNoBlueprintEvents{false};

///=============================================================================================================================================================
/// Data binding

public:

  ///***********************************************************************************************************************************************************
  /// Binding functions to integrate variables into the replication system. It is strongly recommended to only bind members of the movement component itself.
  ///
  /// @param        VariableToBind    The variable to bind.
  /// @param        PredictionMode    How the variable should be handled when used by a predicted client. Client-authoritative variables are taken at face value
  ///                                 by the server and by definition cannot be "wrong". Server-authoritative values are validated and/or replicated by the
  ///                                 server and may cause a client correction if a deviation from the authoritative value is detected.
  /// @param        CombineMode       Determines under which conditions the current client input value of the variable can be combined with the previous one.
  /// @param        SimulationMode    How the variable should be replicated to simulated clients. Simulated pawns do not run the full movement logic locally,
  ///                                 they receive discrete states at a certain frequency from the server.
  /// @param        Interpolation     How the variable should be interpolated locally. Only relevant when the simulation mode is not "None".
  /// @returns      int32             The binding index of the variable. Can be used to retrieve the value of the bound variable from a state at runtime.
  ///***********************************************************************************************************************************************************

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a boolean value. Only uses 1 bit for replication."))
  UPARAM(DisplayName = "Binding Index") int32 BindBool(
    UPARAM(Ref) bool& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Bit.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind an unsigned 8 bit integer that only has its lower 4 bits replicated."))
  UPARAM(DisplayName = "Binding Index") int32 BindHalfByte(
    UPARAM(Ref) uint8& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.UnsignedInt4.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a unsigned 8 bit integer."))
  UPARAM(DisplayName = "Binding Index") int32 BindByte(
    UPARAM(Ref) uint8& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.UnsignedInt8.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a signed 32 bit integer."))
  UPARAM(DisplayName = "Binding Index") int32 BindInt(
    UPARAM(Ref) int32& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Int32.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a single-precision float. Replicated with full precision."))
  UPARAM(DisplayName = "Binding Index") int32 BindSinglePrecisionFloat(
    UPARAM(Ref) float& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Float.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a compressed single-precision float. Replicated with 2 decimal places of accuracy."))
  UPARAM(DisplayName = "Binding Index") int32 BindCompressedSinglePrecisionFloat(
    UPARAM(Ref) float& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Float2Dec.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a double-precision float. Replicated with full precision."))
  UPARAM(DisplayName = "Binding Index") int32 BindDoublePrecisionFloat(
    UPARAM(Ref) double& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Double.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a compressed double-precision float. Replicated with 2 decimal places of accuracy."))
  UPARAM(DisplayName = "Binding Index") int32 BindCompressedDoublePrecisionFloat(
    UPARAM(Ref) double& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Double2Dec.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a double-precision float. Replicated as single-precision float."))
  UPARAM(DisplayName = "Binding Index") int32 BindTruncatedDoublePrecisionFloat(
    UPARAM(Ref) double& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.DoubleAsFloat.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a compressed 2D vector. Replicated with 2 decimal places of precision."))
  UPARAM(DisplayName = "Binding Index") int32 BindCompressedVector2D(
    UPARAM(Ref) FVector2D& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Vector2Double2Dec.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a compressed 3D vector. Replicated with 2 decimal places of precision."))
  UPARAM(DisplayName = "Binding Index") int32 BindCompressedVector(
    UPARAM(Ref) FVector& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Vector3Double2Dec.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a compressed 3D rotator. Replicated with 2 decimal places of precision."))
  UPARAM(DisplayName = "Binding Index") int32 BindCompressedRotator(
    UPARAM(Ref) FRotator& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Rotator3Double2Dec.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a reference to an actor."))
  UPARAM(DisplayName = "Binding Index") int32 BindActorReference(
    UPARAM(Ref) AActor*& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.ActorReference.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a reference to an actor component."))
  UPARAM(DisplayName = "Binding Index") int32 BindActorComponentReference(
    UPARAM(Ref) UActorComponent*& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.ActorComponentReference.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a reference to an animation montage."))
  UPARAM(DisplayName = "Binding Index") int32 BindAnimMontageReference(
    UPARAM(Ref) UAnimMontage*& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.AnimMontageReference.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a name."))
  UPARAM(DisplayName = "Binding Index") int32 BindName(
    UPARAM(Ref) FName& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.Name.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a gameplay tag."))
  UPARAM(DisplayName = "Binding Index") int32 BindGameplayTag(
    UPARAM(Ref) FGameplayTag& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.GameplayTag.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind a gameplay tag container."))
  UPARAM(DisplayName = "Binding Index") int32 BindGameplayTagContainer(
    UPARAM(Ref) FGameplayTagContainer& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.GameplayTagContainer.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component",
    meta = (ToolTip = "Bind an instanced struct."))
  UPARAM(DisplayName = "Binding Index") int32 BindInstancedStruct(
    UPARAM(Ref) FInstancedStruct& VariableToBind,
    EGMC_PredictionMode PredictionMode,
    EGMC_CombineMode CombineMode,
    EGMC_SimulationMode SimulationMode,
    EGMC_InterpolationFunction Interpolation
  )
  {
    int32 BindingIndex = -1;
    AliasData.InstancedStruct.BindMember(
      VariableToBind,
      TranslateToSyncSettings(PredictionMode, CombineMode, SimulationMode, Interpolation),
      BindingIndex
    );
    gmc_ck(BindingIndex >= 0)
    return BindingIndex;
  }

  ///***********************************************************************************************************************************************************
  /// Getters for bound data from a pawn state via the binding index.
  ///
  /// @param        Index    The index that was returned when the member was bound. Passing anything else here might cause a crash.
  /// @param        State    The state from which to retrieve the bound value at the specified index.
  /// @returns      T        The bound value from the pawn state at the specified index.
  ///***********************************************************************************************************************************************************

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  bool GetBoundBool(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Bit.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  uint8 GetBoundHalfByte(int32 Index, const FGMC_PawnState& State) const
  {
    return State.UnsignedInt4.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  uint8 GetBoundByte(int32 Index, const FGMC_PawnState& State) const
  {
    return State.UnsignedInt8.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  int32 GetBoundInt(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Int32.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  float GetBoundSinglePrecisionFloat(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Float.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  float GetBoundCompressedSinglePrecisionFloat(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Float2Dec.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double GetBoundDoublePrecisionFloat(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Double.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double GetBoundCompressedDoublePrecisionFloat(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Double2Dec.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  double GetBoundTruncatedDoublePrecisionFloat(int32 Index, const FGMC_PawnState& State) const
  {
    return State.DoubleAsFloat.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector2D GetBoundCompressedVector2D(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Vector2Double2Dec.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FVector GetBoundCompressedVector(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Vector3Double2Dec.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FRotator GetBoundCompressedRotator(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Rotator3Double2Dec.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  AActor* GetBoundActorReference(int32 Index, const FGMC_PawnState& State) const
  {
    return State.ActorReference.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  UActorComponent* GetBoundActorComponentReference(int32 Index, const FGMC_PawnState& State) const
  {
    return State.ActorComponentReference.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  UAnimMontage* GetBoundAnimMontageReference(int32 Index, const FGMC_PawnState& State) const
  {
    return State.AnimMontageReference.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FName GetBoundName(int32 Index, const FGMC_PawnState& State) const
  {
    return State.Name.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FGameplayTag GetBoundGameplayTag(int32 Index, const FGMC_PawnState& State) const
  {
    return State.GameplayTag.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FGameplayTagContainer GetBoundGameplayTagContainer(int32 Index, const FGMC_PawnState& State) const
  {
    return State.GameplayTagContainer.Read(Index);
  }

  UFUNCTION(BlueprintCallable, Category = "General Movement Component")
  FInstancedStruct GetBoundInstancedStruct(int32 Index, const FGMC_PawnState& State) const
  {
    return State.InstancedStruct.Read(Index);
  }

  ///***********************************************************************************************************************************************************
  /// Setters for bound data of a pawn state via the binding index.
  ///
  /// @param        Value    The new value to set the data in the pawn state to.
  /// @param        Index    The index that was returned when the member was bound. Passing anything else here might cause a crash.
  /// @param        State    The state of which to set the bound value at the specified index.
  /// @returns      void
  ///***********************************************************************************************************************************************************

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundBool(const bool& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Bit.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundHalfByte(const uint8& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.UnsignedInt4.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundByte(const uint8& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.UnsignedInt8.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundInt(const int32& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Int32.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundSinglePrecisionFloat(const float& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Float.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedSinglePrecisionFloat(const float& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Float2Dec.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundDoublePrecisionFloat(const double& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Double.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedDoublePrecisionFloat(const double& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Double2Dec.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundTruncatedDoublePrecisionFloat(const double& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.DoubleAsFloat.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedVector2D(const FVector2D& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Vector2Double2Dec.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedVector(const FVector& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Vector3Double2Dec.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedRotator(const FRotator& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Rotator3Double2Dec.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundActorReference(AActor* const& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.ActorReference.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundActorComponentReference(UActorComponent* const& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.ActorComponentReference.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundAnimMontageReference(UAnimMontage* const& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.AnimMontageReference.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundName(FName const& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Name.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundGameplayTag(FGameplayTag const& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.GameplayTag.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundGameplayTagContainer(FGameplayTagContainer const& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.GameplayTagContainer.Write(Value, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundInstancedStruct(FInstancedStruct const& Value, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.InstancedStruct.Write(Value, Index);
  }

  ///***********************************************************************************************************************************************************
  /// Suspending a variable stops replication of the value to the target machine.
  ///
  /// @param        bSuspend    Whether the bound variable should be suspended from replication to the target machine.
  /// @param        Index       The index that was returned when the member was bound. Passing anything else here might cause a crash.
  /// @param        State       The state of which to suspend the bound variable at the specified index (must be a state of the local, AP or SP move).
  /// @returns      void
  ///***********************************************************************************************************************************************************

  /// SetBoundBoolSuspended doesn't exist as suspension is not supported for bools (since they contain just 1 bit of information anyway).

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundHalfByteSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.UnsignedInt4.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundByteSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.UnsignedInt8.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundIntSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Int32.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundSinglePrecisionFloatSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Float.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedSinglePrecisionFloatSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Float2Dec.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundDoublePrecisionFloatSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Double.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedDoublePrecisionFloatSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Double2Dec.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundTruncatedDoublePrecisionFloatSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.DoubleAsFloat.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedVector2DSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Vector2Double2Dec.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedVectorSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Vector3Double2Dec.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundCompressedRotatorSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Rotator3Double2Dec.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundActorReferenceSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.ActorReference.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundActorComponentReferenceSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.ActorComponentReference.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundAnimMontageReferenceSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.AnimMontageReference.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundNameSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.Name.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundGameplayTagSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.GameplayTag.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundGameplayTagContainerSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.GameplayTagContainer.SetSuspended(bSuspend, Index);
  }

  UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "General Movement Component")
  void SetBoundInstancedStructSuspended(bool bSuspend, int32 Index, UPARAM(Ref) FGMC_PawnState& State) const
  {
    State.InstancedStruct.SetSuspended(bSuspend, Index);
  }
};
