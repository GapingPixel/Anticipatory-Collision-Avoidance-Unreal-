// Copyright MuuKnighted Games 2024. All rights reserved.


#include "AnimGraph/AnimNode_RequestInertialization.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_Inertialization.h"

void FAnimNode_RequestInertialization::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread);

	Super::Initialize_AnyThread(Context);
	Source.Initialize(Context);

	bReinitialized = true;
}


void FAnimNode_RequestInertialization::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread);

	Super::CacheBones_AnyThread(Context);
	Source.CacheBones(Context);
}


void FAnimNode_RequestInertialization::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread);

	GetEvaluateGraphExposedInputs().Execute(Context);
	
	const bool bSkipForBecomingRelevant =
		GetSkipOnBecomingRelevant() &&
		UpdateCounter.HasEverBeenUpdated() &&
		!UpdateCounter.WasSynchronizedCounter(Context.AnimInstanceProxy->GetUpdateCounter());
	UpdateCounter.SynchronizeWith(Context.AnimInstanceProxy->GetUpdateCounter());

	SetRequestInertialization(GetRequestInertialization() && !bSkipForBecomingRelevant);

	if (GetRequestInertialization())
	{
		UE::Anim::IInertializationRequester* InertializationRequester = 
			Context.GetMessage<UE::Anim::IInertializationRequester>();
		if (InertializationRequester)
		{
			InertializationRequester->RequestInertialization(GetBlendTime());
			InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
		}
		else
		{
			FAnimNode_Inertialization::LogRequestError(Context, Source);
		}
	}

	Source.Update(Context);

	TRACE_ANIM_NODE_VALUE(Context, TEXT("Request Inertialization"), GetRequestInertialization());
}

void FAnimNode_RequestInertialization::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread);
	Source.Evaluate(Output);
}

void FAnimNode_RequestInertialization::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData);
	Source.GatherDebugData(DebugData);
}

bool FAnimNode_RequestInertialization::GetRequestInertialization() const
{
	return GET_ANIM_NODE_DATA(bool, bRequestInertialization);
}

bool FAnimNode_RequestInertialization::SetRequestInertialization(bool bInRequestInertialization)
{
#if WITH_EDITORONLY_DATA
	bRequestInertialization = bInRequestInertialization;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bRequestInertialization) = bInRequestInertialization;
#endif

	if (bool* bRequestInertializationPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bRequestInertialization))
	{
		*bRequestInertializationPtr = bInRequestInertialization;
		return true;
	}

	return false;
}

float FAnimNode_RequestInertialization::GetBlendTime() const
{
	return GET_ANIM_NODE_DATA(float, BlendTime);
}

bool FAnimNode_RequestInertialization::SetBlendTime(float InBlendTime)
{
#if WITH_EDITORONLY_DATA
	BlendTime = InBlendTime;
	GET_MUTABLE_ANIM_NODE_DATA(float, BlendTime) = InBlendTime;
#endif

	if (float* BlendTimePtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(float, BlendTime))
	{
		*BlendTimePtr = InBlendTime;
		return true;
	}

	return false;
}

bool FAnimNode_RequestInertialization::GetSkipOnBecomingRelevant() const
{
	return GET_ANIM_NODE_DATA(bool, bSkipOnBecomingRelevant);
}

bool FAnimNode_RequestInertialization::SetSkipOnBecomingRelevant(bool bInSkipOnBecomingRelevant)
{
#if WITH_EDITORONLY_DATA
	bSkipOnBecomingRelevant = bInSkipOnBecomingRelevant;
	GET_MUTABLE_ANIM_NODE_DATA(bool, bSkipOnBecomingRelevant) = bInSkipOnBecomingRelevant;
#endif

	if (bool* bSkipOnBecomingRelevantPtr = GET_INSTANCE_ANIM_NODE_DATA_PTR(bool, bSkipOnBecomingRelevant))
	{
		*bSkipOnBecomingRelevantPtr = bInSkipOnBecomingRelevant;
		return true;
	}

	return false;
}
