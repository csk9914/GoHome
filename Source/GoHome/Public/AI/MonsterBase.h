

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AI/MonsterNoiseListener.h"
#include "MonsterBase.generated.h"

/**
 * 내부 상태 머신/조향 로직은 BP_Monster(이 클래스의 자식 BP)가 구현한다 (이 클래스 범위 밖).
 * 이 클래스는 경계 인터페이스만 고정한다: IDamageable 호출(공격), IMonsterNoiseListener 구현(소음 감지),
 * UDockingDoorComponent 공개 상태 구독(도킹 문 위협 판정).
 */
UCLASS()
class GOHOME_API AMonsterBase : public ACharacter, public IMonsterNoiseListener
{
	GENERATED_BODY()

public:
	AMonsterBase();

	virtual void OnNoiseHeard_Implementation(FVector Location, float Radius, ENoiseType Type, AActor* Source) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Monster")
	void Attack(AActor* Target);
};
