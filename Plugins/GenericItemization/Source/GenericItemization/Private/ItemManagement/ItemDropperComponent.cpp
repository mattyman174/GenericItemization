// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "ItemManagement/ItemDropperComponent.h"
#include "ItemManagement/ItemDrop.h"
#include "ItemManagement/ItemInstancer.h"
#include "Kismet/GameplayStatics.h"

UItemDropperComponent::UItemDropperComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	ItemDropClass = AItemDrop::StaticClass();
	ItemInstancer = CreateDefaultSubobject<UItemInstancer>("ItemInstancer");
}

bool UItemDropperComponent::DropItems_Implementation(FInstancedStruct UserContextData, TArray<AItemDrop*>& ItemDrops)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!IsValid(ItemDropClass) || !IsValid(ItemInstancer))
	{
		return false;
	}
	
	TArray<FInstancedStruct> ItemInstances;
	if(ItemInstancer->GenerateItems(UserContextData, ItemInstances))
	{
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
	}

	return ItemDrops.Num() > 0;
}
