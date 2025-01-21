// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InstancedStruct.h"
#include "GenericItemizationTypes.h"
#include "GenericItemizationTableTypes.h"
#include "GenericItemizationInstanceTypes.h"
#include "GenericItemizationInstancingFunctions.generated.h"

class UItemInstancer;
class UAffixPickFunction;

/**
 * Class that manages calculating certain attributes of an ItemInstance.
 */
UCLASS(ClassGroup = ("Generic Itemization"), BlueprintType, Blueprintable)
class UItemInstancingFunction : public UObject
{
	GENERATED_BODY()

public:

	UItemInstancingFunction();

	/* Makes a new instance of an Item. */
	virtual void MakeItemInstance(const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItemInstance) const;

	/* Makes a new instance of an Affix. */
	virtual void MakeAffixInstance(const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutAffixInstance) const;

	/* Returns the calculated Affix Level for the ItemInstance. The default implementation uses the ItemLevel and QualityLevel to determine AffixLevel. */
	UFUNCTION(BlueprintNativeEvent)
	bool CalculateAffixLevel(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, int32& OutAffixLevel) const;

	/* Returns a weighted random Item Quality Type for the ItemInstance. The default implementation uses the ItemQualityRatios table. */
	UFUNCTION(BlueprintNativeEvent)
	bool SelectItemQualityType(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, FGameplayTag& OutQualityType) const;

	/* Returns how many Affixes the ItemInstance needs to generate. */
	UFUNCTION(BlueprintNativeEvent)
	bool DetermineAffixCount(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, int32& OutAffixCount) const;

	/* Returns the size of the StackCount the ItemInstance will have. The StackCount must be >= 1. */
	UFUNCTION(BlueprintNativeEvent)
	bool CalculateStackCount(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, int32& OutStackCount) const;

	/* Returns all of the SocketDefinitions on the ItemSocketSettings that will be set to Active on the ItemInstance when its generated. */
	UFUNCTION(BlueprintNativeEvent)
	bool DetermineActiveSockets(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext, TArray<int32>& OutActiveSocketDefinitions) const;

	/* Returns the Maximum Item Level. */
	int32 GetMaximumItemLevel() const { return MaximumItemLevel; }

	/* The Pick Function that describes how ItemInstances will choose which Affixes they will have applied to them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UAffixPickFunction> AffixPickFunction;

protected:

	/* The maximum value of ItemLevel that an ItemInstance can have. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaximumItemLevel = 99;

};

/**
 * Class that manages building an Item Instancing Context.
 * 
 * Override to provide your own context. 
 * You may want to pass data important to you through the UserContextData and embed it in the Item Instancing Context.
 * For interpretation elsewhere in the Item Instancing Process or other external systems.
 */
UCLASS(ClassGroup = ("Generic Itemization"), BlueprintType, Blueprintable)
class UItemInstancingContextFunction : public UObject
{
	GENERATED_BODY()

public:

	/* Returns the ItemInstancingContext that will be passed into the Item Instancing Process to give context to that functionality around the Item being instanced. */
	UFUNCTION(BlueprintNativeEvent)
	bool BuildItemInstancingContext(const UItemInstancer* ItemInstancer, const FInstancedStruct& UserContextData, FInstancedStruct& OutItemInstancingContext);

};

