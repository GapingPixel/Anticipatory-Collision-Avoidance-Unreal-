// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNodeMessages.h"
#include "AnimNode_RequestInertialization.generated.h"

/**
 * A custom inertialization node that can be toggled on and off via a boolean.
 */
USTRUCT(BlueprintInternalUseOnly)
struct ANIMATIONMATCHINGSUITE_API FAnimNode_RequestInertialization : public FAnimNode_Base
{
	GENERATED_BODY()

public:

	// ==== FAnimNode_Base
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	
	// ==== Custom functions
	virtual bool GetRequestInertialization() const;
	virtual bool SetRequestInertialization(bool bInRequestInertialization);
	virtual float GetBlendTime() const;
	virtual bool SetBlendTime(float InBlendTime);
	virtual bool GetSkipOnBecomingRelevant() const;
	virtual bool SetSkipOnBecomingRelevant(bool bInSkipOnBecomingRelevant);
	
public:

	/** The input source pose. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Links")
	FPoseLink Source;

private:
#if WITH_EDITORONLY_DATA

	/** Indicates whether inertialization should be requested. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta=(FoldProperty, PinShownByDefault))
	bool bRequestInertialization = false;

	/** The blend time over which to apply inertialization.*/
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (FoldProperty, PinShownByDefault))
	float BlendTime = 0.2f;

	/** Indicates whether to skip inertialization on the frame this node becomes relevant. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (FoldProperty, PinHiddenByDefault))
	bool bSkipOnBecomingRelevant = false;
	
#endif

protected:
	bool bLastInput = false;
	bool bReinitialized = false;
	
	FGraphTraversalCounter UpdateCounter;
};
