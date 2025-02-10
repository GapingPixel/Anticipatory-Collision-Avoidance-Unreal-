// Copyright MuuKnighted Games 2024. All rights reserved.

#include "AnimGraph/AnimGraphNode_BlendSpacePlayerMatcher.h"

#include "AnimGraphCommands.h"
#include "DetailLayoutBuilder.h"
#include "IAnimBlueprintNodeOverrideAssetsContext.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "Animation/AimOffsetBlendSpace1D.h"
#include "Animation/AnimRootMotionProvider.h"
#include "Animation/BlendSpace1D.h"

#define LOCTEXT_NAMESPACE "AnimationMatchingSuiteNodes"

UAnimGraphNode_BlendSpacePlayerMatcher::UAnimGraphNode_BlendSpacePlayerMatcher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_BlendSpacePlayerMatcher::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UEdGraphPin* BlendSpacePin = FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_BlendSpacePlayerMatcher, BlendSpace));
	return GetNodeTitleHelper(TitleType, BlendSpacePin, LOCTEXT("PlayerDesc", "Blendspace Player (Matching)"));
}

void UAnimGraphNode_BlendSpacePlayerMatcher::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	GetMenuActionsHelper(
		ActionRegistrar,
		GetClass(),
		{ UBlendSpace::StaticClass() }, 
		{ },
		[](const FAssetData& InAssetData, UClass* InClass)
		{
			if(InAssetData.IsValid())
			{
				return FText::Format(LOCTEXT("MenuDescFormat", "Blendspace Player (Matching) '{0}'"), FText::FromName(InAssetData.AssetName));
			}
			else
			{
				return LOCTEXT("MenuDesc", "Blendspace Player (Matching)");
			}
		},
		[](const FAssetData& InAssetData, UClass* InClass)
		{
			if(InAssetData.IsValid())
			{
				return FText::Format(LOCTEXT("MenuDescTooltipFormat", "Blendspace Player (Matching)\n'{0}'"), FText::FromString(InAssetData.GetObjectPathString()));
			}
			else
			{
				return LOCTEXT("MenuDescTooltip", "Blendspace Player (Matching)");
			}
		},
		[](UEdGraphNode* InNewNode, bool bInIsTemplateNode, const FAssetData InAssetData)
		{
			UAnimGraphNode_AssetPlayerBase::SetupNewNode(InNewNode, bInIsTemplateNode, InAssetData);
		});	
}

void UAnimGraphNode_BlendSpacePlayerMatcher::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton,
	FCompilerResultsLog& MessageLog)
{
	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);

	// UE_LOG(LogTemp, Warning, TEXT("%p"), FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_BlendSpaceDMEvaluator,
	// 		BlendSpace)));
	//
	// UE_LOG(LogTemp, Warning, TEXT("%p"), Node.GetBlendSpace());
	
	ValidateAnimNodeDuringCompilationHelper(ForSkeleton, MessageLog, Node.GetBlendSpace(),
		UBlendSpace::StaticClass(), FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_BlendSpacePlayerMatcher,
			BlendSpace)), GET_MEMBER_NAME_CHECKED(FAnimNode_BlendSpacePlayerMatcher, BlendSpace));
}

FLinearColor UAnimGraphNode_BlendSpacePlayerMatcher::GetNodeTitleColor() const
{
	return FLinearColor(0.0f, 0.56f, 1.0f);
}

FSlateIcon UAnimGraphNode_BlendSpacePlayerMatcher::GetIconAndTint(FLinearColor& OutColor) const
{
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.BlendSpace");
}

void UAnimGraphNode_BlendSpacePlayerMatcher::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	const TObjectPtr<UBlendSpace> BlendSpace = Node.GetBlendSpace();

	if (BlendSpace != NULL)
	{
		if (SourcePropertyName == TEXT("X"))
		{
			Pin->PinFriendlyName = FText::FromString(BlendSpace->GetBlendParameter(0).DisplayName);
		}
		else if (SourcePropertyName == TEXT("Y"))
		{
			Pin->PinFriendlyName = FText::FromString(BlendSpace->GetBlendParameter(1).DisplayName);
			Pin->bHidden = BlendSpace->IsA<UBlendSpace1D>() ? 1 : 0;
		}
		else if (SourcePropertyName == TEXT("Z"))
		{
			Pin->PinFriendlyName = FText::FromString(BlendSpace->GetBlendParameter(2).DisplayName);
		}
	}
}


