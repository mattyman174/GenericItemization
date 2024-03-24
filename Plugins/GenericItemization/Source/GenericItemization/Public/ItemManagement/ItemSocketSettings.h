// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InstancedStruct.h"
#include "GameplayTagContainer.h"
#include "StructView.h"
#include "ItemSocketSettings.generated.h"

/**
 * Defines a Socket that ItemInstances can be placed into.
 */
USTRUCT(BlueprintType)
struct FItemSocketDefinition
{
    GENERATED_BODY()

public:

    /* The type of Socket this is. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.SocketType"))
    FGameplayTag SocketType;

	/* The ItemTypes that can be placed into this Socket. Leaving this empty means any ItemType will be accepted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.ItemType"))
    FGameplayTagContainer AcceptsItemTypes;

	/* The Item QualityTypes that can be placed into this Socket. Leaving this empty means any QualityType will be accepted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.QualityType"))
    FGameplayTagContainer AcceptsQualityTypes;

	/* Identifies this Socket uniquely. */
	FGuid SocketDefinitionHandle = FGuid::NewGuid();

};

/**
 * Describes what Sockets an ItemInstance will have.
 */
UCLASS(ClassGroup = ("Generic Itemization"), Blueprintable, Abstract)
class UItemSocketSettings : public UObject
{
	GENERATED_BODY()

public:

	friend class UItemInstancingFunction;
	friend struct FSetSocketInstanceSocketDefinition;

	UItemSocketSettings();

	/**
	 * Checks if ItemToSocket can be socketed into the SocketInstance on ItemToSocketInto.
	 * 
	 * Restrictions on Socketing are as follows:
	 *		ItemToSocketInto must be socketable. Obviously.
	 *		ItemToSocket cannot itself be socketable. Socket depth is therefore always 1.
	 *		ItemToSocket cannot be the same ItemInstance as ItemToSocketInto. Logically we cannot socket an ItemInstance into itself.
	 * 
	 * @param ItemToSocket			The ItemInstance that we want to socket into the SocketInstance.
	 * @param ItemToSocketInto		The ItemInstance that owns the SocketInstance.
	 * @param SocketId				The Id of the SocketInstance of the ItemToSocketInto that we want to check if ItemToSocket can be socketed into.
	 * @return						True if we can socket the ItemInstance into the others SocketInstance.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item Stack Settings")
	bool CanSocketInto(const FInstancedStruct& ItemToSocket, const FInstancedStruct& ItemToSocketInto, const FGuid& SocketId);

	/* Returns all of the SocketDefinitions corresponding to the array indexes. */
	TArray<TInstancedStruct<FItemSocketDefinition>> GetSocketDefinitions(TArray<int32> SocketDefinitionIndexes) const;
	TArray<FConstStructView> GetSocketDefinitions() const;

protected:

	/* All of the Sockets that an ItemInstance with these settings can have. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.ItemSocketDefinition"), Category = "Item Socket Settings")
	TArray<TInstancedStruct<FItemSocketDefinition>> SocketDefinitions;

	/* Returns all of the SocketDefinitions on the ItemSocketSettings that will be set to Active on the ItemInstance when its generated. Default implementation returns nothing. */
	UFUNCTION(BlueprintNativeEvent)
	bool DetermineActiveSockets(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, TArray<int32>& OutActiveSocketDefinitions) const;

};

