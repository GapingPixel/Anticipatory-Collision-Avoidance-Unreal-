// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "Bot/AITasks/ADogAITask_UpdateMoveTarget.h"
#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_4
#include "NavFilters/NavigationQueryFilter.h"
#endif
#include "BTTask_AlphaDogMoveTo.generated.h"

struct FBTADogMoveToTaskMemory : FBTMoveToTaskMemory
{
	FUpdateMoveTargetRequest UpdateMoveTargetRequest;
	
	TWeakObjectPtr<UADogAITask_UpdateMoveTarget> MoveTargetTask;
};

/**
 * Move To task node specific to quadruped movements.
 * Provides a mechanism for forcing turns to be wider and more rounded corners as would be natural for dogs or
 * other quadrupeds. Uses TargetActor to determine movement but does not actually MoveTo TargetActor, rather it
 * MovesTo the ADogMoveTarget. TargetActor is used to determine movement of the ADogMoveTarget.
 */
UCLASS(MinimalAPI)
class UBTTask_AlphaDogMoveTo : public UBTTask_MoveTo
{
	GENERATED_UCLASS_BODY()

public:
	
	/** Base turn rate used for interpolating move target rotation */
	UPROPERTY(Category = MoveTarget, EditAnywhere)
	float TurnRate;

	UPROPERTY(Category = MoveTarget, EditAnywhere)
	float TurnRateTimerLimit;

	UPROPERTY(Category = MoveTarget, EditAnywhere)
	float TurnRateThrottleDistance;

	UPROPERTY(Category = MoveTarget, EditAnywhere)
	float TurnThrottleTrigger;

	UPROPERTY(Category = MoveTarget, EditAnywhere)
	float MoveTargetDistanceSpeed;

	UPROPERTY(Category = MoveTarget, EditAnywhere)
	float MoveTargetDistanceMax;

	/** Draw the position of the move target */
	UPROPERTY(Category = MoveTarget, EditAnywhere)
	uint32 bDrawDebug : 1;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;

	virtual EBTNodeResult::Type PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
protected:

	virtual EBTNodeResult::Type PerformMoveTargetTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
	
	/** prepares move task for activation */
	virtual UADogAITask_UpdateMoveTarget* PrepareUpdateMoveTargetTask(UBehaviorTreeComponent& OwnerComp,
		UADogAITask_UpdateMoveTarget* ExistingTask, FUpdateMoveTargetRequest UpdateMoveTargetRequest);

};
