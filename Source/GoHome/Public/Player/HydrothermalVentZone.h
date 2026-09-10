

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HydrothermalVentZone.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class ACharacter;
class UNiagaraComponent;
class UNiagaraSystem;

// 열수분출공의 분출 사이클 상태
UENUM(BlueprintType)
enum class EVentState : uint8
{
	Idle,		// 대기 - 분출 사이 쉬는 구간. 힘도 데미지도 없다
	Warning,	// 예고 - 곧 분출한다는 신호만 주는 구간. 아직 데미지 없음
	Erupting	// 분출 - 실제 분출 구간. 상승력 + 지속 데미지가 들어간다
};

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

	// 분출 사이클 타이머를 진행시키고 때가 되면 다음 상태로 넘긴다 (서버 전용)
	void UpdateVentCycle(float DeltaTime);

	// 상태를 바꾸는 유일한 창구. 상태별 지속 시간까지 함께 세팅한다 (서버 전용)
	void SetVentState(EVentState NewState);

	// 상태가 바뀌었을 때 VFX/감속/블루프린트 이벤트를 반영 (서버와 클라이언트 공통)
	void OnVentStateChanged();

	// 캐릭터의 수영 감속값을 존 값으로 덮어쓰거나(bEnable=true) 원래 값으로 되돌린다(bEnable=false)
	void ApplyBrakingOverride(ACharacter* Character, bool bEnable);

	// VentState가 복제되어 도착했을 때 클라이언트에서 호출된다
	UFUNCTION()
	void OnRep_VentState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 효과가 적용되는 범위. 이 구체 안에 있는 캐릭터에게 상승력과 지속 데미지를 적용
	UPROPERTY(VisibleAnywhere, Category = "Hydrothermal Vent")
	TObjectPtr<UBoxComponent> EffectArea;

	// 초당 위로 밀어내는 힘의 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float VentForceStrength = 600.f;

	// 존 안에 있는 동안 초당 들어가는 지속 데미지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamagePerSecond = 5.f;

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

	// 체크하면 트리거 방식으로 동작한다. 존 안에 아무도 없는 동안에는 대기 상태에서 멈춰 있다가,
	// 누가 들어오면 그때부터 예고 -> 분출 사이클이 돌아간다.
	// 체크를 풀면 사람이 있든 없든 계속 주기적으로 분출한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent|Cycle")
	bool bEruptOnlyWhenCharacterInside = false;

	// 분출과 분출 사이에 쉬는 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent|Cycle", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float IdleDuration = 3.f;

	// 0보다 크면 쉬는 시간에 0 ~ 이 값 사이의 랜덤을 더한다 (플레이어가 타이밍을 외우지 못하게)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent|Cycle", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float IdleDurationVariance = 0.f;

	// 분출 직전 예고 구간. 이 동안에는 상승력도 데미지도 들어가지 않으므로 플레이어가 피할 수 있다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent|Cycle", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WarningDuration = 0.8f;

	// 실제 분출이 지속되는 시간. 이 동안에만 상승력과 지속 데미지가 들어간다
	// (한 번의 분출로 들어가는 총 데미지 = DamagePerSecond * EruptDuration)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hydrothermal Vent|Cycle", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EruptDuration = 8.f;

	// 현재 분출 사이클 상태. 서버가 굴리고 클라이언트로 복제된다 (사람마다 타이밍이 어긋나지 않도록)
	UPROPERTY(ReplicatedUsing = OnRep_VentState, BlueprintReadOnly, Category = "Hydrothermal Vent|Cycle")
	EVentState VentState = EVentState::Idle;

	// 현재 상태가 끝나기까지 남은 시간. 서버에서만 의미가 있다
	float StateTimeRemaining = 0.f;

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

	// 예고 구간이 시작될 때 - 경고음, 바닥 기포 등 "곧 터진다"는 신호를 블루프린트에서 붙이는 지점
	UFUNCTION(BlueprintImplementableEvent, Category = "Hydrothermal Vent")
	void OnVentWarningStarted();

	// 실제 분출이 시작될 때 - 분출음, 카메라 흔들림 등
	UFUNCTION(BlueprintImplementableEvent, Category = "Hydrothermal Vent")
	void OnVentEruptStarted();

	// 분출이 끝나고 대기 상태로 돌아갈 때
	UFUNCTION(BlueprintImplementableEvent, Category = "Hydrothermal Vent")
	void OnVentEruptEnded();

	// 지금 분출 중인지 블루프린트에서 확인할 때 사용
	UFUNCTION(BlueprintPure, Category = "Hydrothermal Vent")
	bool IsErupting() const { return VentState == EVentState::Erupting; }

};
