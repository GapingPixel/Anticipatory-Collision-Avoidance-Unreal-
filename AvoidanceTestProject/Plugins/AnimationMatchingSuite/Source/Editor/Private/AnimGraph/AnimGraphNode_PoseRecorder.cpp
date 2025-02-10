// Copyright MuuKnighted Games 2024. All rights reserved.

#include "AnimGraph/AnimGraphNode_PoseRecorder.h"

#define LOCTEXT_NAMESPACE "AnimationSuiteNodes"

FLinearColor UAnimGraphNode_PoseRecorder::GetNodeTitleColor() const
{
	return FLinearColor(0.0f, 0.56f, 1.0f);
}

FText UAnimGraphNode_PoseRecorder::GetTooltipText() const
{
	return LOCTEXT("NodeToolTip", "Pose Grabber");
}

FText UAnimGraphNode_PoseRecorder::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Pose Grabber");
}

FText UAnimGraphNode_PoseRecorder::GetMenuCategory() const
{
	return LOCTEXT("NodeCategory", "Animation Matching Suite");
}

FString UAnimGraphNode_PoseRecorder::GetNodeCategory() const
{
	return FString("Animation Matching Suite");
}

#undef LOCTEXT_NAMESPACE
