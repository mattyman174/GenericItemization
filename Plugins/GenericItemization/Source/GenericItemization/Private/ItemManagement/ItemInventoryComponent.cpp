// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "ItemManagement/ItemInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "ItemManagement/ItemDrop.h"

UItemInventoryComponent::UItemInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);

	ItemDropClass = AItemDrop::StaticClass();

	bCachedIsNetSimulated = false;
}

void UItemInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	// Technically, this doesn't need to be PushModel based because it's a FastArray and they ignore it, but it can't hurt.
	DOREPLIFETIME_WITH_PARAMS_FAST(UItemInventoryComponent, ItemInstances, SharedParams);
}

void UItemInventoryComponent::PreNetReceive()
{
	// Update the cached IsNetSimulated value here if this component is still considered authority.
	// Even though the value is also cached in OnRegister and BeginPlay, clients may
	// receive properties before OnBeginPlay, so this ensures the role is correct for that case.
	if (!bCachedIsNetSimulated)
	{
		CacheIsNetSimulated();
	}
}

void UItemInventoryComponent::OnRegister()
{
	Super::OnRegister();

	// Cached off net role to avoid constant checking on owning actor.
	CacheIsNetSimulated();

	ItemInstances.Register(this);
}

void UItemInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache net role here as well since for map-placed actors on clients, the Role may not be set correctly yet in OnRegister.
	CacheIsNetSimulated();
}

bool UItemInventoryComponent::CanTakeItem_Implementation(const FInstancedStruct& Item, FInstancedStruct UserContextData)
{
	if (Item.IsValid() && Item.GetPtr<FItemInstance>() && Item.Get<FItemInstance>().IsValid())
	{
		return true;
	}

	return false;
}

bool UItemInventoryComponent::TakeItem(FInstancedStruct& Item, FInstancedStruct UserContextData)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!CanTakeItem(Item, UserContextData))
	{
		return false;
	}

	// Capture the ItemInstance so that we can take it and manage it thereafter.
	FInstancedStruct& ItemInstance = Item;

	ItemInstances.AddItemInstance(ItemInstance, UserContextData);

	Item.Reset(); // Reset the Instanced Struct just to make sure, we are actually making a logical transfer of ownership to the Inventory Component.

	return true;
}

bool UItemInventoryComponent::TakeItemDrop(AItemDrop* ItemDrop, FInstancedStruct UserContextData, bool bDestroyItemDrop /*= true*/)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!IsValid(ItemDrop) || !ItemDrop->HasValidItemInstance())
	{
		return false;
	}

	// Capture the ItemInstance from the ItemDrop so that we can take it and manage it thereafter.
	FInstancedStruct ItemInstance;
	ItemInstance.InitializeAs(ItemDrop->ItemInstance.GetScriptStruct(), ItemDrop->ItemInstance.GetMemory());

	if (!ItemDrop->CanTakeItem(this) || !CanTakeItem(ItemInstance, UserContextData))
	{
		return false;
	}

	ItemInstances.AddItemInstance(ItemInstance, UserContextData);

	ItemDrop->ItemInstance.Reset(); // This is critical, we need to Reset the Instanced Struct on the ItemDrop, as we are actually making a logical transfer of ownership to the Inventory Component.

	if (bDestroyItemDrop)
	{
		ItemDrop->Destroy(true);
	}

	return true;
}

