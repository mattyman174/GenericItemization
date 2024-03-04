// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "GenericItemizationStatics.h"
#include "GenericItemizationInstanceTypes.h"
#include "GenericItemizationTableTypes.h"
#include "GenericItemizationTypes.h"
#include "GenericItemizationPickFunctions.h"
#include "GenericItemizationInstancingFunctions.h"
#include "InstancedStruct.h"
#include "StructView.h"
#include "Engine/DataTable.h"
#include "Kismet/KismetSystemLibrary.h"

TOptional<TInstancedStruct<FItemDropTableType>> UGenericItemizationStatics::PickDropTableCollectionEntry(const TInstancedStruct<FItemDropTableCollectionRow>& DropTableCollectionEntry, const FInstancedStruct& ItemInstancingContext, bool bIncludeNoPick)
{
	TOptional<TInstancedStruct<FItemDropTableType>> Result = TOptional<TInstancedStruct<FItemDropTableType>>();

	if (!DropTableCollectionEntry.IsValid())
	{
		return Result;
	}

	UItemDropTableCollectionPickFunction* const PickFunctionCDO = DropTableCollectionEntry.Get().PickFunction.GetDefaultObject();
	check(PickFunctionCDO);

	// Seed the PickFunction and then execute it.
	PickFunctionCDO->ItemDropTableCollectionEntry = DropTableCollectionEntry.Get().ItemDropTableCollectionRow;
	PickFunctionCDO->bIncludeNoPick = bIncludeNoPick;

	FInstancedStruct PickedItem;
	if (PickFunctionCDO->PickItem(DropTableCollectionEntry.Get().PickRequirements, ItemInstancingContext, PickedItem))
	{
		TInstancedStruct<FItemDropTableType> InstancedItem;
		InstancedItem.InitializeAsScriptStruct(PickedItem.GetScriptStruct(), PickedItem.GetMemory());

		Result.Emplace(InstancedItem);
	}

	return Result;
}

TOptional<TInstancedStruct<FItemDefinition>> UGenericItemizationStatics::PickItemDefinitionFromCollection(const TInstancedStruct<FItemDefinitionCollection>& ItemDefinitionCollection, const FInstancedStruct& ItemInstancingContext)
{
	TOptional<TInstancedStruct<FItemDefinition>> Result = TOptional<TInstancedStruct<FItemDefinition>>();

	if (!ItemDefinitionCollection.IsValid())
	{
		return Result;
	}

	if (!IsValid(ItemDefinitionCollection.Get().ItemDefinitions) 
		|| !ItemDefinitionCollection.Get().ItemDefinitions->GetRowStruct()->IsChildOf(FItemDefinitionEntry::StaticStruct()))
	{
		return Result;
	}

	if (!IsValid(ItemDefinitionCollection.Get().PickFunction))
	{
		return Result;
	}

	UItemDefinitionCollectionPickFunction* const PickFunctionCDO = ItemDefinitionCollection.Get().PickFunction.GetDefaultObject();
	check(PickFunctionCDO);

	// Seed the PickFunction and then execute it.
	PickFunctionCDO->ItemDefinitions = ItemDefinitionCollection.Get().ItemDefinitions;

	FInstancedStruct PickedItemDefinition;
	if (PickFunctionCDO->PickItem(ItemDefinitionCollection.Get().PickRequirements, ItemInstancingContext, PickedItemDefinition))
	{
		TInstancedStruct<FItemDefinition> InstancedItemDefinition; 
		InstancedItemDefinition.InitializeAsScriptStruct(PickedItemDefinition.GetScriptStruct(), PickedItemDefinition.GetMemory());

		Result.Emplace(InstancedItemDefinition);
	}

	return Result;
}

