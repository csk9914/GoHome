

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Upgrade/EquipmentUpgradeTypes.h"
#include "Shop/ItemShopTypes.h"
#include "GoHomeSaveGame.generated.h"

/** 호스트 로컬 세이브: 파티 공유 재화, 구매 완료 업그레이드 목록, 마지막 진행 지점. */
UCLASS()
class GOHOME_API UGoHomeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 자금, (납품 +, 강화 or 구매 -
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 CurrentFunds = 1000;

	// 이번 라운드 누적 납품액, 판정 후 0으로 리셋
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 CurrentRoundDeliveredValue = 0;
	
	// 완료한 라운드 수
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 CurrentRound = 0;
	
	// 할당량 누적 미달 횟수(연속 아님, 성공해도 리셋 안 됨). 
	// 3회(3스트라이크) 도달 시 게임오버로 세이브 전체 초기화
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 QuotaMissCount = 0;

	// 강화
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FName> PurchasedUpgrades;

	// 업그레이드별 현재 강화 레벨 저장(강화 ID, 현재 레벨)
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FEquipmentUpgradeLevelState> SavedUpgradeLevels;

	// 마지막 도달 진행 지점
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName LastProgressPoint;
	
	// 세이브 스키마 버전. 이 필드가 없던 옛 세이브는 로드 시 0으로 읽혀 마이그레이션 대상이 된다
	// (그래서 기본값은 0이고, 새로 만드는 세이브만 UGoHomeSaveSubsystem이 CurrentSaveVersion을 넣는다).
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 SaveVersion = 0;

	// 1: 플레이어별 상점 보유 기록 → 잠수정 공유 보관함
	static constexpr int32 CurrentSaveVersion = 1;

	// 잠수정 공유 보관함 — 팀 전체 상품 보유 수량(영구 장비 + 소모품). 게임 초기화(ResetSave) 시 함께 비워진다.
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	TArray<FSharedLockerEntry> SharedLockerItems;

	// 이번 라운드 팀 구매 수량(MaxPurchasesPerRun 검증). FinalizeRound마다 비운다.
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	TArray<FItemShopRoundPurchase> RoundShopPurchases;

	// [레거시] 플레이어별 상점 구매/보유 기록. SaveVersion 0 세이브를 읽을 때만 채워져 있고, 마이그레이션 후 비운다.
	UPROPERTY()
	TArray<FItemShopLoadoutEntry> ItemShopPurchaseStates;
};
