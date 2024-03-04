// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "ItemManagement/ItemDropperComponent.h"
#include "ItemManagement/ItemDrop.h"
#include "GenericItemizationInstancingFunctions.h"
#include "GenericItemizationTableTypes.h"
#include "GenericItemizationStatics.h"
#include "Kismet/GameplayStatics.h"

UItemDropperComponent::UItemDropperComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	ItemDropClass = AItemDrop::StaticClass();
	ContextProviderFunction = UItemInstancingContextFunction::StaticClass();
}

bool UItemDropperComponent::DropItems_Implementation(FInstancedStruct UserContextData, TArray<AItemDrop*>& ItemDrops)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!IsValid(ItemDropClass) || !IsValid(ContextProviderFunction))
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

			// Create all of the ItemDrops from those ItemInstances.
			for (const FInstancedStruct& ItemInstance : ItemInstances)
			{
				AItemDrop* ItemDrop = nullptr;
				const FTransform SpawnTransform = FTransform(GetOwner()->GetActorRotation(), GetOwner()->GetActorLocation());
				ItemDrop = GetWorld()->SpawnActorDeferred<AItemDrop>(ItemDropClass, SpawnTransform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				ItemDrop->ItemInstance.InitializeAsScriptStruct(ItemInstance.GetScriptStruct(), ItemInstance.GetMemory());
				UGameplayStatics::FinishSpawningActor(ItemDrop, SpawnTransform);

				// Pass out the new ItemDrop.
				if (IsValid(ItemDrop))
				{
					ItemDrops.Add(ItemDrop);
				}
			}
			
			return ItemDrops.Num() > 0;
		}
	}

	return false;
}