TOptional<TInstancedStruct<FItemDefinition>> UGenericItemizationStatics::PickItemDefinitionEntry(const TInstancedStruct<FItemDefinitionRow>& ItemDefinitionEntry, const FInstancedStruct& ItemInstancingContext)
{
	TOptional<TInstancedStruct<FItemDefinition>> Result = TOptional<TInstancedStruct<FItemDefinition>>();

	const FItemDefinitionEntry* ItemDefinitionEntryPtr = ItemDefinitionEntry.Get().ItemDefinitionRow.GetRow<FItemDefinitionEntry>(FString());
	if (!IsValid(ItemDefinitionEntry.Get().ItemDefinitionRow.DataTable) 
		|| !ItemDefinitionEntry.Get().ItemDefinitionRow.DataTable->GetRowStruct()->IsChildOf(FItemDefinitionEntry::StaticStruct()) 
		|| !ItemDefinitionEntryPtr)
	{
		return Result;
	}

	if (ItemDefinitionEntryPtr->ItemDefinition.IsValid() && ItemDefinitionEntryPtr->ItemDefinition.Get().bSpawnable)
	{
		const FItemDefinition& ItemDefinition = ItemDefinitionEntryPtr->ItemDefinition.Get();
		if (ItemDefinition.bSpawnable)
		{
			Result.Emplace(ItemDefinitionEntryPtr->ItemDefinition);
		}
	}

	return Result;
}

TOptional<FDataTableRowHandle> UGenericItemizationStatics::PickAffixDefinitionForItemInstance(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext)
{
	TOptional<FDataTableRowHandle> Result = TOptional<FDataTableRowHandle>();

	const FItemInstance* ItemInstancePtr = ItemInstance.GetPtr<FItemInstance>();
	if (!ItemInstancePtr
		|| !ItemInstancePtr->ItemDefinition.IsValid()
		|| !IsValid(ItemInstancePtr->ItemDefinition.Get().InstancingFunction))
	{
		return Result;
	}

	UDataTable* const AffixPool = ItemInstancePtr->ItemDefinition.Get().AffixPool;
	if (!AffixPool || !AffixPool->GetRowStruct()->IsChildOf(FAffixDefinitionEntry::StaticStruct()))
	{
		return Result;
	}

	const UItemInstancingFunction* const InstancingFunctionCDO = ItemInstancePtr->ItemDefinition.Get().InstancingFunction.GetDefaultObject();
	check(InstancingFunctionCDO);

	if (IsValid(InstancingFunctionCDO->AffixPickFunction))
	{
		UAffixPickFunction* const AffixPickFunctionCDO = InstancingFunctionCDO->AffixPickFunction.GetDefaultObject();
		check(AffixPickFunctionCDO);

		AffixPickFunctionCDO->AffixPool = AffixPool;
		
		FDataTableRowHandle PickedAffixHandle;
		FInstancedStruct PickedAffix;
		if (AffixPickFunctionCDO->PickAffix(ItemInstance, ItemInstancingContext, PickedAffixHandle))
		{
			Result.Emplace(PickedAffixHandle);
		}
	}

	return Result;
}

TOptional<TInstancedStruct<FAffixInstance>> UGenericItemizationStatics::GenerateAffixInstanceFromAffixDefinition(const FDataTableRowHandle& AffixDefinitionHandle, const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext)
{
	TOptional<TInstancedStruct<FAffixInstance>> Result = TOptional<TInstancedStruct<FAffixInstance>>();

	const FItemInstance* ItemInstancePtr = ItemInstance.GetPtr<FItemInstance>();
	if (!ItemInstancePtr 
		|| !ItemInstancePtr->ItemDefinition.IsValid() 
		|| !IsValid(ItemInstancePtr->ItemDefinition.Get().InstancingFunction))
	{
		return Result;
	}

	const UItemInstancingFunction* const InstancingFunctionCDO = ItemInstancePtr->ItemDefinition.Get().InstancingFunction.GetDefaultObject();
	check(InstancingFunctionCDO);

	FInstancedStruct NewAffixInstance;
	InstancingFunctionCDO->MakeAffixInstance(ItemInstancingContext, NewAffixInstance);
	FAffixInstance* MutableAffixInstance = NewAffixInstance.GetMutablePtr<FAffixInstance>();
	if(MutableAffixInstance)
	{
		MutableAffixInstance->SetAffixDefinition(AffixDefinitionHandle);

		TInstancedStruct<FAffixInstance> AffixInstance = TInstancedStruct<FAffixInstance>::Make();
		AffixInstance.InitializeAsScriptStruct(NewAffixInstance.GetScriptStruct(), NewAffixInstance.GetMemory());

		Result.Emplace(AffixInstance);
	}

	return Result;
}

