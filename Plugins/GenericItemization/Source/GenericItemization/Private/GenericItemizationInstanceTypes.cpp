#include "GenericItemizationInstanceTypes.h"
#include "ItemManagement/ItemInventoryComponent.h"

/************************************************************************/
/* Affixes
/************************************************************************/

bool FAffixInstance::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << bPredefinedAffix;
	Ar << AffixDefinitionHandle.DataTable;
	Ar << AffixDefinitionHandle.RowName;

	if (Ar.IsLoading())
	{
		SetAffixDefinition(AffixDefinitionHandle);
	}

	bOutSuccess = true;
	return true;
}

void FAffixInstance::SetAffixDefinition(const FDataTableRowHandle& Handle)
{
	AffixDefinitionHandle = Handle;
	if (!AffixDefinitionHandle.IsNull())
	{
		const FAffixDefinitionEntry* AffixDefinitionEntry = AffixDefinitionHandle.GetRow<FAffixDefinitionEntry>(FString());
		if (AffixDefinitionEntry)
		{
			AffixDefinition = AffixDefinitionEntry->AffixDefinition;
		}
	}
}

/************************************************************************/
/* Items
/************************************************************************/

FItemSocketInstance::FItemSocketInstance()
{
	SocketId = FGuid::NewGuid();
	bIsEmpty = false;
}

const FConstStructView FItemSocketInstance::GetSocketedItem() const
{
	return FConstStructView(SocketedItemInstance);
}

FItemInstance::FItemInstance()
{
	ItemId = FGuid::NewGuid();
	ItemSeed = -1;
	ItemLevel = -1;
	AffixLevel = -1;
	StackCount = 1;
}

bool FItemInstance::HasAnyAffixOfType(const FGameplayTag& AffixType) const
{
	for (const TInstancedStruct<FAffixInstance>& AffixInstance : Affixes)
	{
		if (AffixInstance.IsValid() && AffixInstance.Get().GetAffixDefinition().IsValid())
		{
			if (AffixType.MatchesTag(AffixInstance.Get().GetAffixDefinition().Get().AffixType))
			{
				return true;
			}
		}
	}

	return false;
}

bool FItemInstance::IsValid() const
{
	return ItemSeed != -1;
}

bool FItemInstance::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << ItemId;
	Ar << ItemSeed;
	Ar << ItemLevel;
	Ar << AffixLevel;
	Ar << QualityType;
	Ar << StackCount;

	// Ar << ItemDefinitionHandle;
	Ar << ItemDefinitionHandle.DataTable;
	Ar << ItemDefinitionHandle.RowName;
	if (Ar.IsLoading())
	{
		SetItemDefinition(ItemDefinitionHandle);
	}

	//Ar << ItemStream;
	int32 ItemStreamSeed;
	if (Ar.IsSaving())
	{
		ItemStreamSeed = ItemStream.GetCurrentSeed();
	}
	Ar << ItemStreamSeed;
	if (Ar.IsLoading())
	{
		ItemStream.Initialize(ItemStreamSeed);
	}

	// Ar << Affixes;
	TArray<FInstancedStruct> ReplicatedAffixes;
	if (Ar.IsSaving())
	{
		for (const TInstancedStruct<FAffixInstance>& Affix : Affixes)
		{
			FInstancedStruct ReplicatedAffix;
			ReplicatedAffix.InitializeAs(Affix.GetScriptStruct(), Affix.GetMemory());
			ReplicatedAffixes.Add(ReplicatedAffix);
		}
	}
	SafeNetSerializeTArray_WithNetSerialize<31>(Ar, ReplicatedAffixes, Map); // @NOTE: This means we have a practical maximum of 32 Affixes per ItemInstance. Should we expose this somehow?
	if (Ar.IsLoading())
	{
		// We need to empty the actual array as we are effectively rebuilding it.
		Affixes.Empty(ReplicatedAffixes.Num());
		for (const FInstancedStruct& ReplicatedAffix : ReplicatedAffixes)
		{
			TInstancedStruct<FAffixInstance> Affix;
			Affix.InitializeAsScriptStruct(ReplicatedAffix.GetScriptStruct(), ReplicatedAffix.GetMemory());
			Affixes.Add(Affix);
		}
	}

	// Ar << Sockets;
	TArray<FInstancedStruct> ReplicatedSockets;
	if (Ar.IsSaving())
	{
		for (const TInstancedStruct<FItemSocketInstance>& Socket : Sockets)
		{
			FInstancedStruct ReplicatedSocket;
			ReplicatedSocket.InitializeAs(Socket.GetScriptStruct(), Socket.GetMemory());
			ReplicatedSockets.Add(ReplicatedSocket);
		}
	}
	SafeNetSerializeTArray_WithNetSerialize<31>(Ar, ReplicatedSockets, Map); // @NOTE: This means we have a practical maximum of 32 Sockets per ItemInstance. Should we expose this somehow?
	if (Ar.IsLoading())
	{
		// We need to empty the actual array as we are effectively rebuilding it.
		Sockets.Empty(ReplicatedSockets.Num());
		for (const FInstancedStruct& ReplicatedSocket : ReplicatedSockets)
		{
			TInstancedStruct<FItemSocketInstance> Socket;
			Socket.InitializeAsScriptStruct(ReplicatedSocket.GetScriptStruct(), ReplicatedSocket.GetMemory());

			AddSocket(Socket);
		}
	}

	bOutSuccess = true;
	return true;
}

