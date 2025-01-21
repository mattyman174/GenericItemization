// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "GenericItemizationInstancingFunctions.h"
#include "StructView.h"
#include "GenericItemizationTypes.h"
#include "GenericItemizationInstanceTypes.h"
#include "GenericItemizationPickFunctions.h"
#include "GameplayTagsManager.h"
#include "ItemManagement/ItemSocketSettings.h"

UItemInstancingFunction::UItemInstancingFunction()
{
	AffixPickFunction = UAffixPickFunction::StaticClass();
}

void UItemInstancingFunction::MakeItemInstance(const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItemInstance) const
{
	// Override this if you want a different base.
	OutItemInstance = FInstancedStruct::Make<FItemInstance>();
}

void UItemInstancingFunction::MakeAffixInstance(const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutAffixInstance) const
{
	// Override this if you want a different base.
	OutAffixInstance = FInstancedStruct::Make<FAffixInstance>();
}

bool UItemInstancingFunction::CalculateAffixLevel_Implementation(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, int32& OutAffixLevel) const
{
	const FConstStructView ItemInstanceView = FConstStructView(ItemInstance);
	const FItemInstance* const ItemInstancePtr = ItemInstanceView.GetPtr<const FItemInstance>();
	if (!ItemInstancePtr)
	{
		return false;
	}

	const TInstancedStruct<FItemDefinition>& ItemDefinition = ItemInstancePtr->GetItemDefinition();
	if (!ItemDefinition.IsValid())
	{
		return false;
	}

	int32 ItemLevel = ItemInstancePtr->ItemLevel;
	ItemLevel = (ItemLevel > MaximumItemLevel) ? MaximumItemLevel : ItemLevel; // Clamp ItemLevel to our Maximum ItemLevel.

	const int32 QualityLevel = ItemDefinition.Get().QualityLevel;
	ItemLevel = (QualityLevel > ItemLevel) ? QualityLevel : ItemLevel; // Clamp ItemLevel to our QualityLevel.

	int32 AffixLevel = MaximumItemLevel;

	if (ItemLevel < (MaximumItemLevel - QualityLevel / 2))
	{
		AffixLevel = ItemLevel - QualityLevel / 2;
	}
	else
	{
		AffixLevel = 2 * ItemLevel - MaximumItemLevel;
	}

	OutAffixLevel = (AffixLevel > MaximumItemLevel) ? MaximumItemLevel : AffixLevel; // Clamp AffixLevel to our Maximum ItemLevel.
	return true;
}

bool UItemInstancingFunction::SelectItemQualityType_Implementation(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, FGameplayTag& OutQualityType) const
{
	const FItemInstance* ItemInstancePtr = ItemInstance.GetPtr<FItemInstance>();
	if (!ItemInstancePtr)
	{
		return false;
	}

	const FConstStructView ItemInstancingContextView = FConstStructView(ItemInstancingContext);
	const FItemInstancingContext* const ItemInstancingContextPtr = ItemInstancingContextView.GetPtr<const FItemInstancingContext>();
	if (!ItemInstancingContextPtr)
	{
		return false;
	}

	const TInstancedStruct<FItemDefinition>& ItemDefinition = ItemInstancePtr->GetItemDefinition();
	if (!ItemDefinition.IsValid())
	{
		return false;
	}

	// If the ItemDefinition has a predefined QualityType, we can early out and avoid all the work below.
	if (ItemDefinition.Get().bHasPredefinedQualityType)
	{
		OutQualityType = ItemDefinition.Get().PredefinedQualityType;
		return true;
	}

	const FItemQualityRatioTypesTableEntry* ItemQualityRatioTypesTableEntryPtr = ItemDefinition.Get().QualityTypeRatio.GetRow<FItemQualityRatioTypesTableEntry>(FString());
	if (!IsValid(ItemDefinition.Get().QualityTypeRatio.DataTable)
		|| !ItemDefinition.Get().QualityTypeRatio.DataTable->GetRowStruct()->IsChildOf(FItemQualityRatioTypesTableEntry::StaticStruct())
		|| !ItemQualityRatioTypesTableEntryPtr)
	{
		return false;
	}

	const TArray<TInstancedStruct<FItemQualityRatioType>>& ItemQualityRatios = ItemQualityRatioTypesTableEntryPtr->ItemQualityRatios;

	// Iterate through each QualityType and perform a selection test to find out our desired QualityType.
	// This determines the number and type of Affixes that we can select for.
	const int32 ItemLevel = ItemInstancePtr->ItemLevel;
	const int32 QualityLevel = ItemDefinition.Get().QualityLevel;
	const int32 MagicFind = ItemInstancingContextPtr->MagicFind;

	for (const TInstancedStruct<FItemQualityRatioType>& ItemQualityRatio : ItemQualityRatios)
	{
		if (ItemQualityRatio.IsValid())
		{
			const FItemQualityRatioType& ItemQualityRatioType = ItemQualityRatio.Get();
			const FItemQualityTypeBonuses* QualityTypeBonusesPtr = ItemInstancingContextPtr->DropTable->QualityTypeBonuses.Find(ItemQualityRatioType.QualityType);
			const int32 DropTableQualityFactor = QualityTypeBonusesPtr ? QualityTypeBonusesPtr->AdjustedFactor : 0;
			const int32 DiminishingReturnsFactor = ItemQualityRatioType.Factor;

			// Seed the PickChance from the Ratio Type we are currently iterating.
			int32 PickChance = (ItemQualityRatioType.Base - ((ItemLevel - QualityLevel) / ItemQualityRatioType.Divisor)) * 128;

			// Calculate the Magic Find.
			const int32 EffectiveMagicFind = (DiminishingReturnsFactor > 0) ? (MagicFind * DiminishingReturnsFactor / (MagicFind + DiminishingReturnsFactor)) : MagicFind;

			// Integrate the Effective Magic Find.
			PickChance = PickChance * 100 / (100 + EffectiveMagicFind);

			// Calculate the Final Pick Chance at this Quality Type.
			int32 FinalPickChance = 0;
			if (DropTableQualityFactor > 0)
			{
				FinalPickChance = PickChance - (PickChance * DropTableQualityFactor / 1024);
			}
			else
			{
				FinalPickChance = PickChance - (PickChance / 1024);
			}

			// Decide if this Quality Type is the one we want.
			FinalPickChance = ItemInstancePtr->ItemStream.RandHelper(FMath::Clamp(FinalPickChance, 0, FinalPickChance));
			if (FinalPickChance < 128)
			{
				// This Quality Type is successfully selected.
				OutQualityType = ItemQualityRatioType.QualityType;
				return true;
			}
		}
	}

	// If we couldn't make a selection randomly. Select the last element by default.
	const FItemQualityRatioType& ItemQualityRatioType = ItemQualityRatios.Last().Get();
	if (!ItemQualityRatioType.QualityType.IsValid())
	{
		return false;
	}

	OutQualityType = ItemQualityRatioType.QualityType;
	return true;
}

