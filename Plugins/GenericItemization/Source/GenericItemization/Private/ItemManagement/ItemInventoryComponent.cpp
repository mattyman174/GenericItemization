// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "ItemManagement/ItemInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "ItemManagement/ItemDrop.h"
#include "GenericItemizationStatics.h"
#include "ItemManagement/ItemStackSettings.h"
#include "GenericItemizationTags.h"
#include "ItemManagement/ItemInstancer.h"

UItemInventoryComponent::UItemInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);

	ItemDropClass = AItemDrop::StaticClass();
	ItemInstancer = CreateDefaultSubobject<UItemInstancer>("ItemInstancer");

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
	ItemInstances.AddItemInstance(Item, UserContextData);

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

bool UItemInventoryComponent::CanSplitItemStack_Implementation(FGuid ItemToSplit, int32 SplitCount, int32& OutRemainder)
{
	bool bHasItemToSplit = false;
	const FInstancedStruct& ItemToSplitItemInstance = GetItem(ItemToSplit, bHasItemToSplit);
	if (!bHasItemToSplit)
	{
		OutRemainder = 0;
		return false;
	}

	const FItemInstance& ItemInstance = ItemToSplitItemInstance.Get<FItemInstance>();
	const int32 StackCount = ItemInstance.StackCount;
	OutRemainder = StackCount - SplitCount;
	return OutRemainder >= 1;
}

bool UItemInventoryComponent::SplitItemStack(FGuid ItemToSplit, int32 SplitCount, FInstancedStruct& OutSplitItemInstance)
{
	int32 Remainder;
	if (!CanSplitItemStack(ItemToSplit, SplitCount, Remainder))
	{
		return false;
	}

	const FFastItemInstance* FastItemInstanceToSplit = ItemInstances.GetItemInstance(ItemToSplit);
	if (!FastItemInstanceToSplit)
	{
		return false;
	}

	// Create the new Split ItemInstance that represents the Item we split and assign it the a new StackCount as the SplitCount.
	const bool bGeneratedNewItemInstance = UGenericItemizationStatics::GenerateItemInstanceFromTemplate(FastItemInstanceToSplit->ItemInstance, OutSplitItemInstance);
	if (!bGeneratedNewItemInstance)
	{
		return false;
	}

	OutSplitItemInstance.GetMutable<FItemInstance>().StackCount = SplitCount;

	// Then modify the original Item that we split from so that it has the remaining StackCount completing the split.
	ItemInstances.ModifyItemInstanceWithChangeDescriptor<FItemInstance>(
		ItemToSplit, 
		GenericItemizationGameplayTags::ItemInstanceChange_StackCount, 
		{ GET_MEMBER_NAME_CHECKED(FItemInstance, StackCount) },
		[Remainder](FItemInstance* MutableItemInstance)
		{
			MutableItemInstance->StackCount = Remainder;
		}
	);

	return true;
}

bool UItemInventoryComponent::CanStackItems(UItemInventoryComponent* ItemToStackFromInventory, FGuid ItemToStackFrom, FGuid ItemToStackWith, bool& bOutItemToStackFromWillExpunge)
{
	if (!IsValid(ItemToStackFromInventory))
	{
		return false;
	}

	const FFastItemInstance* FastItemToStackWithInstance = ItemInstances.GetItemInstance(ItemToStackWith);
	if (!FastItemToStackWithInstance)
	{
		return false;
	}

	const FItemInstance* ItemToStackWithPtr = FastItemToStackWithInstance->ItemInstance.GetPtr<FItemInstance>();
	if (!ItemToStackWithPtr)
	{
		return false;
	}

	const FItemDefinition* const ItemToStackWithItemDefinitionPtr = ItemToStackWithPtr->GetItemDefinition().GetPtr();
	if (!ItemToStackWithItemDefinitionPtr)
	{
		return false;
	}

	bool bItemToStackFromInstanceSuccessful = false;
	FInstancedStruct ItemToStackFromInstance = ItemToStackFromInventory->GetItem(ItemToStackFrom, bItemToStackFromInstanceSuccessful);
	if (!bItemToStackFromInstanceSuccessful)
	{
		return false;
	}

	// Find out if we can actually stack these Items and if anything will remain.
	int32 StackRemainder = 0;
	const UItemStackSettings* const ItemToStackWithStackSettingsCDO = ItemToStackWithItemDefinitionPtr->StackSettings.GetDefaultObject();
	if (!ItemToStackWithStackSettingsCDO || !ItemToStackWithStackSettingsCDO->CanStackWith(ItemToStackFromInstance, FastItemToStackWithInstance->ItemInstance, StackRemainder))
	{
		return false;
	}

	bOutItemToStackFromWillExpunge = StackRemainder < 1;
	return true;
}