void FItemInstance::SetItemDefinition(const FDataTableRowHandle& Handle)
{
	ItemDefinitionHandle = Handle;
	if (!ItemDefinitionHandle.IsNull())
	{
		const FItemDefinitionEntry* ItemDefinitionEntry = ItemDefinitionHandle.GetRow<FItemDefinitionEntry>(FString());
		if (ItemDefinitionEntry)
		{
			ItemDefinition = ItemDefinitionEntry->ItemDefinition;
		}
	}
}

void FItemInstance::AddSocket(TInstancedStruct<FItemSocketInstance>& NewSocket)
{
	// Let the Socket know what its definition is.
	FSetSocketInstanceSocketDefinition(ItemDefinition.GetPtr(), NewSocket.GetMutablePtr());
	Sockets.Add(NewSocket);
}

TOptional<const FConstStructView> FItemInstance::GetSocket(const FGuid SocketId) const
{
	TOptional<const FConstStructView> Result;
	for (const TInstancedStruct<FItemSocketInstance>& Socket : Sockets)
	{
		if (Socket.Get().SocketId == SocketId)
		{
			Result.Emplace(FConstStructView(Socket.GetScriptStruct(), Socket.GetMemory()));
		}
	}

	return Result;
}

bool FItemInstance::HasSocket(const FGuid SocketId) const
{
	for (const TInstancedStruct<FItemSocketInstance>& Socket : Sockets)
	{
		if (Socket.Get().SocketId == SocketId)
		{
			return true;
		}
	}

	return false;
}

void FFastItemInstance::Initialize(const FInstancedStruct& InItemInstance, const FInstancedStruct& InUserContextData)
{
	ItemInstance = InItemInstance;
	UserContextData = InUserContextData;

	// Make a copy of the ItemInstance for change comparison.
	PreReplicatedChangeItemInstance.InitializeAs(ItemInstance.GetScriptStruct(), ItemInstance.GetMemory());
}

void FFastItemInstance::PostReplicatedAdd(const struct FFastItemInstancesContainer& InArray)
{
	// Update our cached state.
	PreReplicatedChangeItemInstance.InitializeAs(ItemInstance.GetScriptStruct(), ItemInstance.GetMemory());
	PreviousChangesId = RecentChangesId;
}

void FFastItemInstance::PostReplicatedChange(const struct FFastItemInstancesContainer& InArray)
{

}

void FFastItemInstance::PreReplicatedRemove(const struct FFastItemInstancesContainer& InArray)
{

}

void FFastItemInstancesContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (const int32& Index : AddedIndices)
	{
		const FFastItemInstance& FastItemInstance = ItemInstances[Index];
		const FInstancedStruct& PostAddItemInstance = FastItemInstance.ItemInstance;
		if (Owner)
		{
			Owner->OnAddedItemInstance(FastItemInstance);
		}
	}
}

void FFastItemInstancesContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	for (const int32& Index : ChangedIndices)
	{
		FFastItemInstance& FastItemInstance = ItemInstances[Index];
		OnItemInstanceChanged(FastItemInstance);
	}
}

void FFastItemInstancesContainer::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (const int32& Index : RemovedIndices)
	{
		const FFastItemInstance& FastItemInstance = ItemInstances[Index];
		const FInstancedStruct& PreRemoveItemInstance = FastItemInstance.ItemInstance;
		if (Owner)
		{
			Owner->OnRemovedItemInstance(FastItemInstance);
		}
	}
}

