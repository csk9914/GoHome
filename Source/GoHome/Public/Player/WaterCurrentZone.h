

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterCurrentZone.generated.h"

class USphereComponent;
class UArrowComponent;
class UPrimitiveComponent;
class ACharacter;

// 수류 구역의 동작 방식
UENUM(BlueprintType)
enum class EWaterCurrentZoneType : uint8
{
	Current,
	Whirlpool
};

UCLASS()
class GOHOME_API AWaterCurrentZone : public AActor
{
	GENERATED_BODY()
	
public:	
	AWaterCurrentZone();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void OnEffectAreaBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnEffectAreaEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	// 지금 구역 안에 있는 캐릭터들에게 힘을 계산해서 더함(서버에서만 호출)
	void ApplyForceToOverlappingCharacters(float DeltaTime);

	// 물살(Current) 모드일 때 한 캐릭터에게 가할 힘
	FVector CalculateCurrentForce(const ACharacter* Character) const;

	// 소용돌이(Whirlpool) 모드일 때 한 캐릭터에게 가할 힘
	FVector CalculateWhirlpoolForce(const ACharacter* Character) const;

	// bScaleForceByWeight가 꺼져 있으면 항상 1을 반환
	// 켜져 있으면 OtherActor에서 ICarryWeightProvider를 찾아 현재 운반 무게 기준 배율을 계산
	float GetWeightMultiplier(const AActor* OtherActor) const;

	void UpdateFlowIndicator();

	void DrawFlowDebugVisual() const;

	void DrawWhirlpoolDebugVisual() const;

	// 효과가 미치는 범위. 이 구 안에 들어온 캐릭터만 힘을 받음
	// 소용돌이는 이 반경이 곧 소용돌이 크기, 물살은 통로 폭에 맞춰 조절하면 됨
	UPROPERTY(VisibleAnywhere, Category = "Water Current")
	TObjectPtr<USphereComponent> EffectArea;

	// Current 타입일 때 FlowDirection 방향을 화살표로 표시. 게임 중엔 안 보이고 에디터에서만 보임
	UPROPERTY(VisibleAnywhere, Category = "Water Current")
	TObjectPtr<UArrowComponent> FlowIndicator;

	// 이 구역이 물살인지 소용돌이인지 파악
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current")
	EWaterCurrentZoneType ZoneType = EWaterCurrentZoneType::Current;

	// [물살 전용] 밀려나는 방향(월드 기준, 자동 정규화). 기본값은 액터의 전방(빨간 화살표, X축)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Current", meta = (EditCondition = "ZoneType == EWaterCurrentZoneType::Current"))
	FVector FlowDirection = FVector::ForwardVector;

	// [물살 전용] 초당 가해지는 힘의 크기.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Current", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "ZoneType == EWaterCurrentZoneType::Current"))
	float FlowStrength = 400.f;

	// [소용돌이 전용] 중심으로 끌어당기는 힘의 크기.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Whirlpool", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "ZoneType == EWaterCurrentZoneType::Whirlpool"))
	float PullStrength = 250.f;

	// [소용돌이 전용] 중심 축을 기준으로 회전시키는 힘의 크기 ("빙글빙글 도는" 느낌).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Whirlpool", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "ZoneType == EWaterCurrentZoneType::Whirlpool"))
	float SpinStrength = 350.f;

	// 켜면 플레이어가 들고 있는 무게(ICarryWeightProvider)에 비례해 힘이 세진다.
	// 기본은 꺼져 있음 - 나중에 체크박스만 켜면 무게 연동이 바로 동작한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Weight")
	bool bScaleForceByWeight = false;

	// 무게 1당 힘 배율이 늘어나는 비율. 예: 0.01이면 무게 100당 배율이 +1.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Weight", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bScaleForceByWeight"))
	float WeightForceScale = 0.01f;

	// 무게 배율의 상한. 아이템을 아주 많이/무겁게 들어도 힘이 비정상적으로 커지지 않게 막는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Weight", meta = (ClampMin = "1.0", UIMin = "1.0", EditCondition = "bScaleForceByWeight"))
	float MaxWeightMultiplier = 3.f;

	// 이 구역 안에 있는 동안 캐릭터의 수영 제동력을 이 값으로 낮춰 이동 입력이 없어도 물살이 계속 유지되게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current")
	float ZoneBrakingDeceleration = 0.f;

protected:
	// 지금 구역 안에 들어와 있는 캐릭터들.
	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> OverlappingCharacters;

	// 각 캐릭터의 원래 BrakingDecelerationSwimming 값을 저장해뒀다가 나갈 때 복원
	TMap<TObjectPtr<ACharacter>, float> OriginalBrakingDeceleration;

public:
	// 플레이어가 구역에 들어오고 나갈 때 - 카메라 흔들림/사운드/화면 이펙트 등 연출은
	// 이 액터의 블루프린트 자식 클래스나 레벨 블루프린트에서 이 이벤트를 받아 붙이면 된다
	// (Docs/Dev/CODING_CONVENTIONS.md "C++ / Blueprint 경계" - 연출은 BP 담당).
	UFUNCTION(BlueprintImplementableEvent, Category = "Water Current")
	void OnCharacterEnteredZone(ACharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, Category = "Water Current")
	void OnCharacterExitedZone(ACharacter* Character);

};