bool PickItemDefinition_Recursive(const TInstancedStruct<FItemDropTableType>& InPick, const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutPick)
{
	if (!InPick.IsValid())
	{
		return false;
	}

	if (InPick.GetScriptStruct()->IsChildOf(FItemDropTableCollectionRow::StaticStruct()))
	{
		TInstancedStruct<FItemDropTableCollectionRow> PickedItemDropTableCollection;
		PickedItemDropTableCollection.InitializeAsScriptStruct(InPick.GetScriptStruct(), InPick.GetMemory());
		const TOptional<TInstancedStruct<FItemDropTableType>> DropTableType = UGenericItemizationStatics::PickDropTableCollectionEntry(PickedItemDropTableCollection, ItemInstancingContext, false);
		if (DropTableType.IsSet())
		{
			return PickItemDefinition_Recursive(DropTableType.GetValue(), ItemInstancingContext, OutPick);
		}
	}
	else if (InPick.GetScriptStruct()->IsChildOf(FItemDefinitionCollection::StaticStruct()))
	{
		TInstancedStruct<FItemDefinitionCollection> PickedItemDefinitionCollection;
		PickedItemDefinitionCollection.InitializeAsScriptStruct(InPick.GetScriptStruct(), InPick.GetMemory());
		const TOptional<TInstancedStruct<FItemDefinition>> ItemDefinition = UGenericItemizationStatics::PickItemDefinitionFromCollection(PickedItemDefinitionCollection, ItemInstancingContext);
		if (ItemDefinition.IsSet())
		{
			FInstancedStruct Item;
			Item.InitializeAs(ItemDefinition.GetValue().GetScriptStruct(), ItemDefinition.GetValue().GetMemory());
			OutPick = Item;
			return true;
		}
	}
	else if (InPick.GetScriptStruct()->IsChildOf(FItemDefinitionRow::StaticStruct()))
	{
		TInstancedStruct<FItemDefinitionRow> PickedItemDefinitionRow;
		PickedItemDefinitionRow.InitializeAsScriptStruct(InPick.GetScriptStruct(), InPick.GetMemory());
		const TOptional<TInstancedStruct<FItemDefinition>> ItemDefinition = UGenericItemizationStatics::PickItemDefinitionEntry(PickedItemDefinitionRow, ItemInstancingContext);
		if (ItemDefinition.IsSet())
		{
			FInstancedStruct Item;
			Item.InitializeAs(ItemDefinition.GetValue().GetScriptStruct(), ItemDefinition.GetValue().GetMemory());
			OutPick = Item;
			return true;
		}
	}

	return false;
}

bool UGenericItemizationStatics::PickItemDefinitionsFromDropTable(const FDataTableRowHandle& ItemDropTableCollectionEntry, const FInstancedStruct& ItemInstancingContext, TArray<FInstancedStruct>& OutItemDefinitions)
{
	const FDataTableRowHandle& DropTable = ItemDropTableCollectionEntry;
	const FItemDropTableCollectionEntry* DropTableCollection = DropTable.GetRow<FItemDropTableCollectionEntry>(FString());
	if (!IsValid(DropTable.DataTable)
		|| !DropTable.DataTable->GetRowStruct()->IsChildOf(FItemDropTableCollectionEntry::StaticStruct())
		|| !DropTableCollection)
	{
		return false;
	}

	int32 PickCount = DropTableCollection->PickCount;
	OutItemDefinitions.Empty(PickCount);
	while (PickCount > 0)
	{
		PickCount--;

		// Setup and make the IntialPick. 
		// We need to build an input struct from the DataTable parameter so we can make use of the convenience function.
		FItemDropTableCollectionRow DropTableCollectionRow;
		DropTableCollectionRow.ItemDropTableCollectionRow = DropTable;
		const TOptional<TInstancedStruct<FItemDropTableType>> InitialPickResult = UGenericItemizationStatics::PickDropTableCollectionEntry(TInstancedStruct<FItemDropTableCollectionRow>::Make(DropTableCollectionRow), ItemInstancingContext, true);
		if (InitialPickResult.IsSet()) // If this is false, then we either ended with NoPick being selected or something failed.
		{
			FInstancedStruct FinalPick;
			if (PickItemDefinition_Recursive(InitialPickResult.GetValue(), ItemInstancingContext, FinalPick))
			{
				OutItemDefinitions.Add(FinalPick);
			}
		}
	}

	return OutItemDefinitions.Num() > 0;
}