void FFastItemInstancesContainer::Register(UItemInventoryComponent* InOwner)
{
	if (Owner != InOwner && InOwner != nullptr)
	{
		Owner = InOwner;
		bOwnerIsNetAuthority = Owner->HasAuthority();
	}
}

void FFastItemInstancesContainer::AddItemInstance(FInstancedStruct& ItemInstance, FInstancedStruct& UserContextData)
{
	if (!Owner)
	{
		return;
	}

	FFastItemInstance FastItemInstance;
	FastItemInstance.Initialize(ItemInstance, UserContextData);
	ItemInstances.Add(FastItemInstance);

	if (HasAuthority())
	{
		Owner->OnAddedItemInstance(FastItemInstance);
		MarkItemDirty(FastItemInstance);
	}
}

void FFastItemInstancesContainer::OnItemInstanceChanged(FFastItemInstance& ChangedItemInstance) const
{
	DiffItemInstanceChanges(ChangedItemInstance);

	if (Owner)
	{
		Owner->OnChangedItemInstance(ChangedItemInstance);
	}

	// Update our cached state.
	ChangedItemInstance.PreReplicatedChangeItemInstance.InitializeAs(ChangedItemInstance.ItemInstance.GetScriptStruct(), ChangedItemInstance.ItemInstance.GetMemory());
	ChangedItemInstance.PreviousChangesId = ChangedItemInstance.RecentChangesId;
}

bool FFastItemInstancesContainer::RemoveItemInstance(const FGuid& Item)
{
	for (int32 i = ItemInstances.Num() - 1; i >= 0; --i)
	{
		const FItemInstance* ItemInstancePtr = ItemInstances[i].ItemInstance.GetPtr<FItemInstance>();
		if (ItemInstancePtr && ItemInstancePtr->IsValid() && ItemInstancePtr->ItemId == Item)
		{
			FFastItemInstance OldItemInstance = ItemInstances[i];
			ItemInstances.RemoveAt(i);

			if(HasAuthority())
			{
				Owner->OnRemovedItemInstance(OldItemInstance);
				MarkArrayDirty();
			}

			return true;
		}
	}

	return false;
}

TArray<FInstancedStruct> FFastItemInstancesContainer::GetItemInstances() const
{
	TArray<FInstancedStruct> Result;
	Result.Reserve(ItemInstances.Num());

	for (const FFastItemInstance& ItemInstance : ItemInstances)
	{
		Result.Add(ItemInstance.ItemInstance);
	}

	return Result;
}

const FFastItemInstance* FFastItemInstancesContainer::GetItemInstance(const FGuid& Item) const
{
	for (const FFastItemInstance& FastItemInstance : ItemInstances)
	{
		const FItemInstance* ItemInstancePtr = FastItemInstance.ItemInstance.GetPtr<FItemInstance>();
		if (ItemInstancePtr && ItemInstancePtr->ItemId == Item)
		{
			return &FastItemInstance;
		}
	}

	return nullptr;
}

int32 FFastItemInstancesContainer::GetNum() const
{
	return ItemInstances.Num();
}

void FFastItemInstancesContainer::DiffItemInstanceChanges(const FFastItemInstance& ChangedItemInstance) const
{
	if(IsValid(Owner))
	{
		for (const FItemInstanceChange& RecentChange : ChangedItemInstance.RecentChangesBuffer)
		{
			if (RecentChange.ChangeId > ChangedItemInstance.PreviousChangesId)
			{
				for (const FName& PropertyName : RecentChange.ChangedProperties)
				{
					const FProperty* OldProperty = nullptr;
					const void* OldPropertyData = nullptr;
					const bool bFoundOldProperty = ChangedItemInstance.PreReplicatedChangeItemInstance.FindInnerPropertyInstance(PropertyName, OldProperty, OldPropertyData);

					const FProperty* NewProperty = nullptr;
					const void* NewPropertyData = nullptr;
					const bool bFoundNewProperty = ChangedItemInstance.ItemInstance.FindInnerPropertyInstance(PropertyName, NewProperty, NewPropertyData);

					if (bFoundOldProperty && bFoundNewProperty)
					{
						const void* OldPropertyValue = OldProperty->ContainerPtrToValuePtr<const void>(OldPropertyData);
						const void* NewPropertyValue = NewProperty->ContainerPtrToValuePtr<const void>(NewPropertyData);
						Owner->OnItemInstancePropertyValueChanged_Internal(ChangedItemInstance, RecentChange.ChangeDescriptor, RecentChange.ChangeId, PropertyName, OldPropertyValue, NewPropertyValue);
					}
				}
			}
		}
	}
}
