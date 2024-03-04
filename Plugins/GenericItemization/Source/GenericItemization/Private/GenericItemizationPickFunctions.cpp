// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "GenericItemizationPickFunctions.h"
#include "Engine/DataTable.h"
#include "GenericItemizationInstanceTypes.h"
#include "GenericItemizationInstancingFunctions.h"

/************************************************************************/
/* Items
/************************************************************************/

bool UItemPickFunction::PickItem_Implementation(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItem) const
{
	return false;
}

bool UItemDropTableCollectionPickFunction::PickItem_Implementation(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItem) const
{
	const FItemDropTableCollectionEntry* DropTableCollection = ItemDropTableCollectionEntry.GetRow<FItemDropTableCollectionEntry>(FString());
	if (!IsValid(ItemDropTableCollectionEntry.DataTable)
		|| !ItemDropTableCollectionEntry.DataTable->GetRowStruct()->IsChildOf(FItemDropTableCollectionEntry::StaticStruct())
		|| !DropTableCollection)
	{
		return false;
	}

	// Seed all of the Picks from the DropTableCollection.
	using FItemDropTablePickEntry = FPickEntry<TInstancedStruct<FItemDropTableType>>;
	TArray<FItemDropTablePickEntry> PickEntries;
	PickEntries.Reserve(DropTableCollection->ItemDropTables.Num() + static_cast<int32>(bIncludeNoPick));

	// Create and add our NoPick Entry first if necessary.
	if (bIncludeNoPick)
	{
		FItemDropTablePickEntry NoPick;
		NoPick.PickChance = DropTableCollection->NoPickChance;
		PickEntries.Add(NoPick);
	}

	// Add all the entries from the collection.
	for (const TInstancedStruct<FItemDropTableType>& ItemDropTable : DropTableCollection->ItemDropTables)
	{
		if (ItemDropTable.IsValid())
		{
			FInstancedStruct InstancedItemDropTable = FInstancedStruct::Make<FItemDropTableType>();
			InstancedItemDropTable.InitializeAs(ItemDropTable.GetScriptStruct(), ItemDropTable.GetMemory());
			if (DoesItemDropTableCollectionSatisfyPickRequirements(PickRequirements, ItemInstancingContext, InstancedItemDropTable))
			{
				const FItemDropTableType& DropTable = ItemDropTable.Get();
				FItemDropTablePickEntry PickEntry;
				PickEntry.PickChance = DropTable.PickChance;
				PickEntry.PickType = ItemDropTable;
				PickEntries.Add(PickEntry);
			}
		}
	}

	// @TODO: We need to make this generic so it can be used by any Pick type code and overridden if necessary to implement other algorithms.
	auto GetRandomEntry = [&PickEntries](FItemDropTablePickEntry& OutEntry)
	{
		float TotalPickChance = 0.f;
		float CurrentPickChance = 0.f;

		for (const FItemDropTablePickEntry& PickEntry : PickEntries)
		{
			TotalPickChance += PickEntry.PickChance;
		}

		CurrentPickChance = FMath::RandRange(0.f, TotalPickChance);

		for (const FItemDropTablePickEntry& PickEntry : PickEntries)
		{
			const float EntryPickChance = PickEntry.PickChance;
			if (CurrentPickChance < EntryPickChance)
			{
				OutEntry = PickEntry;
				return;
			}
			else
			{
				CurrentPickChance -= EntryPickChance;
			}
		}
	};

	// Pick an Entry.
	FItemDropTablePickEntry RandomPick;
	GetRandomEntry(RandomPick);
	if (RandomPick.PickType.IsSet())
	{
		OutItem = FInstancedStruct::Make<FItemDropTableType>();
		OutItem.InitializeAs(RandomPick.PickType.GetValue().GetScriptStruct(), RandomPick.PickType.GetValue().GetMemory());
		return true;
	}

	return false;
}

bool UItemDropTableCollectionPickFunction::DoesItemDropTableCollectionSatisfyPickRequirements_Implementation(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, const FInstancedStruct& ItemDropTableCollection) const
{
	// ItemDropTableCollections don't have any PickRequirements by default.
	return true;
}

