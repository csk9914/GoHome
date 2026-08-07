

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeightProvider.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UWeightProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 보유 무게를 계산할 수 있는 오브젝트가 구현한다 (UInventoryComponent 등).
 * Player의 산소 소모 계산이 이 인터페이스로만 참조한다 (구체 클래스 결합 금지).
 */
class GOHOME_API IWeightProvider
{
	GENERATED_BODY()

public:
	virtual float GetTotalWeight() const = 0;
};
