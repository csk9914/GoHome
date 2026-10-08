#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Shop/ItemShopTypes.h"
#include "SharedLockerBackend.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FSharedLockerResultEvent, const FSharedLockerResult& /*Result*/);

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class USharedLockerBackend : public UInterface
{
	GENERATED_BODY()
};

/**
 * 잠수정 공유 보관함 UI(UI/Locker)가 아는 요청 계약. 구현체(AGoHomePlayerController)가 서버 RPC로 넘기고,
 * 위젯 생성·입력 모드도 구현체가 소유한다. 수량은 이 계약이 아니라 AGoHomeGameState 복제 미러로 읽는다
 * — UI는 요청만 하고 게임 상태를 직접 바꾸지 않는다.
 */
class GOHOME_API ISharedLockerBackend
{
	GENERATED_BODY()

public:
	virtual void RequestLockerWithdraw(FName ProductId) = 0;
	virtual void RequestLockerDeposit(FName ProductId) = 0;

	// 위젯 제거와 게임 입력·커서 복원
	virtual void RequestCloseSharedLocker() = 0;

	// 서버가 꺼내기/넣기를 처리한 결과(실패 사유 표시용). 성공 수량 변화는 복제 미러로도 따로 온다.
	virtual FSharedLockerResultEvent& OnSharedLockerResult() = 0;
};
