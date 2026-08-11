/*

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CarryWeightProvider.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCarryWeightProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 현재 소지 무게와 최대 허용 무게를 알려줄 수 있는 오브젝트가 구현한다.
 * OxygenComponent는 구체 클래스가 아니라 이 인터페이스만 보고 초과 무게를 계산한다.
 
class GOHOME_API ICarryWeightProvider
{
	GENERATED_BODY()

public:
	virtual float GetCurrentCarryWeight() const = 0;
	virtual float GetMaxCarryWeight() const = 0;
	virtual float GetOverweightAmount() const = 0;
};
*/