

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HydrothermalVentZone.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class ACharacter;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class GOHOME_API AHydrothermalVentZone : public AActor
{
	GENERATED_BODY()
	
public:	
	AHydrothermalVentZone();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnEffectAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEffectAreaEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 존 안에 있는 캐릭터들에게 매 프레임 위쪽 힘을 적용 (서버이거나 로컬로 조작 중인 캐릭터만 해당)
	void ApplyForceToOverlappingCharacters(float DeltaTime);

	// 존 안에 있는 캐릭터들에게 매 프레임 지속 데미지를 적용 (서버 전용 - 데미지 권위 판정은 서버만)
	void ApplyDamageToOverlappingCharacters(float DeltaTime);

	virtual void OnConstruction(const FTransform& Transform) override;
	
	// VentSmokeVFXAsset이 바뀌면 컴포넌트에 반영 (에디터에서 바로 미리보기 가능하게)
	void UpdateVentSmokeVFX();

	// 효과가 적용되는 범위. 이 구체 안에 있는 캐릭터에게 상승력과 지속 데미지를 적용
	UPROPERTY(VisibleAnywhere, Category = "Hydrothermal Vent")
	TObjectPtr<UBoxComponent> EffectArea;

	// 초당 위로 밀어내는 힘의 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float VentForceStrength = 600.f;

	// 존 안에 있는 동안 초당 들어가는 지속 데미지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamagePerSecond = 10.f;

	// IDamageable::ApplyDamage에 전달할 데미지 타입 이름 (HealthComponent가 이 이름으로 원인을 구분할 수 있음)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent")
	FName DamageTypeName = FName(TEXT("HydrothermalVent"));

	// 분출 연기(수증기) 나이아가라 이펙트 애셋. 에디터에서 만든 NS_HydrothermalSmoke 같은 걸 여기에 연결
	UPROPERTY(EditAnywhere, Category = "Hydrothermal Vent")
	TObjectPtr<UNiagaraSystem> VentSmokeVFXAsset;

	// 위 애셋을 재생하는 컴포넌트. FlowVFX(WaterCurrentZone)와 동일한 역할
	UPROPERTY(VisibleAnywhere, Category = "Hydrothermal Vent")
	TObjectPtr<UNiagaraComponent> VentSmokeVFX;

	// 존 안에 있는 동안 적용할 수영 감속값. 낮을수록 입력을 놓아도 상승력이 계속 유지된다
	// (수영 중 기본 감속이 AddForce로 준 상승 속도를 다음 틱에 바로 상쇄시켜서 필요함 - WaterCurrentZone과 동일한 이유)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ZoneBrakingDeceleration = 0.f;

	// 현재 존 안에 겹쳐 있는 캐릭터들
	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> OverlappingCharacters;

	// 존에 들어오기 전 캐릭터의 원래 감속값을 기억해뒀다가 나갈 때 복원
	UPROPERTY()
	TMap<TWeakObjectPtr<ACharacter>, float> OriginalBrakingDeceleration;

public:	
	// 플레이어가 존에 들어오고 나갈 때 훅 - 시야 왜곡(포스트 프로세스)/카메라 흔들림 등 연출용
	// 이 액터를 블루프린트 자식 클래스로 만들어서 블루프린트에서 이 이벤트를 받아 이으면 됨
	UFUNCTION(BlueprintImplementableEvent, Category = "Hydrothermal Vent")
	void OnCharacterEnteredZone(ACharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hydrothermal Vent")
	void OnCharacterExitedZone(ACharacter* Character);

};