bool UItemInventoryComponent::StackItemFromInventory(UItemInventoryComponent* ItemToStackFromInventory, FGuid ItemToStackFrom, FGuid ItemToStackWith, bool& bOutItemToStackFromWasExpunged)
{
	if (!IsValid(ItemToStackFromInventory))
	{
		return false;
	}

	const FFastItemInstance* FastItemToStackWithInstance = ItemInstances.GetItemInstance(ItemToStackWith);
	if (!FastItemToStackWithInstance)
	{
		return false;
	}

	const FItemInstance* const ItemToStackWithPtr = FastItemToStackWithInstance->ItemInstance.GetPtr<FItemInstance>();
	if (!ItemToStackWithPtr)
	{
		return false;
	}

	const FItemDefinition* const ItemToStackWithItemDefinitionPtr = ItemToStackWithPtr->GetItemDefinition().GetPtr();
	if (!ItemToStackWithItemDefinitionPtr)
	{
		return false;
	}

	bool bItemToStackFromInstanceSuccessful = false;
	FInstancedStruct ItemToStackFromInstance = ItemToStackFromInventory->GetItem(ItemToStackFrom, bItemToStackFromInstanceSuccessful);
	if (!bItemToStackFromInstanceSuccessful)
	{
		return false;
	}

	// Find out if we can actually stack these Items and if anything will remain.
	int32 StackRemainder = 0;
	const UItemStackSettings* const ItemToStackWithStackSettingsCDO = ItemToStackWithItemDefinitionPtr->StackSettings.GetDefaultObject();
	if (!ItemToStackWithStackSettingsCDO || !ItemToStackWithStackSettingsCDO->CanStackWith(ItemToStackFromInstance, FastItemToStackWithInstance->ItemInstance, StackRemainder))
	{
		return false;
	}

	const FItemInstance* const ItemToStackFromPtr = ItemToStackFromInstance.GetPtr<FItemInstance>();
	if (!ItemToStackFromPtr)
	{
		return false;
	}

	const int32 ItemToStackFromStackCount = ItemToStackFromPtr->StackCount;
	const bool bPerformDeduction = StackRemainder >= 1;
	bOutItemToStackFromWasExpunged = !bPerformDeduction;

	if (bPerformDeduction)
	{
		ItemInstances.ModifyItemInstanceWithChangeDescriptor<FItemInstance>(
			ItemToStackFrom,
			GenericItemizationGameplayTags::ItemInstanceChange_StackCount, 
			{ GET_MEMBER_NAME_CHECKED(FItemInstance, StackCount) },
			[StackRemainder](FItemInstance* MutableItemInstance)
			{
				// Since there is a remainder after the stacking operation, the ItemToStackFrom needs to be updated to reflect that change.
				MutableItemInstance->StackCount = StackRemainder;
			}
		);
	}
	else
	{
		// Release and reset the ItemToStackFrom, as we are effectively destroying it since all of its stacks will be removed.
		FInstancedStruct ExpungedItemToStackFrom;
		ItemToStackFromInventory->ReleaseItem(ItemToStackFrom, ExpungedItemToStackFrom);
		ExpungedItemToStackFrom.Reset();
	}

	ItemInstances.ModifyItemInstanceWithChangeDescriptor<FItemInstance>(
		ItemToStackWith, 
		GenericItemizationGameplayTags::ItemInstanceChange_StackCount, 
		{ GET_MEMBER_NAME_CHECKED(FItemInstance, StackCount) },
		[ItemToStackFromStackCount, StackRemainder](FItemInstance* MutableItemInstance)
		{
			// Grant the ItemToStackWith all of the stacks that it is receiving.
			MutableItemInstance->StackCount += ItemToStackFromStackCount - FMath::Clamp(StackRemainder, 0, ItemToStackFromStackCount);
		}
	);

	return true;
}