bool UItemInventoryComponent::DropItem_Implementation(FGuid ItemToDrop, AItemDrop*& OutItemDrop)
{
	if (HasAuthority())
	{
		// Capture the ItemInstance from the managed container so that it can be dropped.
		FInstancedStruct ItemInstance;
		for (FFastItemInstance& FastItemInstance : ItemInstances)
		{
			const FItemInstance* const ItemInstancePtr = FastItemInstance.ItemInstance.GetPtr<FItemInstance>();
			if (FastItemInstance.ItemInstance.IsValid() && ItemInstancePtr && ItemInstancePtr->ItemId == ItemToDrop)
			{
				ItemInstance = FastItemInstance.ItemInstance;
				ItemInstances.RemoveItemInstance(ItemToDrop); // This is critical, we must remove the ItemInstance from the Inventory, since we are just copying the data here, we are actually making a logical transfer of ownership to the ItemDrop.
				break;
			}
		}

		if(ItemInstance.IsValid())
		{
			AItemDrop* ItemDrop = nullptr;
			const FTransform SpawnTransform = FTransform(GetOwner()->GetActorRotation(), GetOwner()->GetActorLocation());
			ItemDrop = GetWorld()->SpawnActorDeferred<AItemDrop>(ItemDropClass, SpawnTransform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			ItemDrop->ItemInstance.InitializeAsScriptStruct(ItemInstance.GetScriptStruct(), ItemInstance.GetMemory());
			UGameplayStatics::FinishSpawningActor(ItemDrop, SpawnTransform);

			OutItemDrop = ItemDrop;
			return true;
		}
	}

	OutItemDrop = nullptr;
	return false;
}

bool UItemInventoryComponent::ReleaseItem(FGuid ItemToRelease, FInstancedStruct& OutItem)
{
	if (HasAuthority())
	{
		for (FFastItemInstance& FastItemInstance : ItemInstances)
		{
			const FItemInstance* const ItemInstancePtr = FastItemInstance.ItemInstance.GetPtr<FItemInstance>();
			if (FastItemInstance.ItemInstance.IsValid() && ItemInstancePtr && ItemInstancePtr->ItemId == ItemToRelease)
			{
				OutItem = FastItemInstance.ItemInstance;
				ItemInstances.RemoveItemInstance(ItemToRelease); // This is critical, we must remove the ItemInstance from the Inventory as we are no longer managing it.
				return true;
			}
		}
	}

	return false;
}

TArray<FInstancedStruct> UItemInventoryComponent::GetItems()
{
	TArray<FInstancedStruct> OutItems;
	ItemInstances.GetItemInstances(OutItems);
	return OutItems;
}

TArray<FFastItemInstance> UItemInventoryComponent::GetItemsWithContext()
{
	TArray<FFastItemInstance> OutItems;
	OutItems.Append(ItemInstances.ItemInstances);
	return OutItems;
}

FInstancedStruct UItemInventoryComponent::GetItem(FGuid ItemId, bool& bSuccessful)
{
	FFastItemInstance FastItemInstance;
	if (ItemInstances.GetFastItemInstance(ItemId, FastItemInstance))
	{
		bSuccessful = true;
		return FastItemInstance.ItemInstance;
	}

	return FInstancedStruct();
}

FInstancedStruct UItemInventoryComponent::GetItemContextData(FGuid ItemId, bool& bSuccessful)
{
	FFastItemInstance FastItemInstance;
	if (ItemInstances.GetFastItemInstance(ItemId, FastItemInstance))
	{
		bSuccessful = true;
		return FastItemInstance.UserContextData;
	}

	return FInstancedStruct();
}

int32 UItemInventoryComponent::GetNumItems() const
{
	return ItemInstances.GetNum();
}

bool UItemInventoryComponent::HasAuthority() const
{
	return !bCachedIsNetSimulated;
}

void UItemInventoryComponent::CacheIsNetSimulated()
{
	bCachedIsNetSimulated = IsNetSimulating();
	ItemInstances.bOwnerIsNetAuthority = HasAuthority();
}

void UItemInventoryComponent::K2_OnAddedItem_Implementation(const FInstancedStruct& Item, const FInstancedStruct& UserContextData)
{
	// Left empty intentionally to be overridden.
}

void UItemInventoryComponent::K2_OnRemovedItem_Implementation(const FInstancedStruct& Item, const FInstancedStruct& UserContextData)
{
	// Left empty intentionally to be overridden.
}

void UItemInventoryComponent::OnAddedItemInstance(const FFastItemInstance& FastItemInstance)
{
	GetOwner()->ForceNetUpdate();

	const FInstancedStruct& ItemInstance = FastItemInstance.ItemInstance;
	OnItemTakenDelegate.Broadcast(this, ItemInstance, FastItemInstance.UserContextData);
	K2_OnAddedItem(ItemInstance, FastItemInstance.UserContextData);
}

void UItemInventoryComponent::OnRemovedItemInstance(const FFastItemInstance& FastItemInstance)
{
	GetOwner()->ForceNetUpdate();

	const FInstancedStruct& ItemInstance = FastItemInstance.ItemInstance;
	OnItemRemovedDelegate.Broadcast(this, ItemInstance, FastItemInstance.UserContextData);
	K2_OnRemovedItem(ItemInstance, FastItemInstance.UserContextData);
}
