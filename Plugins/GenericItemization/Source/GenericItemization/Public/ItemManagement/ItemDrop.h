// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InstancedStruct.h"
#include "GenericItemizationInstanceTypes.h"
#include "ItemDrop.generated.h"

/**
 * An Actor that is representing an actual FItemInstance that has been dropped and can be claimed by a UItemInventoryComponent.
 */
UCLASS(ClassGroup = "Generic Itemization", Abstract, Blueprintable)
class GENERICITEMIZATION_API AItemDrop : public AActor
{
	GENERATED_BODY()
	
public:	

	friend class UItemDropperComponent;
	
	AItemDrop();

	/* Returns the ItemInstance this ItemDrop is representing. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Generic Itemization")
	void GetItemInstance(TInstancedStruct<FItemInstance>& OutItemInstance) const;

protected:

	/* The ItemInstance this ItemDrop is representing. */
	UPROPERTY(Replicated)
	TInstancedStruct<FItemInstance> ItemInstance;

};
