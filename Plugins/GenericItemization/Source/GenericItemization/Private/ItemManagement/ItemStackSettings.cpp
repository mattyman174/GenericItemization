// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "ItemManagement/ItemStackSettings.h"
#include "GenericItemizationInstanceTypes.h"
#include "StructView.h"

UItemStackSettings::UItemStackSettings()
{
	StackingRequirements = TInstancedStruct<FItemStackingRequirements>::Make();
}

bool UItemStackSettings::CanStackWith_Implementation(const FInstancedStruct& ItemToStackFrom, const FInstancedStruct& ItemToStackWith, int32& OutRemainder) const
{
	const FConstStructView ItemToStackFromView = FConstStructView(ItemToStackFrom);
	const FConstStructView ItemToStackWithView = FConstStructView(ItemToStackWith);
	const FItemInstance* const ItemToStackFromPtr = ItemToStackFromView.GetPtr<const FItemInstance>();
	const FItemInstance* const ItemToStackWithPtr = ItemToStackWithView.GetPtr<const FItemInstance>();
	if (!ItemToStackFromPtr || !ItemToStackWithPtr)
	{
		return false;
	}

	const TInstancedStruct<FItemDefinition>& ItemToStackFromDefinitionInstance = ItemToStackFromPtr->ItemDefinition;
	const TInstancedStruct<FItemDefinition>& ItemToStackWithDefinitionInstance = ItemToStackWithPtr->ItemDefinition;
	if (!ItemToStackFromDefinitionInstance.IsValid() || !ItemToStackWithDefinitionInstance.IsValid())
	{
		return false;
	}

	const FItemDefinition& ItemToStackFromDefinition = ItemToStackFromDefinitionInstance.Get();
	const FItemDefinition& ItemToStackWithDefinition = ItemToStackWithDefinitionInstance.Get();
	if (!ItemToStackFromDefinition.IsSameItemDefinition(ItemToStackWithDefinition))
	{
		return false;
	}

	const UItemStackSettings* const StackSettingsCDO = ItemToStackFromDefinition.StackSettings.GetDefaultObject();
	if (!StackSettingsCDO || !StackSettingsCDO->IsStackable())
	{
		return false;
	}

	// Check our requirements are met.
	if (!StackSettingsCDO->StackingRequirements.Get().bIgnoreQualityLevel)
	{
		if (ItemToStackFromDefinition.QualityLevel != ItemToStackWithDefinition.QualityLevel)
		{
			return false;
		}
	}

	if (!StackSettingsCDO->StackingRequirements.Get().bIgnoreItemLevel)
	{
		if (ItemToStackFromPtr->ItemLevel != ItemToStackWithPtr->ItemLevel)
		{
			return false;
		}
	}

	if (!StackSettingsCDO->StackingRequirements.Get().bIgnoreAffixLevel)
	{
		if (ItemToStackFromPtr->AffixLevel != ItemToStackWithPtr->AffixLevel)
		{
			return false;
		}
	}

	if (!StackSettingsCDO->StackingRequirements.Get().bIgnoreAffixes)
	{
		// Check if there are any Affixes that are not Predefined, if there are then we assume they cannot be reconciled.
		TArray<TInstancedStruct<FAffixInstance>> Affixes;
		Affixes.Append(ItemToStackFromPtr->Affixes);
		Affixes.Append(ItemToStackWithPtr->Affixes);

		for (const TInstancedStruct<FAffixInstance>& Affix : Affixes)
		{
			if (!Affix.Get().bPredefinedAffix)
			{
				return false;
			}
		}
	}

	const FGameplayTagContainer& QualityTypes = StackSettingsCDO->StackingRequirements.Get().DoesNotStackWithQualityTypes;
	if (!QualityTypes.IsEmpty())
	{
		const FGameplayTag& QualityType = ItemToStackFromPtr->QualityType;
		if (QualityType.MatchesAny(QualityTypes))
		{
			return false;
		}
	}

	if(!StackSettingsCDO->HasUnlimitedStacks())
	{
		// Calculate the remainder.
		const int32 MaxStackSize = StackSettingsCDO->GetStackLimit();
		const int32 StackSize = ItemToStackWithPtr->StackCount + ItemToStackFromPtr->StackCount;
		if (StackSize > MaxStackSize)
		{
			if (ItemToStackWithPtr->StackCount >= MaxStackSize)
			{
				OutRemainder = ItemToStackFromPtr->StackCount;
			}
			else
			{
				OutRemainder = FMath::Abs((ItemToStackWithPtr->StackCount - MaxStackSize) + ItemToStackFromPtr->StackCount);
			}
		}
		else
		{
			OutRemainder = 0;
		}
	}
	else
	{
		OutRemainder = 0;
	}

	return true;
}
