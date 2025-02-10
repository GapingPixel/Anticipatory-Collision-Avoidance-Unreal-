// Copyright MuuKnighted Games 2024. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "AnimGraphNode_AssetPlayerBase.h"
#include "AnimGraph/AnimNode_BlendSpacePlayerMatcher.h"
#include "AnimGraphNode_BlendSpacePlayerMatcher.generated.h"

UCLASS()
class ANIMATIONMATCHINGSUITEEDITOR_API UAnimGraphNode_BlendSpacePlayerMatcher : public UAnimGraphNode_AssetPlayerBase
{
	GENERATED_BODY()

	UAnimGraphNode_BlendSpacePlayerMatcher(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	FAnimNode_BlendSpacePlayerMatcher Node;
	
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FText GetMenuCategory() const override;
	virtual FString GetNodeCategory() const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	//--- Blendspace evaluator
	virtual void ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog) override;
	virtual void BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog) override;
	// Interface to support transition getter
	virtual bool DoesSupportTimeForTransitionGetter() const override;
	virtual UAnimationAsset* GetAnimationAsset() const override;
	virtual const TCHAR* GetTimePropertyName() const override;
	virtual UScriptStruct* GetTimePropertyStruct() const override;
	virtual void GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets) const override;
	virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& AnimAssetReplacementMap) override;
	virtual EAnimAssetHandlerType SupportsAssetClass(const UClass* AssetClass) const override;
	virtual void OnOverrideAssets(IAnimBlueprintNodeOverrideAssetsContext& InContext) const override;
	//--- Blendspace base
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	virtual void PreloadRequiredAssets() override;
	virtual void PostProcessPinName(const UEdGraphPin* Pin, FString& DisplayName) const override;
	virtual void GetOutputLinkAttributes(FNodeAttributeArray& OutAttributes) const;
	virtual TSubclassOf<UAnimationAsset> GetAnimationAssetClass() const { return UBlendSpace::StaticClass(); }
	virtual void CustomizeDetails(IDetailLayoutBuilder& InDetailBuilder) override;
	// End of UAnimGraphNode_Base interface

	// UK2Node interface
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual FBlueprintNodeSignature GetSignature() const override;
	// End of UK2Node interface
	
	// UAnimGraphNode_AssetPlayerBase interface
	virtual void SetAnimationAsset(UAnimationAsset* Asset) override;
	virtual void CopySettingsFromAnimationAsset(UAnimationAsset* Asset) override;
	// End of UAnimGraphNode_AssetPlayerBase interface
	
	protected:
	//UBlendSpace* GetBlendSpace() const { return Cast<UBlendSpace>(GetAnimationAsset()); }
	
	
	/** Util to determine is an asset class is an aim offset */
	static bool IsAimOffsetBlendSpace(const UClass* BlendSpaceClass);
	
};
