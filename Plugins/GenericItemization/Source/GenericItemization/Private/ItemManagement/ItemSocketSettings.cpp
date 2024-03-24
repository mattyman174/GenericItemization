// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "ItemManagement/ItemSocketSettings.h"
#include "GenericItemizationInstanceTypes.h"

UItemSocketSettings::UItemSocketSettings()
{

}

bool UItemSocketSettings::CanSocketInto_Implementation(const FInstancedStruct& ItemToSocket, const FInstancedStruct& ItemToSocketInto, const FGuid& SocketId)
{
	const FItemInstance* const ItemToSocketPtr = ItemToSocket.GetPtr<const FItemInstance>();
	const FItemInstance* const ItemToSocketIntoPtr = ItemToSocketInto.GetPtr<const FItemInstance>();
	if (!ItemToSocketPtr || !ItemToSocketIntoPtr)
	{
		return false;
	}

	// Make sure we have the Socket being requested.
	TOptional<const FConstStructView> SocketInstanceViewResult = ItemToSocketIntoPtr->GetSocket(SocketId);
	if (!SocketInstanceViewResult.IsSet())
	{
		return false;
	}

	const FConstStructView& SocketInstanceView = *SocketInstanceViewResult;
	const FItemSocketInstance* const SocketInstancePtr = SocketInstanceView.GetPtr<const FItemSocketInstance>();
	if (!SocketInstancePtr || !SocketInstancePtr->bIsEmpty)
	{
		return false;
	}

	// We cannot Socket an ItemInstance into itself.
	if (ItemToSocketPtr->ItemId == ItemToSocketIntoPtr->ItemId)
	{
		return false;
	}

	const TInstancedStruct<FItemDefinition>& ItemToSocketDefinitionInstance = ItemToSocketPtr->GetItemDefinition();
	const TInstancedStruct<FItemDefinition>& ItemToSocketIntoDefinitionInstance = ItemToSocketIntoPtr->GetItemDefinition();
	const TInstancedStruct<FItemSocketDefinition>& SocketDefinitionInstance = SocketInstancePtr->GetSocketDefinition();
	if (!ItemToSocketDefinitionInstance.IsValid() || !ItemToSocketIntoDefinitionInstance.IsValid())
	{
		return false;
	}

	const FItemDefinition& ItemToSocketDefinition = ItemToSocketDefinitionInstance.Get();
	const FItemDefinition& ItemToSocketIntoDefinition = ItemToSocketIntoDefinitionInstance.Get();
	const FItemSocketDefinition& SocketDefinition = SocketDefinitionInstance.Get();
	
	// We cannot Socket anything if we are not socketable.
	// We cannot Socket a socketable Item.
	if (ItemToSocketIntoDefinition.bStacksOverSockets && !ItemToSocketDefinition.bStacksOverSockets)
	{
		return false;
	}

	// Check our requirements are met.
	if (!ItemToSocketDefinition.SocketableInto.IsEmpty() && !ItemToSocketDefinition.SocketableInto.HasTag(SocketDefinition.SocketType))
	{
		return false;
	}

	if (!SocketDefinition.AcceptsItemTypes.IsEmpty() && !SocketDefinition.AcceptsItemTypes.HasTag(ItemToSocketDefinition.ItemType))
	{
		return false;
	}

	if (!SocketDefinition.AcceptsQualityTypes.IsEmpty() && !SocketDefinition.AcceptsQualityTypes.HasTag(ItemToSocketPtr->QualityType))
	{
		return false;
	}

	return true;
}

TArray<TInstancedStruct<FItemSocketDefinition>> UItemSocketSettings::GetSocketDefinitions(TArray<int32> SocketDefinitionIndexes) const
{
	TArray<TInstancedStruct<FItemSocketDefinition>> OutSocketDefinitions;
	for (const int32& Index : SocketDefinitionIndexes)
	{
		if (SocketDefinitions.IsValidIndex(Index))
		{
			OutSocketDefinitions.Add(SocketDefinitions[Index]);
		}
	}

	return OutSocketDefinitions;
}

TArray<FConstStructView> UItemSocketSettings::GetSocketDefinitions() const
{
	TArray<FConstStructView> Result;
	for (const TInstancedStruct<FItemSocketDefinition>& SocketDefinition : SocketDefinitions)
	{
		Result.Add(FConstStructView(SocketDefinition.GetScriptStruct(), SocketDefinition.GetMemory()));
	}

	return Result;
}

bool UItemSocketSettings::DetermineActiveSockets_Implementation(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, TArray<int32>& OutActiveSocketDefinitions) const
{
	OutActiveSocketDefinitions.Empty();
	return true;
}