void UAnimGraphNode_BlendSpacePlayerMatcher::PreloadRequiredAssets()
{
	PreloadObject(Node.GetBlendSpace());
	Super::PreloadRequiredAssets();
}

void UAnimGraphNode_BlendSpacePlayerMatcher::PostProcessPinName(const UEdGraphPin* Pin, FString& DisplayName) const
{
	if(Pin->Direction == EGPD_Input)
	{
		const TObjectPtr<UBlendSpace> BlendSpace = Node.GetBlendSpace();

		if(BlendSpace != NULL)
		{
			if(Pin->PinName == TEXT("X"))
			{
				DisplayName = BlendSpace->GetBlendParameter(0).DisplayName;
			}
			else if(Pin->PinName == TEXT("Y"))
			{
				DisplayName = BlendSpace->GetBlendParameter(1).DisplayName;
			}
			else if(Pin->PinName == TEXT("Z"))
			{
				DisplayName = BlendSpace->GetBlendParameter(2).DisplayName;
			}
		}
	}

	Super::PostProcessPinName(Pin, DisplayName);
}

void UAnimGraphNode_BlendSpacePlayerMatcher::GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const
{
	Super::GetOutputLinkAttributes(OutAttributes);

	if (UE::Anim::IAnimRootMotionProvider::Get())
	{
		OutAttributes.Add(UE::Anim::IAnimRootMotionProvider::AttributeName);
	}
}

void UAnimGraphNode_BlendSpacePlayerMatcher::CustomizeDetails(IDetailLayoutBuilder& InDetailBuilder)
{
	const TObjectPtr<UBlendSpace> BlendSpace = Node.GetBlendSpace(); 
	if (BlendSpace)
	{
		TSharedRef<IPropertyHandle> XHandle = InDetailBuilder.GetProperty(TEXT("Node.X"), GetClass());
		XHandle->SetPropertyDisplayName(FText::FromString(BlendSpace->GetBlendParameter(0).DisplayName));
		TSharedRef<IPropertyHandle> YHandle = InDetailBuilder.GetProperty(TEXT("Node.Y"), GetClass());
		if (BlendSpace->IsA<UBlendSpace1D>())
		{
			InDetailBuilder.HideProperty(YHandle);
		}
		else
		{
			YHandle->SetPropertyDisplayName(FText::FromString(BlendSpace->GetBlendParameter(1).DisplayName));
		}
	}
}

void UAnimGraphNode_BlendSpacePlayerMatcher::GetNodeContextMenuActions(UToolMenu* Menu,
	UGraphNodeContextMenuContext* Context) const
{
	if (!Context->bIsDebugging)
	{
		// add an option to convert to single frame
		{
			FToolMenuSection& Section = Menu->AddSection("AnimGraphNodeBlendSpacePlayerMatcher", LOCTEXT("BlendSpaceHeading", "Blend Space"));
			Section.AddMenuEntry(FAnimGraphCommands::Get().OpenRelatedAsset);
			//Section.AddMenuEntry(FAnimGraphCommands::Get().ConvertToBSGraph);
		}
	}
}

FBlueprintNodeSignature UAnimGraphNode_BlendSpacePlayerMatcher::GetSignature() const
{
	FBlueprintNodeSignature NodeSignature = Super::GetSignature();
	NodeSignature.AddSubObject(Node.GetBlendSpace());

	return NodeSignature;
}

void UAnimGraphNode_BlendSpacePlayerMatcher::SetAnimationAsset(UAnimationAsset* Asset)
{
	if (UBlendSpace* BlendSpace = Cast<UBlendSpace>(Asset))
	{
		Node.SetBlendSpace(BlendSpace);
	}
}

