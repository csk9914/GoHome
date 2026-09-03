// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/FCheckPoint.h"
#include "EconomyConfigDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class GOHOME_API UEconomyConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	const FCheckPoint* FindCheckPoint(int32 Round) const;
	const FCheckPoint* FindNextCheckPoint(int32 Round) const;

	// 게임 총 턴 수
	int32 GetFinalRound() const { return CheckPoints.Num() ? CheckPoints.Last().Round : 0; }

	// 엔딩 판정
	bool IsEndingRound(int32 Round) const{return CheckPoints.Num() && Round == CheckPoints.Last().Round;}
	
public:
	// 체크 포인트 목표 할당량 (9라운드 엔딩)
	UPROPERTY(EditDefaultsOnly, Category = "Save")
	TArray<FCheckPoint> CheckPoints;

	// 사망 패널티 차감액 (1인)
	UPROPERTY(EditDefaultsOnly, Category = "Save")
	int32 CasualtyFee = 500;


#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