bool UItemDefinitionCollectionPickFunction::PickItem_Implementation(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItem) const
{
	if (!IsValid(ItemDefinitions) || !ItemDefinitions->GetRowStruct()->IsChildOf(FItemDefinitionEntry::StaticStruct()))
	{
		return false;
	}

	const FString Context;
	TArray<FItemDefinitionEntry*> ItemDefinitionEntries;
	ItemDefinitions->GetAllRows(Context, ItemDefinitionEntries);

	// Seed all of the Picks we will make a selection from.
	using FItemPickEntry = FPickEntry<TInstancedStruct<FItemDefinition>>;
	TArray<FItemPickEntry> PickEntries;
	PickEntries.Reserve(ItemDefinitionEntries.Num());

	for (const FItemDefinitionEntry* ItemDefinition : ItemDefinitionEntries)
	{
		if (ItemDefinition && ItemDefinition->ItemDefinition.IsValid())
		{
			FInstancedStruct InstancedItemDefinition = FInstancedStruct::Make<FItemDefinition>();
			InstancedItemDefinition.InitializeAs(ItemDefinition->ItemDefinition.GetScriptStruct(), ItemDefinition->ItemDefinition.GetMemory());
			if(ItemDefinition->ItemDefinition.Get().bSpawnable && DoesItemDefinitionSatisfyPickRequirements(PickRequirements, ItemInstancingContext, InstancedItemDefinition))
			{
				FItemPickEntry NewEntry;
				NewEntry.PickChance = ItemDefinition->ItemDefinition.Get().PickChance;
				NewEntry.PickType.Emplace(ItemDefinition->ItemDefinition);
				PickEntries.Add(NewEntry);
			}
		}
	}

	// @TODO: We need to make this generic so it can be used by any Pick type code and overridden if necessary to implement other algorithms.
	auto GetRandomPick = [&PickEntries](FItemPickEntry& OutEntry)
	{
		float TotalPickChance = 0.f;
		float CurrentPickChance = 0.f;

		for (const FItemPickEntry& PickEntry : PickEntries)
		{
			TotalPickChance += PickEntry.PickChance;
		}

		CurrentPickChance = FMath::RandRange(0.f, TotalPickChance);

		for (const FItemPickEntry& PickEntry : PickEntries)
		{
			const float EntryPickChance = PickEntry.PickChance;
			if (CurrentPickChance < EntryPickChance)
			{
				OutEntry = PickEntry;
				return;
			}
			else
			{
				CurrentPickChance -= EntryPickChance;
			}
		}
	};

	FItemPickEntry RandomPick;
	GetRandomPick(RandomPick);
	if (RandomPick.PickType.IsSet())
	{
		OutItem = FInstancedStruct::Make<FItemDefinition>();
		OutItem.InitializeAs(RandomPick.PickType.GetValue().GetScriptStruct(), RandomPick.PickType.GetValue().GetMemory());
		return true;
	}

	return false;
}

bool UItemDefinitionCollectionPickFunction::DoesItemDefinitionSatisfyPickRequirements_Implementation(const FInstancedStruct& PickRequirements, const FInstancedStruct& ItemInstancingContext, const FInstancedStruct& ItemDefinition) const
{
	if (PickRequirements.IsValid() 
		&& ItemDefinition.IsValid() 
		&& PickRequirements.GetScriptStruct()->IsChildOf(FItemDefinitionCollectionPickRequirements::StaticStruct())
		&& ItemDefinition.GetScriptStruct()->IsChildOf(FItemDefinition::StaticStruct()))
	{
		const FItemDefinitionCollectionPickRequirements& ItemDefinitionPickRequirements = PickRequirements.Get<FItemDefinitionCollectionPickRequirements>();
		const FItemDefinition& Item = ItemDefinition.Get<FItemDefinition>();

		return (Item.QualityLevel >= ItemDefinitionPickRequirements.QualityLevelMinimum && Item.QualityLevel <= ItemDefinitionPickRequirements.QualityLevelMaximum);
	}

	return false;
}

/************************************************************************/
/* Affixes
/************************************************************************/