bool UItemInstancingFunction::DetermineAffixCount_Implementation(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, int32& OutAffixCount) const
{
	const FItemInstance* ItemInstancePtr = ItemInstance.GetPtr<FItemInstance>();
	if (!ItemInstancePtr)
	{
		return false;
	}

	const TInstancedStruct<FItemDefinition>& ItemDefinition = ItemInstancePtr->GetItemDefinition();
	if (!ItemDefinition.IsValid())
	{
		return false;
	}

	const FItemAffixCountRatiosTableEntry* ItemAffixCountRatioTableEntryPtr = ItemDefinition.Get().AffixCountRatio.GetRow<FItemAffixCountRatiosTableEntry>(FString());
	if (!IsValid(ItemDefinition.Get().AffixCountRatio.DataTable)
		|| !ItemDefinition.Get().AffixCountRatio.DataTable->GetRowStruct()->IsChildOf(FItemAffixCountRatiosTableEntry::StaticStruct())
		|| !ItemAffixCountRatioTableEntryPtr)
	{
		// If there was no entry, don't fail, just assume it wants no Affixes.
		OutAffixCount = 0;
		return true;
	}

	const FGameplayTag& ItemQualityType = ItemInstancePtr->QualityType;
	for (const TInstancedStruct<FItemAffixCountRatioType>& AffixCountRatio : ItemAffixCountRatioTableEntryPtr->AffixCountRatios)
	{
		if (AffixCountRatio.IsValid() && AffixCountRatio.Get().QualityType == ItemQualityType)
		{
			OutAffixCount = ItemInstancePtr->ItemStream.RandRange(AffixCountRatio.Get().Minimum, AffixCountRatio.Get().Maximum);
			return true;
		}
	}

	// If there was no entry, don't fail, just assume it wants no Affixes.
	OutAffixCount = 0;
	return true;
}

bool UItemInstancingFunction::CalculateStackCount_Implementation(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, int32& OutStackCount) const
{
	// This function is designed to be overridden, as the default implementation for ItemInstance stacking is not enabled.
	// Individual Items should manage their own algorithm for what their StackCount should be.
	// The default value for StackCount will remain at 1. It should always be >= 1.
	OutStackCount = 1;
	return true;
}

bool UItemInstancingFunction::DetermineActiveSockets_Implementation(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, TArray<int32>& OutActiveSocketDefinitions) const
{
	const FItemInstance* ItemInstancePtr = ItemInstance.GetPtr<FItemInstance>();
	if (!ItemInstancePtr)
	{
		return false;
	}

	const TInstancedStruct<FItemDefinition>& ItemDefinitionInstance = ItemInstancePtr->GetItemDefinition();
	if (!ItemDefinitionInstance.IsValid())
	{
		return false;
	}

	const FItemDefinition& ItemDefinition = ItemDefinitionInstance.Get();
	if (ItemDefinition.bStacksOverSockets)
	{
		// If we are using Stacking functionality instead of Socketing functionality, then we can exit early with no default Active Sockets.
		OutActiveSocketDefinitions.Empty();
		return true;
	}

	const UItemSocketSettings* const ItemSocketSettingsCDO = ItemDefinition.SocketSettings.GetDefaultObject();
	if (!ItemSocketSettingsCDO)
	{
		// This isn't necessarily a failure case.
		OutActiveSocketDefinitions.Empty();
		return true;
	}

	// By default we let the ItemSocketSettings decide what to do.
	return ItemSocketSettingsCDO->DetermineActiveSockets(ItemInstance, ItemInstancingContext, OutActiveSocketDefinitions);
}

bool UItemInstancingContextFunction::BuildItemInstancingContext_Implementation(const UItemInstancer* ItemInstancer, const FInstancedStruct& UserContextData, FInstancedStruct& OutItemInstancingContext)
{
	// Override to provide your own context. 
	// You may want to pass data important to you through the UserContextData and embed it in the Item Instancing Context.
	// For interpretation elsewhere in the Item Instancing Process or other external systems.
	OutItemInstancingContext = FInstancedStruct::Make<FItemInstancingContext>();
	return true;
}
