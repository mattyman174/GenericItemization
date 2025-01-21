// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "ItemManagement/ItemInstancer.h"
#include "GenericItemizationInstancingFunctions.h"
#include "GenericItemizationStatics.h"
#include "GenericItemizationTableTypes.h"

UItemInstancer::UItemInstancer()
{
	ContextProviderFunction = UItemInstancingContextFunction::StaticClass();
}

bool UItemInstancer::GenerateItems_Implementation(FInstancedStruct UserContextData, TArray<FInstancedStruct>& OutItemInstances)
{
	if (!IsValid(ContextProviderFunction))
	{
		return false;
	}

	const FItemDropTableCollectionEntry* DropTableCollection = ItemDropTable.GetRow<FItemDropTableCollectionEntry>(FString());
	if (!IsValid(ItemDropTable.DataTable)
		|| !ItemDropTable.DataTable->GetRowStruct()->IsChildOf(FItemDropTableCollectionEntry::StaticStruct())
		|| !DropTableCollection)
	{
		return false;
	}

	UItemInstancingContextFunction* const ContextProviderFunctionCDO = ContextProviderFunction.GetDefaultObject();
	check(ContextProviderFunctionCDO);

	// Create our ItemInstancingContext so we can pass it along through the process.
	FInstancedStruct ItemInstancingContext;
	if (ContextProviderFunctionCDO->BuildItemInstancingContext(this, UserContextData, ItemInstancingContext))
	{
		// Embed the DropTable and Mutators for future context.
		if (FItemInstancingContext* ItemInstancingContextPtr = ItemInstancingContext.GetMutablePtr<FItemInstancingContext>())
		{
			ItemInstancingContextPtr->DropTable = DropTableCollection;
			ItemInstancingContextPtr->Mutators.Append(DropTableCollection->CustomMutators);
		}

		// Grab all of the ItemDefinitions that we will be creating ItemInstances for.
		// These represent successfully picked Items that will be dropped from that ItemDropTable.
		TArray<FDataTableRowHandle> ItemDefinitionHandles;
		if (UGenericItemizationStatics::PickItemDefinitionsFromDropTable(ItemDropTable, ItemInstancingContext, ItemDefinitionHandles))
		{
			// Generate all of the actual ItemInstances for the ItemDefinitions we selected.
			TArray<FInstancedStruct> ItemInstances;
			for (const FDataTableRowHandle& ItemDefinitionHandle : ItemDefinitionHandles)
			{
				FInstancedStruct ItemInstance;
				if (UGenericItemizationStatics::GenerateItemInstanceFromItemDefinition(ItemDefinitionHandle, ItemInstancingContext, ItemInstance))
				{
					ItemInstances.Add(ItemInstance);
				}
			}

			OutItemInstances = ItemInstances;
			return ItemInstances.Num() > 0;
		}
	}

	return false;
}
