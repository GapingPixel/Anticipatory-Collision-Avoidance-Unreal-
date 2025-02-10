// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "Animation/AnimTrace.h"
//#include "AnimSuiteTrace.generated.h"

#if ANIM_TRACE_ENABLED

struct FAnimationBaseContext;
struct FAnimNode_BlendSpacePlayerMatcher;

struct FAnimSuiteGraphRuntimeTrace
{
	/** Helper function to output debug info for blendspace player nodes */
	ANIMATIONMATCHINGSUITE_API static void OutputPoseMatchBlendSpacePlayer(const FAnimationBaseContext& InContext, const FAnimNode_BlendSpacePlayerMatcher& InNode);

	//@TODO: create another helper function for the BlendSpaceDMEvaluator if I reparent the class to FAnimNode_AssetPlayerBase.
};

#define TRACE_POSEMATCHBLENDSPACE_PLAYER(Context, Node) \
FAnimSuiteGraphRuntimeTrace::OutputPoseMatchBlendSpacePlayer(Context, Node);

#else

#define TRACE_POSEMATCHBLENDSPACE_PLAYER(Context, Node)

#endif