bool UItemInventoryComponent::StackItemFromItemDrop(AItemDrop* ItemToStackFromItemDrop, FGuid ItemToStackWith, bool& bOutItemToStackFromWasExpunged, bool bDestroyItemDrop /*= true*/)
{
	if (!IsValid(ItemToStackFromItemDrop))
	{
		return false;
	}

	const FFastItemInstance* FastItemToStackWithInstance = ItemInstances.GetItemInstance(ItemToStackWith);
	if (!FastItemToStackWithInstance)
	{
		return false;
	}

	const FItemInstance* const ItemToStackWithPtr = FastItemToStackWithInstance->ItemInstance.GetPtr<FItemInstance>();
	if (!ItemToStackWithPtr)
	{
		return false;
	}

	const FItemDefinition* const ItemToStackWithItemDefinitionPtr = ItemToStackWithPtr->GetItemDefinition().GetPtr();
	if (!ItemToStackWithItemDefinitionPtr)
	{
		return false;
	}

	TInstancedStruct<FItemInstance> ItemToStackFromInstance;
	ItemToStackFromItemDrop->GetItemInstance(ItemToStackFromInstance);
	if (!ItemToStackFromInstance.IsValid())
	{
		return false;
	}

	FInstancedStruct ImmutableItemToStackFromInstance;
	ImmutableItemToStackFromInstance.InitializeAs(ItemToStackFromInstance.GetScriptStruct(), ItemToStackFromInstance.GetMemory());

	// Find out if we can actually stack these Items and if anything will remain.
	int32 StackRemainder = 0;
	const UItemStackSettings* const ItemToStackWithStackSettingsCDO = ItemToStackWithItemDefinitionPtr->StackSettings.GetDefaultObject();
	if (!ItemToStackWithStackSettingsCDO || !ItemToStackWithStackSettingsCDO->CanStackWith(ImmutableItemToStackFromInstance, FastItemToStackWithInstance->ItemInstance, StackRemainder))
	{
		return false;
	}

	const FItemInstance* const ItemToStackFromPtr = ItemToStackFromInstance.GetPtr();
	if (!ItemToStackFromPtr)
	{
		return false;
	}

	const int32 ItemToStackFromStackCount = ItemToStackFromPtr->StackCount;
	const bool bPerformDeduction = StackRemainder >= 1;
	bOutItemToStackFromWasExpunged = !bPerformDeduction;

	if (bPerformDeduction)
	{
		// Since there is a remainder after the stacking operation, the ItemToStackFrom needs to be updated to reflect that change.
		ItemToStackFromInstance.GetMutablePtr()->StackCount = StackRemainder;

		// Because we changed the ItemToStackFrom, we should re-drop it as a new ItemDrop so that it can reflect these changes.
		AItemDrop* NewItemDrop = nullptr;
		const FTransform SpawnTransform = FTransform(ItemToStackFromItemDrop->GetActorRotation(), ItemToStackFromItemDrop->GetActorLocation());
		NewItemDrop = GetWorld()->SpawnActorDeferred<AItemDrop>(ItemDropClass, SpawnTransform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		NewItemDrop->ItemInstance.InitializeAsScriptStruct(ItemToStackFromInstance.GetScriptStruct(), ItemToStackFromInstance.GetMemory());
		UGameplayStatics::FinishSpawningActor(NewItemDrop, SpawnTransform);

		// Reset and Destroy the ItemToStackFrom that we originally came from as the NewItemDrop is taking its place.
		ItemToStackFromInstance.Reset();
		ItemToStackFromItemDrop->Destroy(true);
	}
	else
	{
		// Reset the ItemToStackFrom, as we are effectively destroying it since all of its stacks will be removed.
		ItemToStackFromInstance.Reset();

		if (bDestroyItemDrop)
		{
			ItemToStackFromItemDrop->Destroy(true);
		}
	}

	ItemInstances.ModifyItemInstanceWithChangeDescriptor<FItemInstance>(
		ItemToStackWith, 
		GenericItemizationGameplayTags::ItemInstanceChange_StackCount, 
		{ GET_MEMBER_NAME_CHECKED(FItemInstance, StackCount) },
		[ItemToStackFromStackCount, StackRemainder](FItemInstance* MutableItemInstance)
		{
			// Grant the ItemToStackWith all of the stacks that it is receiving.
			MutableItemInstance->StackCount += ItemToStackFromStackCount - FMath::Clamp(StackRemainder, 0, ItemToStackFromStackCount);
		}
	);

	return true;
}

bool UItemInventoryComponent::CanSocketItem_Implementation(const FInstancedStruct& ItemToSocket, FGuid ItemToSocketInto, FGuid& OutSocketId)
{
	const FFastItemInstance* FastItemToSocketIntoPtr = ItemInstances.GetItemInstance(ItemToSocketInto);
	if (!FastItemToSocketIntoPtr)
	{
		return false;
	}

	const FItemInstance* ItemToSocketIntoPtr = FastItemToSocketIntoPtr->ItemInstance.GetPtr<FItemInstance>();
	if (!ItemToSocketIntoPtr)
	{
		return false;
	}

	for (const TInstancedStruct<FItemSocketInstance>& SocketInstance : ItemToSocketIntoPtr->Sockets)
	{
		if (SocketInstance.GetPtr() && CanSocketItemIntoSocket(ItemToSocket, ItemToSocketInto, SocketInstance.GetPtr()->SocketId))
		{
			OutSocketId = SocketInstance.GetPtr()->SocketId;
			return true;
		}
	}

	return false;
}

bool UItemInventoryComponent::CanSocketItemIntoSocket_Implementation(const FInstancedStruct& ItemToSocket, FGuid ItemToSocketInto, FGuid SocketId)
{
	const FItemInstance* ItemToSocketInstancePtr = ItemToSocket.GetPtr<FItemInstance>();
	if (!ItemToSocketInstancePtr)
	{
		return false;
	}

	const FFastItemInstance* FastItemToSocketIntoPtr = ItemInstances.GetItemInstance(ItemToSocketInto);
	if (!FastItemToSocketIntoPtr)
	{
		return false;
	}

	const FItemInstance* ItemToSocketIntoPtr = FastItemToSocketIntoPtr->ItemInstance.GetPtr<FItemInstance>();
	if (!ItemToSocketIntoPtr)
	{
		return false;
	}

	// Check for any empty sockets. We cant socket at all if they are full already.
	bool bHasEmptySocket = false;
	for (const TInstancedStruct<FItemSocketInstance>& SocketInstance : ItemToSocketIntoPtr->Sockets)
	{
		if (SocketInstance.GetPtr() && SocketInstance.GetPtr()->SocketId == SocketId)
		{
			bHasEmptySocket = SocketInstance.GetPtr()->bIsEmpty;
			break;
		}
	}

	if (!bHasEmptySocket)
	{
		return false;
	}

	const FItemDefinition* const ItemToSocketIntoDefinitionPtr = ItemToSocketIntoPtr->GetItemDefinition().GetPtr();
	if (!ItemToSocketIntoDefinitionPtr)
	{
		return false;
	}

	UItemSocketSettings* const ItemSocketSettingsCDO = ItemToSocketIntoDefinitionPtr->SocketSettings.GetDefaultObject();
	if (!ItemSocketSettingsCDO)
	{
		return false;
	}

	return ItemSocketSettingsCDO->CanSocketInto(ItemToSocket, FastItemToSocketIntoPtr->ItemInstance, SocketId);
}

bool UItemInventoryComponent::HasAnyEmptyItemSockets(FGuid ItemToSocketInto)
{
	const FItemInstance* ItemToSocketIntoInstance = GetItem(ItemToSocketInto);
	if (!ItemToSocketIntoInstance)
	{
		return false;
	}

	for (const TInstancedStruct<FItemSocketInstance>& SocketInstance : ItemToSocketIntoInstance->Sockets)
	{
		if (SocketInstance.GetPtr() && SocketInstance.GetPtr()->bIsEmpty)
		{
			return true;
		}
	}

	return false;
}

bool UItemInventoryComponent::SocketItem(FInstancedStruct& ItemToSocket, FGuid ItemToSocketInto, FGuid SocketId)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!CanSocketItemIntoSocket(ItemToSocket, ItemToSocketInto, SocketId))
	{
		return false;
	}

	bool bResult = false;
	ItemInstances.ModifyItemInstanceWithChangeDescriptor<FItemInstance>(
		ItemToSocketInto,
		GenericItemizationGameplayTags::ItemInstanceChange_SocketChange_Socketed,
		{ GET_MEMBER_NAME_CHECKED(FItemInstance, Sockets) },
		[&ItemToSocket, SocketId, &bResult](FItemInstance* MutableItemInstance)
		{
			for (TInstancedStruct<FItemSocketInstance>& Socket : MutableItemInstance->Sockets)
			{
				FItemSocketInstance* MutableSocketInstance = Socket.GetMutablePtr();
				if (MutableSocketInstance && MutableSocketInstance->SocketId == SocketId)
				{
					MutableSocketInstance->SocketedItemInstance = ItemToSocket;
					MutableSocketInstance->bIsEmpty = false;
					bResult = true;
					return;
				}
			}
		}
	);

	return bResult;
}

