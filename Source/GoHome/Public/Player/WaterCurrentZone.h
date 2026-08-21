

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterCurrentZone.generated.h"

class USphereComponent;
class UArrowComponent;
class UPrimitiveComponent;
class ACharacter;
class UNiagaraComponent;
class UNiagaraSystem;

// 구역 타입 - 물살(Current)인지 소용돌이(Whirlpool)인지 구분
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

	// 존 안에 있는 캐릭터들에게 매 프레임 힘을 계산해서 적용 (서버이거나 로컬로 조작 중인 캐릭터만 해당)
	void ApplyForceToOverlappingCharacters(float DeltaTime);

	// 물살(Current) 타입일 때 캐릭터에게 줄 힘 계산
	FVector CalculateCurrentForce(const ACharacter* Character) const;

	// 소용돌이(Whirlpool) 타입일 때 캐릭터에게 줄 힘 계산
	FVector CalculateWhirlpoolForce(const ACharacter* Character) const;

	// bScaleForceByWeight가 꺼져 있으면 항상 1을 반환
	// 켜져 있으면 OtherActor에서 ICarryWeightProvider를 찾아 무게에 따른 배율을 계산
	float GetWeightMultiplier(const AActor* OtherActor) const;

	void UpdateFlowIndicator();

	void UpdateFlowVFX();

	void DrawFlowDebugVisual() const;

	void DrawWhirlpoolDebugVisual() const;

	// 효과가 적용되는 범위. 이 구체 안에 있는 캐릭터에게 힘을 적용
	// 소용돌이일 때는 이 반지름이 곧 소용돌이 크기, 디버그 시각화 범위 계산에도 사용됨
	UPROPERTY(VisibleAnywhere, Category = "Water Current")
	TObjectPtr<USphereComponent> EffectArea;

	// Current 타입일 때 FlowDirection 방향을 화살표로 표시. 에디터 뷰포트에서만 보임
	UPROPERTY(VisibleAnywhere, Category = "Water Current")
	TObjectPtr<UArrowComponent> FlowIndicator;

	// Current 타입일 때 재생할 나이아가라 이펙트. 에디터에서 만든 NS_WaterFlow 같은 걸 여기에 연결
	UPROPERTY(EditAnywhere, Category = "Water Current")
	TObjectPtr<UNiagaraSystem> CurrentFlowVFXAsset;

	UPROPERTY(EditAnywhere, Category = "Water Current")
	TObjectPtr<UNiagaraSystem> WhirlpoolVFXAsset;

	// 존 종류에 맞는 이펙트를 재생하는 컴포넌트. FlowDirection에 따라 자동으로 회전됨
	UPROPERTY(VisibleAnywhere, Category = "Water Current")
	TObjectPtr<UNiagaraComponent> FlowVFX;

	// 이 존이 물살인지 소용돌이인지 결정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current")
	EWaterCurrentZoneType ZoneType = EWaterCurrentZoneType::Current;

	// [물살 전용] 밀려나는 방향(로컬 기준, 자동 정규화). 기본값은 정면 방향(파란 화살표, X축)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Current", meta = (EditCondition = "ZoneType == EWaterCurrentZoneType::Current"))
	FVector FlowDirection = FVector::ForwardVector;

	// [물살 전용] 초당 밀려나는 힘의 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Current", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "ZoneType == EWaterCurrentZoneType::Current"))
	float FlowStrength = 400.f;

	// [소용돌이 전용] 중심으로 끌려가는 힘의 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Whirlpool", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "ZoneType == EWaterCurrentZoneType::Whirlpool"))
	float PullStrength = 250.f;

	// [소용돌이 전용] 중심 기준 접선 방향으로 회전시키는 힘의 크기 (돌아가는 속도를 결정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Whirlpool", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "ZoneType == EWaterCurrentZoneType::Whirlpool"))
	float SpinStrength = 350.f;

	// 무거운 아이템을 들고 있는 캐릭터(ICarryWeightProvider)일수록 힘을 세게 적용
	// 기본은 꺼진 상태 - 나중에 체크박스만 켜면 바로 적용된다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Weight")
	bool bScaleForceByWeight = false;

	// 무게 1당 배율이 늘어나는 정도. 예: 0.01이면 무게 100당 배율 +1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Weight", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bScaleForceByWeight"))
	float WeightForceScale = 0.01f;

	// 배율의 상한선. 무거운 아이템을 들어도 이 배율 이상으로는 커지지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current|Weight", meta = (ClampMin = "1.0", UIMin = "1.0", EditCondition = "bScaleForceByWeight"))
	float MaxWeightMultiplier = 3.f;

	// 이 존 안에 있는 동안 캐릭터에게 적용할 수영 감속값. 낮을수록 입력을 놓아도 힘이 계속 유지된다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water Current")
	float ZoneBrakingDeceleration = 0.f;

protected:
	// 현재 존 안에 겹쳐 있는 캐릭터들
	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> OverlappingCharacters;

	static TMap<TWeakObjectPtr<ACharacter>, float> GlobalOriginalBraking;
	static TMap<TWeakObjectPtr<ACharacter>, int32> GlobalOverlapCount;

public:
	// 플레이어가 존에 들어오고 나갈 때 훅 - 카메라 흔들림/사운드/화면 이펙트 등 연출용
	// 이 액터를 블루프린트 자식 클래스로 만들어서 블루프린트에서 이 이벤트를 받아 이으면 된다
	// (Docs/Dev/CODING_CONVENTIONS.md "C++ / Blueprint 경계" - 연출은 BP 담당)
	UFUNCTION(BlueprintImplementableEvent, Category = "Water Current")
	void OnCharacterEnteredZone(ACharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, Category = "Water Current")
	void OnCharacterExitedZone(ACharacter* Character);

};
