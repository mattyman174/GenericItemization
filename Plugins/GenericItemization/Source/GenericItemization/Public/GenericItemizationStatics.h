// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenericItemizationTypes.h"
#include "GenericItemizationInstanceTypes.h"
#include "GenericItemizationStatics.generated.h"

/**
 * Static class with useful functions that can be called from both Blueprint and C++.
 */
UCLASS()
class GENERICITEMIZATION_API UGenericItemizationStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Picks a single ItemDropTableType entry from the passed in Collection entry.
	 * 
	 * The Optional will not be set if a NoPick occurred, based on the NoPickChance and the ItemDropTableTypes PickChances.
	 * 
	 * @param DropTableCollectionEntry	The Entry to make a selection from.
	 * @param ItemInstancingContext		Contains information about the Context around which an ItemInstance is called to be generated.
	 * @param bIncludeNoPick			True if the NoPickChance should be included in the pick entries.
	 */
	static TOptional<TInstancedStruct<FItemDropTableType>> PickDropTableCollectionEntry(const TInstancedStruct<FItemDropTableCollectionRow>& DropTableCollectionEntry, const FInstancedStruct& ItemInstancingContext, bool bIncludeNoPick);

	/**
	 * Picks a single ItemDefinition from the passed in ItemDefinitionCollection based on its PickRequirements and PickFunction.
	 *
	 * The Optional will not be set if a Pick could not be made. Generally this should only occur if no ItemDefinition satisfied the requirements.
	 * 
	 * @param ItemDefinitionCollection	The collection of ItemDefinitions we want to make a selection from.
	 * @param ItemInstancingContext		Contains information about the Context around which an ItemInstance is called to be generated.
	 */
	static TOptional<FDataTableRowHandle> PickItemDefinitionFromCollection(const TInstancedStruct<FItemDefinitionCollection>& ItemDefinitionCollection, const FInstancedStruct& ItemInstancingContext);

	/**
	 * Picks the ItemDefinition from the passed in entry.
	 * 
	 * The Optional will not be set if the ItemDefinition could not be selected. Generally this will only happen if it is marked as not spawnable.
	 * 
	 * @param ItemDefinitionEntry	The entry of the particular ItemDefinition we are returning.
	 * @param ItemInstancingContext	Contains information about the Context around which an ItemInstance is called to be generated.
	 */
	static TOptional<FDataTableRowHandle> PickItemDefinitionEntry(const TInstancedStruct<FItemDefinitionRow>& ItemDefinitionEntry, const FInstancedStruct& ItemInstancingContext);

	/**
	 * Gets a handle to a single AffixDefinition for the passed in ItemInstance that meets its requirements.
	 * 
	 * @param ItemInstance				The actual Item that will be getting the Affix applied to it.
	 * @param ItemInstancingContext		Contains information about the Context around which an ItemInstance is called to be generated.
	 */
	static TOptional<FDataTableRowHandle> PickAffixDefinitionForItemInstance(const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext);

	/**
	 * Generates a new Affix Instance from the passed in AffixDefinition.
	 * 
	 * @param AffixDefinitionHandle		A handle to the static information the AffixInstance will use to understand how it is to generate.
	 * @param ItemInstance				The ItemInstance that the AffixInstance will be applied to and is being generated for.
	 * @param ItemInstancingContext		Contains information about the Context around which an ItemInstance is called to be generated.
	 */
	static TOptional<TInstancedStruct<FAffixInstance>> GenerateAffixInstanceFromAffixDefinition(const FDataTableRowHandle& AffixDefinitionHandle, const FInstancedStruct& ItemInstance, const FInstancedStruct& ItemInstancingContext);

	/**
	 * Picks Item Definitions according to the passed in DropTable. The DropTable entry will determine how many Picks are to be attempted and from what tables selections will come from.
	 * 
	 * @param ItemDropTableCollectionEntry		The DropTable we will be using to make the selection from and to understand how many Picks to make. Expects the Data Table Row Type to be `FItemDropTableCollectionEntry`.
     * @param ItemInstancingContext				Contains information about the Context around which an ItemInstance is called to be generated.
	 * @param OutItemDefinitions				The ItemDefinitions that were selected from the DropTable. Not guaranteed to contains the PickCount number of Items, as the NoPickChance can cause empty Picks to occur. Will be type of `FItemDefinition`.
	 * @return									False if no picks were made.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generic Itemization")
	static bool PickItemDefinitionsFromDropTable(const FDataTableRowHandle& ItemDropTableCollectionEntry, const FInstancedStruct& ItemInstancingContext, TArray<FDataTableRowHandle>& OutItemDefinitionHandles);

	/**
	 * Generates a new Item Instance from the passed in ItemDefinition and ItemLevel.
	 * 
	 * @param ItemDefinition		The static information the ItemInstance will use to understand how it is to generate.
	 * @param ItemInstancingContext	Contains information about the Context around which an ItemInstance is called to be generated.
	 * @param OutItemInstance		The generated ItemInstance.
	 * @return						False if an ItemInstance could not be generated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generic Itemization")
	static bool GenerateItemInstanceFromItemDefinition(const FDataTableRowHandle& ItemDefinitionHandle, const FInstancedStruct& ItemInstancingContext, FInstancedStruct& OutItemInstance);

	/**
	 * Makes an exact copy of the ItemInstance except for its ItemId, ItemSeed and ItemStream, these are all regenerated.
	 * 
	 * @param ItemInstanceTemplate		The ItemInstance to make a copy of as a template.
	 * @param OutItemInstanceCopy		The new ItemInstance that was copied, with new unique Id info.
	 * @return							True if we could generate the ItemInstance copy successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generic Itemization")
	static bool GenerateItemInstanceFromTemplate(const FInstancedStruct& ItemInstanceTemplate, FInstancedStruct& OutItemInstanceCopy);

};