bool UItemInventoryComponent::UnsocketItem(FGuid ItemToUnsocketFrom, FGuid SocketId, FInstancedStruct& OutUnsocketedItem)
{
	if (!HasAuthority())
	{
		return false;
	}

	bool bResult = false;
	ItemInstances.ModifyItemInstanceWithChangeDescriptor<FItemInstance>(
		ItemToUnsocketFrom,
		GenericItemizationGameplayTags::ItemInstanceChange_SocketChange_Unsocketed,
		{ GET_MEMBER_NAME_CHECKED(FItemInstance, Sockets) },
		[&OutUnsocketedItem, SocketId, &bResult](FItemInstance* MutableItemInstance)
		{
			for (TInstancedStruct<FItemSocketInstance>& Socket : MutableItemInstance->Sockets)
			{
				FItemSocketInstance* MutableSocketInstance = Socket.GetMutablePtr();
				if (MutableSocketInstance && MutableSocketInstance->SocketId == SocketId)
				{
					OutUnsocketedItem = MutableSocketInstance->SocketedItemInstance;
					MutableSocketInstance->bIsEmpty = true;
					bResult = true;
					return;
				}
			}
		}
	);

	return bResult;
}

bool UItemInventoryComponent::GetItemAffixes(const FGuid& ItemId, TArray<TInstancedStruct<FAffixInstance>>& OutAffixes, bool bIncludeSocketedItems /*= true*/)
{
	bool bFoundItem = false;
	const FInstancedStruct Item = GetItem(ItemId, bFoundItem);
	if (bFoundItem)
	{
		TInstancedStruct<FItemInstance> ItemInstance;
		ItemInstance.InitializeAsScriptStruct(Item.GetScriptStruct(), Item.GetMemory());
		return UGenericItemizationStatics::GetItemAffixes(ItemInstance, OutAffixes, bIncludeSocketedItems);
	}

	return false;
}

