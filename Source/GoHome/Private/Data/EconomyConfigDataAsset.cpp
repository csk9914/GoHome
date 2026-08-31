// 


#include "Data/EconomyConfigDataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

const FCheckPoint* UEconomyConfigDataAsset::FindCheckPoint(int32 Round) const
{
	return CheckPoints.FindByPredicate(
		[Round](const FCheckPoint& CheckPoint)
		{
			return CheckPoint.Round == Round;
		});
}

const FCheckPoint* UEconomyConfigDataAsset::FindNextCheckPoint(int32 Round) const
{
	const FCheckPoint* Next = nullptr;
	for (const FCheckPoint& Rule : CheckPoints)
	{
		if (Rule.Round > Round && (Next == nullptr || Rule.Round < Next->Round))
		{
			Next = &Rule;
		}
	}
	return Next;
}

#if WITH_EDITOR
EDataValidationResult UEconomyConfigDataAsset::IsDataValid(class FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	for (int32 i = 1; i < CheckPoints.Num(); ++i)
	{
		if (CheckPoints[i].Round <= CheckPoints[i - 1].Round)
		{
			Context.AddError(FText::FromString(TEXT("CheckPoints는 Round 오름차순 + 중복 없이여야 함")));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result;
}
#endif
