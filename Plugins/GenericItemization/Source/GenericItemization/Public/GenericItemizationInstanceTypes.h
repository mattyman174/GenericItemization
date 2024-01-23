// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericItemizationTypes.h"
#include "GenericItemizationInstanceTypes.generated.h"

/************************************************************************/
/* Affixes
/************************************************************************/

/**
 * An actual instance of an Affix that exists on an FItemInstance.
 */
USTRUCT(BlueprintType)
struct FAffixInstance
{
    GENERATED_BODY()

public:

    /* The static data that describes this Affix. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TInstancedStruct<FAffixDefinition> AffixDefinition;

};

/************************************************************************/
/* Items
/************************************************************************/

/**
 * Contains information about the Context around which an ItemInstance is called to be generated.
 */
USTRUCT(BlueprintType)
struct FItemInstancingContext
{
    GENERATED_BODY()

public:

    /* The overall level that the ItemInstance will be. This goes on to affect the types of and magnitude of attributes the ItemInstance can have among other things. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UIMin = "0", ClampMin = "0"))
    int32 ItemLevel = 1;

    /* The amount of Magic Find bonus was snapshot in this context, this contributes to the tendency to find certain Item Quality Types. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UIMin = "0", ClampMin = "0"))
    int32 MagicFind = 1;

};

/**
 * An actual instance of an Item that was generated.
 */
USTRUCT(BlueprintType)
struct FItemInstance
{
    GENERATED_BODY()

public:

    /* The static data that describes this Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TInstancedStruct<FItemDefinition> ItemDefinition;

    /* The authoritative seed that was generated, when this Item was created during the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 ItemSeed;

    /* A random stream initialized with the ItemSeed when this Item was created. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FRandomStream ItemStream;

    /* The calculated level, of this Item when it was created during the Item Instancing Process. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 ItemLevel;

    /* The calculated level for Affixes that can appear on this Item, this is determined by the ItemLevel and the QualityLevel. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 AffixLevel;

    /* The Quality Type of this Item, this helps determine what types of Affixes and other attributes can occur on this Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.QualityType"))
    FGameplayTag QualityType;

    /* All of the actual Affixes that this Item has that would apply attribute modifiers from this Item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BaseStruct = "/Script/GenericItemization.AffixInstance"))
    TArray<TInstancedStruct<FAffixInstance>> Affixes;

    /* The Context information around which this ItemInstance was called to be generated. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TInstancedStruct<FItemInstancingContext> InstancingContext;

    bool HasAnyAffixOfType(const FGameplayTag& AffixType) const;

};