TArray<FInstancedStruct> UItemInventoryComponent::GetItems()
{
	return ItemInstances.GetItemInstances();
}

TArray<FFastItemInstance> UItemInventoryComponent::GetItemsWithContext()
{
	TArray<FFastItemInstance> OutItems;
	OutItems.Append(ItemInstances.ItemInstances);
	return OutItems;
}

FInstancedStruct UItemInventoryComponent::GetItem(FGuid ItemId, bool& bSuccessful)
{
	const FFastItemInstance* ItemInstance = ItemInstances.GetItemInstance(ItemId);
	bSuccessful = ItemInstance != nullptr;
	return bSuccessful ? ItemInstance->ItemInstance : FInstancedStruct();
}

const FItemInstance* UItemInventoryComponent::GetItem(const FGuid& ItemId) const
{
	const FFastItemInstance* ItemInstance = ItemInstances.GetItemInstance(ItemId);
	if (!ItemInstance)
	{
		return nullptr;
	}

	return ItemInstance->ItemInstance.GetPtr<FItemInstance>();
}

FInstancedStruct UItemInventoryComponent::GetItemContextData(FGuid ItemId, bool& bSuccessful)
{
	const FFastItemInstance* ItemInstance = ItemInstances.GetItemInstance(ItemId);
	bSuccessful = ItemInstance != nullptr;
	return bSuccessful ? ItemInstance->UserContextData : FInstancedStruct();
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

void UItemInventoryComponent::K2_OnChangedItem_Implementation(const FInstancedStruct& Item, const FInstancedStruct& UserContextData)
{
	// Left empty intentionally to be overridden.
}

void UItemInventoryComponent::K2_OnRemovedItem_Implementation(const FInstancedStruct& Item, const FInstancedStruct& UserContextData)
{
	// Left empty intentionally to be overridden.
}

void UItemInventoryComponent::K2_OnItemStackCountChanged_Implementation(const FInstancedStruct& Item, const FInstancedStruct& UserContextData, int32 OldStackCount, int32 NewStackCount)
{
	// Left empty intentionally to be overridden.
}

void UItemInventoryComponent::K2_OnItemSocketed_Implementation(const FInstancedStruct& Item, const FInstancedStruct& UserContextData, FGuid SocketId)
{
	// Left empty intentionally to be overridden.
}

void UItemInventoryComponent::K2_OnItemUnsocketed_Implementation(const FInstancedStruct& Item, const FInstancedStruct& UserContextData, FGuid SocketId)
{
	// Left empty intentionally to be overridden.
}

void UItemInventoryComponent::OnItemInstancePropertyValueChanged(const FFastItemInstance& FastItemInstance, const FGameplayTag& ChangeDescriptor, int32 ChangeId, const FName& PropertyName, const void* OldPropertyValue, const void* NewPropertyValue)
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

void UItemInventoryComponent::OnChangedItemInstance(const FFastItemInstance& FastItemInstance)
{
	GetOwner()->ForceNetUpdate();

	const FInstancedStruct& ItemInstance = FastItemInstance.ItemInstance;
	OnItemChangedDelegate.Broadcast(this, ItemInstance, FastItemInstance.UserContextData);
	K2_OnChangedItem(ItemInstance, FastItemInstance.UserContextData);
}

void UItemInventoryComponent::OnRemovedItemInstance(const FFastItemInstance& FastItemInstance)
{
	GetOwner()->ForceNetUpdate();

	const FInstancedStruct& ItemInstance = FastItemInstance.ItemInstance;
	OnItemRemovedDelegate.Broadcast(this, ItemInstance, FastItemInstance.UserContextData);
	K2_OnRemovedItem(ItemInstance, FastItemInstance.UserContextData);
}

void UItemInventoryComponent::OnItemInstancePropertyValueChanged_Internal(const FFastItemInstance& FastItemInstance, const FGameplayTag& ChangeDescriptor, int32 ChangeId, const FName& PropertyName, const void* OldPropertyValue, const void* NewPropertyValue)
{
	OnItemInstancePropertyValueChanged(FastItemInstance, ChangeDescriptor, ChangeId, PropertyName, OldPropertyValue, NewPropertyValue);
	OnItemPropertyValueChangedDelegate.Broadcast(this, FastItemInstance, ChangeDescriptor, ChangeId, PropertyName, OldPropertyValue, NewPropertyValue);

	if (ChangeDescriptor == GenericItemizationGameplayTags::ItemInstanceChange_StackCount && PropertyName == GET_MEMBER_NAME_CHECKED(FItemInstance, StackCount))
	{
		const int32* OldStackCount = static_cast<const int32*>(OldPropertyValue);
		const int32* NewStackCount = static_cast<const int32*>(NewPropertyValue);
		OnItemStackCountChangedDelegate.Broadcast(this, FastItemInstance.ItemInstance, FastItemInstance.UserContextData, *OldStackCount, *NewStackCount);
		K2_OnItemStackCountChanged(FastItemInstance.ItemInstance, FastItemInstance.UserContextData, *OldStackCount, *NewStackCount);
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(FItemInstance, Sockets))
	{
		const TArray<TInstancedStruct<FItemSocketInstance>>* OldSockets = static_cast<const TArray<TInstancedStruct<FItemSocketInstance>>*>(OldPropertyValue);
		const TArray<TInstancedStruct<FItemSocketInstance>>* NewSockets = static_cast<const TArray<TInstancedStruct<FItemSocketInstance>>*>(NewPropertyValue);

		FGuid ChangedSocketId;
		bool bFoundChangedSocketId = false;
		for (const TInstancedStruct<FItemSocketInstance>& OldSocket : *OldSockets)
		{
			if (bFoundChangedSocketId)
			{
				break;
			}

			for (const TInstancedStruct<FItemSocketInstance>& NewSocket : *NewSockets)
			{
				const FItemSocketInstance* const OldSocketInstancePtr = OldSocket.GetPtr();
				const FItemSocketInstance* const NewSocketInstancePtr = NewSocket.GetPtr();
				if (OldSocketInstancePtr && NewSocketInstancePtr && OldSocketInstancePtr->SocketId == NewSocketInstancePtr->SocketId)
				{
					// Check if the bIsEmpty flag has changed. If it has then we can skip doing a more expensive compare.
					if (OldSocketInstancePtr->bIsEmpty != NewSocketInstancePtr->bIsEmpty)
					{
						ChangedSocketId = NewSocketInstancePtr->SocketId;
						bFoundChangedSocketId = true;
						break;
					}

					// Since we couldn't determine a change with the bIsEmpty flag we need to check if the ItemInstance itself was changed.
					const FItemInstance* const OldSocketItemInstancePtr = OldSocketInstancePtr->SocketedItemInstance.GetPtr<FItemInstance>();
					const FItemInstance* const NewSocketItemInstancePtr = NewSocketInstancePtr->SocketedItemInstance.GetPtr<FItemInstance>();
					if (OldSocketInstancePtr && !NewSocketItemInstancePtr)
					{
						ChangedSocketId = OldSocketInstancePtr->SocketId;
						bFoundChangedSocketId = true;
						break;
					}

					if (!OldSocketInstancePtr && NewSocketItemInstancePtr)
					{
						ChangedSocketId = NewSocketInstancePtr->SocketId;
						bFoundChangedSocketId = true;
						break;
					}

					if (OldSocketItemInstancePtr && NewSocketItemInstancePtr)
					{
						if(OldSocketItemInstancePtr->ItemId != NewSocketItemInstancePtr->ItemId)
						{
							ChangedSocketId = NewSocketInstancePtr->SocketId;
							bFoundChangedSocketId = true;
							break;
						}
					}
				}
			}
		}

		if(bFoundChangedSocketId)
		{
			if (ChangeDescriptor == GenericItemizationGameplayTags::ItemInstanceChange_SocketChange_Socketed)
			{
				OnItemSocketedDelegate.Broadcast(this, FastItemInstance.ItemInstance, FastItemInstance.UserContextData, ChangedSocketId);
				K2_OnItemSocketed(FastItemInstance.ItemInstance, FastItemInstance.UserContextData, ChangedSocketId);
			}
			else if (ChangeDescriptor == GenericItemizationGameplayTags::ItemInstanceChange_SocketChange_Unsocketed)
			{
				OnItemUnsocketedDelegate.Broadcast(this, FastItemInstance.ItemInstance, FastItemInstance.UserContextData, ChangedSocketId);
				K2_OnItemUnsocketed(FastItemInstance.ItemInstance, FastItemInstance.UserContextData, ChangedSocketId);
			}
		}
	}
}
