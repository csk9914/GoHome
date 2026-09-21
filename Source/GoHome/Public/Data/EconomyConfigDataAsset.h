// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/FCheckPoint.h"
#include "EconomyConfigDataAsset.generated.h"

/**
 * UPrimaryDataAsset + Asset Manager 스캔 등록(Config/DefaultGame.ini)으로 참조 그래프와 무관하게
 * 항상 쿡에 포함되도록 함 — GoHomeSaveSubsystem이 경로 문자열로 LoadObject하는데, 순수 UDataAsset이면
 * 아무도 하드 레퍼런스하지 않아 패키징 빌드에서 쿡이 잘라내 LoadObject가 null을 반환했었음(에디터/PIE는
 * 언쿡 상태라 경로만으로도 로드돼 증상이 안 보였음).
 */
UCLASS(BlueprintType)
class GOHOME_API UEconomyConfigDataAsset : public UPrimaryDataAsset
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
