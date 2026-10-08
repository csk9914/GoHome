

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Shop/ItemShopTypes.h"
#include "ItemShopSubsystem.generated.h"


class UItemShopCatalogDataAsset;
class AItemActorBase;
class APlayerController;
class APawn;
class UGoHomeSaveSubsystem;


// 보관함에서 꺼내 간 상품 하나(런타임 장부). 세이브에는 팀 보유 수량만 남고, 이 장부는 맵/라운드 안에서만 산다.
struct FSharedLockerCheckout
{
	TWeakObjectPtr<AItemActorBase> Item;
	FName ProductId = NAME_None;
};


/**
 * 상점 + 잠수정 공유 보관함의 서버 권위 로직.
 * - 저장소(팀 보유 수량, 라운드 구매 수량)는 UGoHomeSaveSubsystem의 SaveGame — 호스트 전용, 트래블·정산을 넘어 유지되고 ResetSave로 함께 초기화된다.
 * - 꺼내 간 액터 장부(Checkouts)는 여기 런타임에만 있다. 보관함 안 수량 = 보유 수량 − 꺼내 간 수량.
 * - 클라 표시는 AGoHomeGameState의 복제 미러(SharedLockerItems)로만 한다 — RefreshLockerView가 갱신.
 */
UCLASS()
class GOHOME_API UItemShopSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& CollectionBase) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Item Shop")
	bool IsShopAvailable() const
	{
		return bShopAvailable;
	}

	UFUNCTION(BlueprintPure, Category = "Item Shop")
	UItemShopCatalogDataAsset* GetCatalog() const
	{
		return Catalog;
	}

	const FItemShopProduct* FindProduct(FName ProductId) const;

	// 서버의 실제 GameState로 구매 장소를 정한다(로비 = LobbyLoadout, 탐사 = InGameImmediate). 그 외 단계면 false.
	bool TryResolveShopContext(EItemShopContext& OutContext) const;

	// 상품·수량·장소·가격 검증. 클라가 보낸 Request.Context는 쓰지 않는다.
	bool TryCalculateOrderTotal(
		const FItemShopPurchaseRequest& Request,
		EItemShopContext Context,
		int32& OutTotalPrice,
		EItemShopResult& OutResult
	) const;

	// 검증 → 자금 차감 + 공유 보관함 수량 증가를 한 번에 처리한다. 어느 단계든 실패하면 둘 다 되돌린다.
	bool TryProcessPurchase(
		APlayerController* Requester,
		const FItemShopPurchaseRequest& Request,
		FItemShopPurchaseResult& OutResult);

	// --- 공유 보관함 ---

	TArray<FSharedLockerViewEntry> BuildLockerView() const;

	// 보관함에서 상품 하나를 꺼내 요청자 손(빈 핫바 슬롯)에 지급한다.
	bool TryWithdrawFromLocker(
		APlayerController* Requester,
		FName ProductId,
		FSharedLockerResult& OutResult);

	// 요청자 핫바에 있는 보관함 상품 하나를 다시 넣는다(손에 든 것 우선).
	bool TryDepositToLocker(
		APlayerController* Requester,
		FName ProductId,
		FSharedLockerResult& OutResult);

	// 라운드 정산 직전(FinalizeRound): 꺼내 간 상품 중 살아있는 플레이어가 들고 있거나 잠수정 안에 있는 것만 회수하고,
	// 나머지는 분실(보유 수량 −1) 처리한다. 다 쓴 소모품도 소모로 처리한다.
	void ResolveCheckoutsForRoundEnd();

	// 게임 초기화(ResetSave) 시 장부만 버린다.
	void ClearCheckouts();

	// 서버: 현재 월드 GameState의 보관함 미러를 다시 싣는다.
	void RefreshLockerView();

private:
	UFUNCTION()
	void HandleCheckedOutItemEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UGoHomeSaveSubsystem* GetSaveSubsystem() const;

	int32 GetCheckedOutQuantity(FName ProductId) const;

	// 보유 수량 1 감소(사용 성공 소모 또는 분실)
	void RemoveOwnedOne(FName ProductId);

	// 장부 항목 제거 + EndPlay 구독 해제
	void ReleaseCheckout(int32 CheckoutIndex);

	// 보관함 꺼내기/넣기가 가능한 게임 단계(로비·탐사)인지
	bool IsLockerUsableInCurrentState() const;

	bool IsNearSharedLocker(const APawn* Pawn) const;

	// 다 쓴 소모품(충전량 소진 등)인지 — 회수 시 보관함으로 되돌리지 않는다.
	static bool IsSpentConsumable(const AItemActorBase* Item, EItemShopItemLifetime Lifetime);

	UPROPERTY()
	TObjectPtr<UItemShopCatalogDataAsset> Catalog;

	bool bShopAvailable = false;

	TArray<FSharedLockerCheckout> Checkouts;
};
