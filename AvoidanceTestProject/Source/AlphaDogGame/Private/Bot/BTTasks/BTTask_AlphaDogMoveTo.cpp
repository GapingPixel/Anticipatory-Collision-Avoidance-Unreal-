// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/BTTasks/BTTask_AlphaDogMoveTo.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Bot/ADogMoveTarget.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Player/ADogPlayerBotController.h"
#include "Tasks/AITask_MoveTo.h"

UBTTask_AlphaDogMoveTo::UBTTask_AlphaDogMoveTo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "AlphaDog Move To";

	TurnRate = 4.f;
	TurnRateTimerLimit = 0.5f;
	TurnRateThrottleDistance = 200;
	TurnThrottleTrigger = -0.75;
	MoveTargetDistanceSpeed = 2.0f;
	MoveTargetDistanceMax = 400.f;
	bDrawDebug = false;
}


EBTNodeResult::Type UBTTask_AlphaDogMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type NodeResult = EBTNodeResult::InProgress;

	FBTADogMoveToTaskMemory* MyMemory = CastInstanceNodeMemory<FBTADogMoveToTaskMemory>(NodeMemory);
	MyMemory->PreviousGoalLocation = FAISystem::InvalidLocation;
	MyMemory->MoveRequestID = FAIRequestID::InvalidRequest;

	AADogPlayerBotController* MyController = Cast<AADogPlayerBotController>(OwnerComp.GetAIOwner());
	if (MyController == nullptr)
	{
		UE_VLOG(OwnerComp.GetOwner(), LogBehaviorTree, Error, TEXT("UBTTask_AlphaDogMoveTo::ExecuteTask failed since AADogPlayerBotController is missing."));
		NodeResult = EBTNodeResult::Failed;
	}
	else
	{
		PerformMoveTargetTask(OwnerComp, NodeMemory);
		NodeResult = PerformMoveTask(OwnerComp, NodeMemory);
	}

	if (NodeResult == EBTNodeResult::InProgress && bObserveBlackboardValue)
	{
		UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
		if (ensure(BlackboardComp))
		{
			if (MyMemory->BBObserverDelegateHandle.IsValid())
			{
				UE_VLOG(MyController, LogBehaviorTree, Warning, TEXT("UBTTask_MoveTo::ExecuteTask \'%s\' Old BBObserverDelegateHandle is still valid! Removing old Observer."), *GetNodeName());
				BlackboardComp->UnregisterObserver(BlackboardKey.GetSelectedKeyID(), MyMemory->BBObserverDelegateHandle);
			}
			MyMemory->BBObserverDelegateHandle = BlackboardComp->RegisterObserver(BlackboardKey.GetSelectedKeyID(), this, FOnBlackboardChangeNotification::CreateUObject(this, &UBTTask_MoveTo::OnBlackboardValueChange));
		}
	}	
	
	return NodeResult;
}

void UBTTask_AlphaDogMoveTo::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	FBTADogMoveToTaskMemory* MyMemory = CastInstanceNodeMemory<FBTADogMoveToTaskMemory>(NodeMemory);
	MyMemory->MoveTargetTask.Reset();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

EBTNodeResult::Type UBTTask_AlphaDogMoveTo::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTADogMoveToTaskMemory* MyMemory = CastInstanceNodeMemory<FBTADogMoveToTaskMemory>(NodeMemory);
	
	UADogAITask_UpdateMoveTarget* moveTargetTask = MyMemory->MoveTargetTask.Get();
	if (moveTargetTask)
	{
		moveTargetTask->ExternalCancel();
	}
	
	return Super::AbortTask(OwnerComp, NodeMemory);
}

uint16 UBTTask_AlphaDogMoveTo::GetInstanceMemorySize() const
{
	return sizeof(FBTADogMoveToTaskMemory);
}

void UBTTask_AlphaDogMoveTo::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FBTADogMoveToTaskMemory>(NodeMemory, InitType);
}

void UBTTask_AlphaDogMoveTo::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FBTADogMoveToTaskMemory>(NodeMemory, CleanupType);
}