bool UAffixPickFunction::PickAffix_Implementation(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, FDataTableRowHandle& OutAffixHandle) const
{
	TArray<FDataTableRowHandle> AffixDefinitionHandles;
	GetAffixesWithMinimumNativeRequirements(ItemInstance, ItemInstancingContext, AffixDefinitionHandles);

	// Seed all of the Picks we will make a selection from.
	using FAffixPickEntry = FPickEntry<FDataTableRowHandle>;
	TArray<FAffixPickEntry> PickEntries;
	PickEntries.Reserve(AffixDefinitionHandles.Num());
	for (const FDataTableRowHandle& AffixDefinitionHandle : AffixDefinitionHandles)
	{
		const FAffixDefinitionEntry* AffixDefinitionEntry = AffixDefinitionHandle.GetRow<FAffixDefinitionEntry>(FString());
		if (AffixDefinitionEntry)
		{
			FAffixPickEntry NewEntry;
			NewEntry.PickChance = AffixDefinitionEntry->AffixDefinition.Get().PickChance;
			NewEntry.PickType.Emplace(AffixDefinitionHandle);
			PickEntries.Add(NewEntry);
		}
	}

	// @TODO: We need to make this generic so it can be used by any Pick type code and overridden if necessary to implement other algorithms.
	auto GetRandomPick = [&PickEntries](FAffixPickEntry& OutEntry)
	{
		float TotalPickChance = 0.f;
		float CurrentPickChance = 0.f;

		for (const FAffixPickEntry& PickEntry : PickEntries)
		{
			TotalPickChance += PickEntry.PickChance;
		}

		CurrentPickChance = FMath::RandRange(0.f, TotalPickChance);

		for (const FAffixPickEntry& PickEntry : PickEntries)
		{
			const float EntryPickChance = PickEntry.PickChance;
			if (CurrentPickChance < EntryPickChance)
			{
				OutEntry = PickEntry;
				return;
			}
			else
			{
				CurrentPickChance -= EntryPickChance;
			}
		}
	};

	FAffixPickEntry RandomPick;
	GetRandomPick(RandomPick);
	if (RandomPick.PickType.IsSet())
	{
		OutAffixHandle = RandomPick.PickType.GetValue();
		return true;
	}

	return false;
}

bool UAffixPickFunction::GetAffixesWithMinimumNativeRequirements(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, TArray<FDataTableRowHandle>& OutAffixHandles) const
{
	const FItemInstance* ItemInstancePtr = ItemInstance.GetPtr<FItemInstance>();
	if (!ItemInstancePtr
		|| !ItemInstancePtr->ItemDefinition.IsValid()
		|| !IsValid(ItemInstancePtr->ItemDefinition.Get().InstancingFunction))
	{
		return false;
	}

	if (!AffixPool || !AffixPool->GetRowStruct()->IsChildOf(FAffixDefinitionEntry::StaticStruct()))
	{
		return false;
	}

	// Generate the pool of all Affixes that meet our minimum requirements for selection.
	OutAffixHandles.Empty();
	const FString Context;
	AffixPool->ForeachRow<FAffixDefinitionEntry>(Context, [&](const FName& RowName, const FAffixDefinitionEntry& AffixDefinitionEntry)
	{
		const TInstancedStruct<FAffixDefinition>& AffixDefinitionInstance = AffixDefinitionEntry.AffixDefinition;
		if (AffixDefinitionInstance.IsValid())
		{
			const FAffixDefinition& AffixDefinition = AffixDefinitionInstance.Get();
			if (AffixDefinition.bSpawnable)
			{
				// =====================================================================================
				// 1. The ItemDefinition must be of the same or lower QualityLevel for this Affix to be available.
				if (AffixDefinition.OccursForQualityLevel > 0 && ItemInstancePtr->ItemDefinition.Get().QualityLevel > AffixDefinition.OccursForQualityLevel)
				{
					return;
				}

				// =====================================================================================
				// 2. The AffixLevel of the ItemInstance must be within the range defined on the AffixDefinition.
				if ((AffixDefinition.MinimumRequiredItemAffixLevel > 0 && ItemInstancePtr->AffixLevel < AffixDefinition.MinimumRequiredItemAffixLevel)
					|| (AffixDefinition.MaximumRequiredItemAffixLevel > 0 && ItemInstancePtr->AffixLevel > AffixDefinition.MaximumRequiredItemAffixLevel))
				{
					return;
				}

				// =====================================================================================
				// 3. Make sure the ItemDefinition is of a valid ItemType to receive this Affix.
				const FGameplayTag& ItemType = ItemInstancePtr->ItemDefinition.Get().ItemType;
				if (ItemType.IsValid() && !ItemType.MatchesAny(AffixDefinition.OccursForItemTypes))
				{
					return;
				}

				// =====================================================================================
				// 4. Check to see if the ItemInstance is of the right QualityType to receive this Affix.
				if (ItemInstancePtr->QualityType.IsValid() && !ItemInstancePtr->QualityType.MatchesAny(AffixDefinition.OccursForQualityTypes))
				{
					return;
				}

				// =====================================================================================
				// 5. Check if we already have an Affix of the same type.
				if (ItemInstancePtr->HasAnyAffixOfType(AffixDefinition.AffixType))
				{
					return;
				}

				// Add the Affix to the list of ones we can select from.
				FDataTableRowHandle Handle;
				Handle.DataTable = AffixPool;
				Handle.RowName = RowName;
				OutAffixHandles.Add(Handle);
			}
		}
	});

	return OutAffixHandles.Num() > 0;
}
