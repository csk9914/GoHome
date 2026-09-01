

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemActorBase.h"
#include "UsableItemBase.generated.h"

// 오른손 활성 슬롯 아이템 중 "사용(좌클릭)" 가능한 것들의 공통 베이스.
// 부착/활성 슬롯 로직은 AItemActorBase 재사용.
// 여기서는 "지금 사용 가능한 상태인지" 판단 지점만 추가함.
// (쿨다운/충전량/탄약 등 구체적인 제약은 서브클래스에 CanUse()를 오버라이드 해서 구현.

UCLASS()
class GOHOME_API AUsableItemBase : public AItemActorBase
{
	GENERATED_BODY()
	
public:

	// 지금 사용(좌클릭) 가능한 상태인지 확인.
	// 기본은 true -> 쿨다운/충전량 있는 아이템만 오버라이드.
	UFUNCTION(BlueprintPure, Category = "Item")
	virtual bool CanUse() const { return true; }

};