bool UGenericItemizationStatics::GenerateItemInstanceFromItemDefinition(const FInstancedStruct& ItemDefinition, const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItemInstance)
{
	if (!ItemDefinition.IsValid() || !ItemDefinition.GetScriptStruct()->IsChildOf(FItemDefinition::StaticStruct()))
	{
		return false;
	}

	const FConstStructView ItemInstancingContextView = FConstStructView(ItemInstancingContext);
	const FItemInstancingContext* const ItemInstancingContextPtr = ItemInstancingContextView.GetPtr<const FItemInstancingContext>();
	if (!ItemInstancingContextPtr)
	{
		return false;
	}

	// =====================================================================================
	// 1. Create the new ItemInstance from the ItemDefinition that we have.

	TInstancedStruct<FItemDefinition> ItemInstanceItemDefinition;
	ItemInstanceItemDefinition.InitializeAsScriptStruct(ItemDefinition.GetScriptStruct(), ItemDefinition.GetMemory());

	if (!IsValid(ItemInstanceItemDefinition.Get().InstancingFunction))
	{
		return false;
	}

	UItemInstancingFunction* const InstancingFunctionCDO = ItemInstanceItemDefinition.Get().InstancingFunction.GetDefaultObject();
	check(InstancingFunctionCDO);

	FInstancedStruct NewItemInstance;
	InstancingFunctionCDO->MakeItemInstance(ItemInstancingContext, NewItemInstance);
	FItemInstance* MutableItemInstance = NewItemInstance.GetMutablePtr<FItemInstance>();
	if (!MutableItemInstance)
	{
		return false;
	}

	MutableItemInstance->ItemDefinition = ItemInstanceItemDefinition;
	MutableItemInstance->ItemSeed = FMath::Rand(); // @TODO: How do we generate the ItemSeed? Do we need some kind of global manager that handles that?
	MutableItemInstance->ItemStream.Initialize(MutableItemInstance->ItemSeed);
	MutableItemInstance->ItemLevel = FMath::Clamp<int32>(ItemInstancingContextPtr->ItemLevel, 1, InstancingFunctionCDO->GetMaximumItemLevel());

	// =====================================================================================
	// 2. Calculate the Affix Level. This affects what Affixes can be selected for later.
	{
		bool bCalculatedAffixLevel = InstancingFunctionCDO->CalculateAffixLevel(NewItemInstance, ItemInstancingContext, MutableItemInstance->AffixLevel);
		if (!bCalculatedAffixLevel)
		{
			return false;
		}
	}
	
	// =====================================================================================
	// 3. Roll for the QualityType from the ItemQualityRatio defined on the ItemDefinition. 
	// This also affects what Affixes can be selected for later.
	{
		bool bSelectedItemQualityType = InstancingFunctionCDO->SelectItemQualityType(NewItemInstance, ItemInstancingContext, MutableItemInstance->QualityType);
		if (!bSelectedItemQualityType)
		{
			return false;
		}
	}

	// =====================================================================================
	// 4. Roll for the number of Affixes from the AffixCountRatio on the ItemDefinition.
	// Generate all the AffixInstances for the ItemInstance based on the information we generated earlier.
	{
		// Add all our predefined Affixes.
		for (const FDataTableRowHandle& PredefinedAffix : ItemInstanceItemDefinition.Get().PredefinedAffixes)
		{
			const FAffixDefinitionEntry* AffixDefinitionEntryPtr = PredefinedAffix.GetRow<FAffixDefinitionEntry>(FString());
			if (IsValid(PredefinedAffix.DataTable)
				&& PredefinedAffix.DataTable->GetRowStruct()->IsChildOf(FAffixDefinitionEntry::StaticStruct())
				&& AffixDefinitionEntryPtr)
			{
				TOptional<TInstancedStruct<FAffixInstance>> AffixInstance = UGenericItemizationStatics::GenerateAffixInstanceFromAffixDefinition(PredefinedAffix, NewItemInstance, ItemInstancingContext);
				if (AffixInstance.IsSet() && AffixInstance.GetValue().IsValid())
				{
					// Successfully generated the Affix, now apply it to the ItemInstance.
					TInstancedStruct<FAffixInstance>& AffixInstanceStruct = AffixInstance.GetValue();
					AffixInstanceStruct.GetMutable().bPredefinedAffix = true;

					MutableItemInstance->Affixes.Add(AffixInstanceStruct);
				}
				else
				{
					// @TODO: Emmit an error?
				}
			}
		}

		if (!ItemInstanceItemDefinition.Get().bOnlyPredefinedAffixes)
		{
			int32 AffixCount = 0;
			bool bDeterminedAffixCount = InstancingFunctionCDO->DetermineAffixCount(NewItemInstance, ItemInstancingContext, AffixCount);
			if (!bDeterminedAffixCount)
			{
				return false;
			}

			// Instance AffixCount number of new Affixes for the ItemInstance.
			while (AffixCount > 0)
			{
				AffixCount--;

				const TOptional<FDataTableRowHandle> AffixDefinitionHandle = UGenericItemizationStatics::PickAffixDefinitionForItemInstance(NewItemInstance, ItemInstancingContext);
				if (AffixDefinitionHandle.IsSet() && !AffixDefinitionHandle.GetValue().IsNull())
				{
					const TOptional<TInstancedStruct<FAffixInstance>> AffixInstance = UGenericItemizationStatics::GenerateAffixInstanceFromAffixDefinition(AffixDefinitionHandle.GetValue(), NewItemInstance, ItemInstancingContext);
					if (AffixInstance.IsSet() && AffixInstance.GetValue().IsValid())
					{
						// Successfully generated the Affix, now apply it to the ItemInstance and move on.
						MutableItemInstance->Affixes.Add(AffixInstance.GetValue());
					}
					else
					{
						// @TODO: Emmit an error?
					}
				}
				else
				{
					// @TODO: Emmit an error?
				}
			}
		}
	}

	// =====================================================================================
	// 5. Determine the number of Stacks this ItemInstance will have.
	// This is based on the StackSettings defined on the ItemDefinition.
	{
		int32 DesiredStackCount = 1;
		bool bCalculatedStackCount = InstancingFunctionCDO->CalculateStackCount(NewItemInstance, ItemInstancingContext, DesiredStackCount);
		if (!bCalculatedStackCount)
		{
			// @TODO: Emmit an error? This isn't necessarily a failure case as we can default.
		}

		// The StackCount must be >= 1, otherwise it doesnt make sense.
		// An empty or negative stack would mean the ItemInstance shouldn't exist.
		MutableItemInstance->StackCount = FMath::Max(1, DesiredStackCount);
	}

	OutItemInstance = NewItemInstance;
	return true;
}

bool UGenericItemizationStatics::GenerateItemInstanceFromTemplate(const FInstancedStruct& ItemInstanceTemplate, FInstancedStruct& OutItemInstanceCopy)
{
	const FConstStructView ItemInstanceTemplateView = FConstStructView(ItemInstanceTemplate);
	const FItemInstance* const ItemInstanceTemplatePtr = ItemInstanceTemplateView.GetPtr<const FItemInstance>();
	if (!ItemInstanceTemplatePtr)
	{
		return false;
	}

	// Make the copy.
	OutItemInstanceCopy.Reset();
	OutItemInstanceCopy = ItemInstanceTemplate;

	// Generate new unique Id information.
	FItemInstance& MutableItemInstance = OutItemInstanceCopy.GetMutable<FItemInstance>();
	MutableItemInstance.ItemId = FGuid::NewGuid();
	MutableItemInstance.ItemSeed = FMath::Rand(); // @TODO: How do we generate the ItemSeed? Do we need some kind of global manager that handles that?
	MutableItemInstance.ItemStream.Initialize(MutableItemInstance.ItemSeed);

	return true;
}
