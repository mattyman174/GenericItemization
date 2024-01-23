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

	ItemDropClass = AItemDrop::StaticClass();
	ContextProviderFunction = UItemInstancingContextFunction::StaticClass();
}

bool UItemDropperComponent::DropItems_Implementation(const FInstancedStruct& UserContextData, TArray<AItemDrop*>& ItemDrops, bool bDeferredSpawn /*= false*/)
{
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
		// Grab all of the ItemDefinitions that we will be creating ItemInstances for.
		// These represent successfully picked Items that will be dropped from that ItemDropTable.
		TArray<FInstancedStruct> ItemDefinitions;
		if (UGenericItemizationStatics::PickItemDefinitionsFromDropTable(ItemDropTable, ItemInstancingContext, ItemDefinitions))
		{
			// Generate all of the actual ItemInstances for the ItemDefinitions we selected.
			TArray<FInstancedStruct> ItemInstances;
			for (const FInstancedStruct& ItemDefinition : ItemDefinitions)
			{
				FInstancedStruct ItemInstance;
				if (UGenericItemizationStatics::GenerateItemInstanceFromItemDefinition(ItemDefinition, ItemInstancingContext, ItemInstance))
				{
					ItemInstances.Add(ItemInstance);
				}
			}

			// Create all of the ItemDrops from those ItemInstances.
			for (const FInstancedStruct& ItemInstance : ItemInstances)
			{
				AItemDrop* ItemDrop = nullptr;
				const FTransform SpawnTransform = FTransform(GetOwner()->GetActorRotation(), GetOwner()->GetActorLocation());
				if (bDeferredSpawn)
				{
					ItemDrop = GetWorld()->SpawnActorDeferred<AItemDrop>(ItemDropClass, SpawnTransform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					ItemDrop->ItemInstance.InitializeAsScriptStruct(ItemInstance.GetScriptStruct(), ItemInstance.GetMemory());
					// @NOTE: We are not calling UGameplayStatics::FinishSpawningActor here as we are deferring that for the calling code to manage.
				}
				else
				{
					ItemDrop = GetWorld()->SpawnActorDeferred<AItemDrop>(ItemDropClass, SpawnTransform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					ItemDrop->ItemInstance.InitializeAsScriptStruct(ItemInstance.GetScriptStruct(), ItemInstance.GetMemory());
					UGameplayStatics::FinishSpawningActor(ItemDrop, SpawnTransform);
				}

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
