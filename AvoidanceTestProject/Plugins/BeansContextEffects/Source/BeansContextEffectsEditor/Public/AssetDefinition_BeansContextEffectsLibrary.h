
#pragma once

#include "AssetDefinitionDefault.h"
#include "BeansContextEffectsLibrary.h"

#include "AssetDefinition_BeansContextEffectsLibrary.generated.h"

UCLASS()
class UAssetDefinition_BeansContextEffectsLibrary : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AlphaDogContextEffectsLibrary", "AlphaDogContextEffectsLibrary"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(65, 200, 98)); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UBeansContextEffectsLibrary::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { EAssetCategoryPaths::Gameplay };
		return Categories;
	}
	// UAssetDefinition End
};
