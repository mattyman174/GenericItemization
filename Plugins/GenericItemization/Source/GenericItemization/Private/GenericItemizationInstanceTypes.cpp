#include "GenericItemizationInstanceTypes.h"

bool FItemInstance::HasAnyAffixOfType(const FGameplayTag& AffixType) const
{
	for (const TInstancedStruct<FAffixInstance>& AffixInstance : Affixes)
	{
		if (AffixInstance.IsValid() && AffixInstance.Get().AffixDefinition.IsValid())
		{
			if (AffixType.MatchesTag(AffixInstance.Get().AffixDefinition.Get().AffixType))
			{
				return true;
			}
		}
	}

	return false;
}
