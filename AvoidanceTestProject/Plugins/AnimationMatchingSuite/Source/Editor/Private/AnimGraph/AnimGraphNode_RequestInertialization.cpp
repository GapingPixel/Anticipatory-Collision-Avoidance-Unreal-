// Copyright MuuKnighted Games 2024. All rights reserved.


#include "AnimGraph/AnimGraphNode_RequestInertialization.h"
#include "Animation/AnimNode_Inertialization.h"

#define LOCTEXT_NAMESPACE "AnimationSuiteNodes"

FLinearColor UAnimGraphNode_RequestInertialization::GetNodeTitleColor() const
{
	return FLinearColor(0.0f, 0.56f, 1.0f);
}

FText UAnimGraphNode_RequestInertialization::GetTooltipText() const
{
	return LOCTEXT("NodeToolTip", "Inertialization");
}

FText UAnimGraphNode_RequestInertialization::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Inertialization");
}

FText UAnimGraphNode_RequestInertialization::GetMenuCategory() const
{
	return LOCTEXT("NodeCategory", "Animation Matching Suite");
}

FString UAnimGraphNode_RequestInertialization::GetNodeCategory() const
{
	return FString("Animation Matching Suite");
}

void UAnimGraphNode_RequestInertialization::GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const
{
	OutAttributes.Add(UE::Anim::IInertializationRequester::Attribute);
}

#undef LOCTEXT_NAMESPACE