#include "Player/OxygenDrainBoostConfig.h"
#include "GameFramework/Actor.h"

namespace
{
	int32 GetClassDistanceToBase(const UClass* DerivedClass, const UClass* BaseClass)
	{
		if (!DerivedClass || !BaseClass || !DerivedClass->IsChildOf(BaseClass))
		{
			return INDEX_NONE;
		}

		int32 Distance = 0;
		for (const UClass* CurrentClass = DerivedClass; CurrentClass; CurrentClass = CurrentClass->GetSuperClass())
		{
			if (CurrentClass == BaseClass)
			{
				return Distance;
			}

			++Distance;
		}

		return INDEX_NONE;
	}
}

float UOxygenDrainBoostConfig::GetDrainMultiplierForInstigator(const AActor* InstigatorActor) const
{
	const float DefaultMultiplier = FMath::Max(1.f, DefaultRule.DrainMultiplier);

	if (!IsValid(InstigatorActor))
	{
		return DefaultMultiplier;
	}

	const UClass* InstigatorClass = InstigatorActor->GetClass();
	float SelectedMultiplier = DefaultMultiplier;
	int32 BestDistance = MAX_int32;

	for (const TPair<TSubclassOf<AActor>, FOxygenDrainBoostRule>& RulePair : MonsterRules)
	{
		const UClass* RuleClass = RulePair.Key.Get();
		const int32 Distance = GetClassDistanceToBase(InstigatorClass, RuleClass);

		if (Distance != INDEX_NONE && Distance < BestDistance)
		{
			BestDistance = Distance;
			SelectedMultiplier = FMath::Max(1.f, RulePair.Value.DrainMultiplier);
		}
	}

	return SelectedMultiplier;
}