EBTNodeResult::Type UBTTask_AlphaDogMoveTo::PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	FBTMoveToTaskMemory* MyMemory = CastInstanceNodeMemory<FBTMoveToTaskMemory>(NodeMemory);
	AADogPlayerBotController* MyController = Cast<AADogPlayerBotController>(OwnerComp.GetAIOwner());
	check(MyController);

	EBTNodeResult::Type NodeResult = EBTNodeResult::Failed;
	if (MyController && MyBlackboard)
	{
		FAIMoveRequest MoveReq;
		MoveReq.SetNavigationFilter(*FilterClass ? FilterClass : MyController->GetDefaultNavigationFilterClass());
		MoveReq.SetAllowPartialPath(bAllowPartialPath);
		MoveReq.SetAcceptanceRadius(AcceptableRadius);
		MoveReq.SetCanStrafe(bAllowStrafe);
		MoveReq.SetReachTestIncludesAgentRadius(bReachTestIncludesAgentRadius);
		MoveReq.SetReachTestIncludesGoalRadius(bReachTestIncludesGoalRadius);
		MoveReq.SetRequireNavigableEndLocation(bRequireNavigableEndLocation);
		MoveReq.SetProjectGoalLocation(bProjectGoalLocation);
		MoveReq.SetUsePathfinding(bUsePathfinding);

		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
		{
			AActor* TargetActor = MyController->GetMoveTarget();

			if (TargetActor)
			{
				if (bTrackMovingGoal)
				{
					MoveReq.SetGoalActor(TargetActor);
				}
				else
				{
					MoveReq.SetGoalLocation(TargetActor->GetActorLocation());
				}
			}
			else
			{
				UE_VLOG(MyController, LogBehaviorTree, Warning, TEXT("UBTTask_AlphaDogMoveTo::ExecuteTask tried to go to actor while BB %s entry was empty"), *BlackboardKey.SelectedKeyName.ToString());
			}
		}
		else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			const FVector TargetLocation = MyBlackboard->GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
			MoveReq.SetGoalLocation(TargetLocation);

			MyMemory->PreviousGoalLocation = TargetLocation;
		}

		if (MoveReq.IsValid())
		{
			UAITask_MoveTo* MoveTask = MyMemory->Task.Get();
			const bool bReuseExistingTask = (MoveTask != nullptr);

			MoveTask = PrepareMoveTask(OwnerComp, MoveTask, MoveReq);
			if (MoveTask)
			{
				MyMemory->bObserverCanFinishTask = false;

				if (bReuseExistingTask)
				{
					if (MoveTask->IsActive())
					{
						UE_VLOG(MyController, LogBehaviorTree, Verbose, TEXT("\'%s\' reusing AITask %s"), *GetNodeName(), *MoveTask->GetName());
						MoveTask->ConditionalPerformMove();
					}
					else
					{
						UE_VLOG(MyController, LogBehaviorTree, Verbose, TEXT("\'%s\' reusing AITask %s, but task is not active - handing over move performing to task mechanics"), *GetNodeName(), *MoveTask->GetName());
					}
				}
				else
				{
					MyMemory->Task = MoveTask;
					UE_VLOG(MyController, LogBehaviorTree, Verbose, TEXT("\'%s\' task implementing move with task %s"), *GetNodeName(), *MoveTask->GetName());
					MoveTask->ReadyForActivation();
				}

				MyMemory->bObserverCanFinishTask = true;
				NodeResult = (MoveTask->GetState() != EGameplayTaskState::Finished) ? EBTNodeResult::InProgress :
					MoveTask->WasMoveSuccessful() ? EBTNodeResult::Succeeded :
					EBTNodeResult::Failed;
			}
		}
	}

	return NodeResult;
}

EBTNodeResult::Type UBTTask_AlphaDogMoveTo::PerformMoveTargetTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	FBTADogMoveToTaskMemory* MyMemory = CastInstanceNodeMemory<FBTADogMoveToTaskMemory>(NodeMemory);
	AAIController* MyController = OwnerComp.GetAIOwner();

	EBTNodeResult::Type nodeResult = EBTNodeResult::Failed;
	if (MyController && MyBlackboard)
	{
		FUpdateMoveTargetRequest MoveTargetReq;
		MoveTargetReq.TurnRate = TurnRate;
		MoveTargetReq.TurnRateTimerLimit = TurnRateTimerLimit;
		MoveTargetReq.TurnRateThrottleDistance = TurnRateThrottleDistance;
		MoveTargetReq.TurnThrottleTrigger = TurnThrottleTrigger;
		MoveTargetReq.MoveTargetDistanceTime = MoveTargetDistanceSpeed;
		MoveTargetReq.MoveTargetDistanceMax = MoveTargetDistanceMax;
		MoveTargetReq.bDrawDebug = bDrawDebug;
		MoveTargetReq.TargetActor = Cast<AActor>(MyBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID()));
			
		UADogAITask_UpdateMoveTarget* moveTargetTask = MyMemory->MoveTargetTask.Get();
		const bool bReuseExistingTask = (moveTargetTask != nullptr);
		
		moveTargetTask = PrepareUpdateMoveTargetTask(OwnerComp, moveTargetTask, MoveTargetReq);
		if(moveTargetTask && moveTargetTask->bInitialized)
		{
			if (!bReuseExistingTask)
			{
				MyMemory->MoveTargetTask = moveTargetTask;
				moveTargetTask->ReadyForActivation();
				if (moveTargetTask->GetState() != EGameplayTaskState::Finished)
				{
					nodeResult = EBTNodeResult::InProgress;
				}
			}
		}
	}

	return nodeResult;
}

UADogAITask_UpdateMoveTarget* UBTTask_AlphaDogMoveTo::PrepareUpdateMoveTargetTask(UBehaviorTreeComponent& OwnerComp,
	UADogAITask_UpdateMoveTarget* ExistingTask, FUpdateMoveTargetRequest TaskRequest)
{
	UADogAITask_UpdateMoveTarget* moveTargetTask = ExistingTask ? ExistingTask : NewBTAITask<UADogAITask_UpdateMoveTarget>(OwnerComp);
	if (moveTargetTask)
	{
		AAIController* controller = moveTargetTask->GetAIController();
		const bool bTaskSetup = moveTargetTask->SetUp(controller, controller->GetPawn(),TaskRequest);
		check(bTaskSetup);
	}

	return moveTargetTask;
}