void UAnimGraphNode_BlendSpacePlayerMatcher::CopySettingsFromAnimationAsset(UAnimationAsset* Asset)
{
	//Super::CopySettingsFromAnimationAsset(Asset);
	if (UBlendSpace* BlendSpace = Cast<UBlendSpace>(Asset))
	{
		Node.SetLoop(BlendSpace->bLoop);
	}
}

FText UAnimGraphNode_BlendSpacePlayerMatcher::GetMenuCategory() const
{
	//return LOCTEXT("BlendSpaceCategory_Label", "Animation Matching Suite");
	return LOCTEXT("NodeCategory", "Animation Matching Suite");
}

FString UAnimGraphNode_BlendSpacePlayerMatcher::GetNodeCategory() const
{
	return FString("Animation Matching Suite");
}


void UAnimGraphNode_BlendSpacePlayerMatcher::BakeDataDuringCompilation(FCompilerResultsLog& MessageLog)
{
	UAnimBlueprint* AnimBlueprint = GetAnimBlueprint();
	AnimBlueprint->FindOrAddGroup(Node.GetGroupName());
}

bool UAnimGraphNode_BlendSpacePlayerMatcher::DoesSupportTimeForTransitionGetter() const
{
	return true;
}

UAnimationAsset* UAnimGraphNode_BlendSpacePlayerMatcher::GetAnimationAsset() const
{
	UBlendSpace* CurrentBlendSpace = Node.GetBlendSpace();
	UEdGraphPin* BlendSpacePin = FindPin(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_BlendSpacePlayerMatcher, BlendSpace));
	if (BlendSpacePin != nullptr && CurrentBlendSpace == nullptr)
	{
		CurrentBlendSpace = Cast<UBlendSpace>(BlendSpacePin->DefaultObject);
	}

	return CurrentBlendSpace;
}

const TCHAR* UAnimGraphNode_BlendSpacePlayerMatcher::GetTimePropertyName() const
{
	return TEXT("InternalTimeAccumulator");
}

UScriptStruct* UAnimGraphNode_BlendSpacePlayerMatcher::GetTimePropertyStruct() const
{
	return FAnimNode_BlendSpacePlayerMatcher::StaticStruct();
}

void UAnimGraphNode_BlendSpacePlayerMatcher::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets) const
{
	if(Node.GetBlendSpace())
	{
		HandleAnimReferenceCollection(Node.BlendSpace, AnimationAssets); 
	}
}

void UAnimGraphNode_BlendSpacePlayerMatcher::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& AnimAssetReplacementMap)
{
	HandleAnimReferenceReplacement(Node.BlendSpace, AnimAssetReplacementMap);
}

EAnimAssetHandlerType UAnimGraphNode_BlendSpacePlayerMatcher::SupportsAssetClass(const UClass* AssetClass) const
{
	if (AssetClass->IsChildOf(UBlendSpace::StaticClass()) && !IsAimOffsetBlendSpace(AssetClass))
	{
		return EAnimAssetHandlerType::Supported;
	}
	else
	{
		return EAnimAssetHandlerType::NotSupported;
	}
}

void UAnimGraphNode_BlendSpacePlayerMatcher::OnOverrideAssets(IAnimBlueprintNodeOverrideAssetsContext& InContext) const
{
	if(InContext.GetAssets().Num() > 0)
	{
		if (UBlendSpace* BlendSpace = Cast<UBlendSpace>(InContext.GetAssets()[0]))
		{
			FAnimNode_BlendSpacePlayerMatcher& AnimNode = InContext.GetAnimNode<FAnimNode_BlendSpacePlayerMatcher>();
			AnimNode.SetBlendSpace(BlendSpace);
		}
	}
}

bool UAnimGraphNode_BlendSpacePlayerMatcher::IsAimOffsetBlendSpace(const UClass* BlendSpaceClass)
{
	return  BlendSpaceClass->IsChildOf(UAimOffsetBlendSpace::StaticClass()) ||
		BlendSpaceClass->IsChildOf(UAimOffsetBlendSpace1D::StaticClass());
}

#undef LOCTEXT_NAMESPACE