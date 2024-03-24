// Copyright Fissure Entertainment, Pty Ltd. All Rights Reserved.

#include "GenericItemizationTypes.h"
#include "GenericItemizationPickFunctions.h"
#include "GenericItemizationInstancingFunctions.h"
#include "GenericItemizationTags.h"

FItemDropTableCollectionRow::FItemDropTableCollectionRow()
{
	PickRequirements = FInstancedStruct::Make<FItemDropTableCollectionPickRequirements>();
	PickFunction = UItemDropTableCollectionPickFunction::StaticClass();
}

FItemDefinitionCollection::FItemDefinitionCollection()
{
	PickRequirements = FInstancedStruct::Make<FItemDefinitionCollectionPickRequirements>();
	PickFunction = UItemDefinitionCollectionPickFunction::StaticClass();
}

FItemDefinition::FItemDefinition()
{
	InstancingFunction = UItemInstancingFunction::StaticClass();
	SocketableInto.AddTag(GenericItemizationGameplayTags::SocketType);
}

bool FItemDefinition::IsSameItemDefinition(const FItemDefinition& Other) const
{
	return ItemType == Other.ItemType && ItemIdentifier == Other.ItemIdentifier;
}
