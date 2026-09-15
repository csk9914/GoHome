

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "ElectricSwitchboardActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ACharacter;

UENUM(BlueprintType)
enum class ESwitchboardState : uint8
{
	PuzzleActive, // 퍼즐 진행 가능(성공/실패 확정 전)
	Resolved, // 성공 - 위험 완전 해제, 최고 보상 입구 오픈.
	Locked // 실패 - 차단기 잠김, 고위험 강행 루트만 남음.
};

UENUM(BlueprintType)
enum class EWireHintMode : uint8
{
	FlashSequence, // 시작 시 전선이 빛나는 순서로 정답을 암시.
	ColorIndexClue // 색상이 랜덤 순서로 나타나고 "N번째 색"을 지시
};


// 합선 배전반: 위험도 게이지 + 전선 제거 퍼즐 + 차단기 연동 위협형 보상 오브젝트.
// 권위: 게이지·퍼즐 진행·SwitchboardState 전부 서버 권위, 클라는 표시만.
// 개인별 페널티는 IStunnable을 통해 호출 (캐릭터 타입 결합 금지 - HydrothermalVentZone과 같은 원칙).
UCLASS()
class GOHOME_API AElectricSwitchboardActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:

	AElectricSwitchboardActor();

	bool IsBreakerUnlocked() const { return SwitchboardState == ESwitchboardState::Resolved; }

	// AElectricBreakerActor::OnInteract에서 호출.
	bool TryResolveViaBreaker();

	// 포커스 모드에서 전선 하나를 제거 시도할 때 호출.
	UFUNCTION(Server, Reliable)
	void ServerRemoveWire(int32 WireIndex, AActor* InInstigator);

	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override; // 포커스 모드 진입 트리거
	virtual FText GetInteractionPromptText_Implementation() const override;

	int32 GetWireTargetCount() const { return WireTargets.Num(); }
	
	UPrimitiveComponent* GetWireTarget(int32 Index) const { return WireTargets.IsValidIndex(Index) ? WireTargets[Index] : nullptr; }

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnEffectAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
		                          AActor* OtherActor, 
		                          UPrimitiveComponent* OtherComp, 
		                          int32 OtherBodyIndex, 
		                          bool bFromSweep, 
		                          const FHitResult& SweepResult);

	UFUNCTION()
	void OnEffectAreaEndOverlap(UPrimitiveComponent* OverlappedComponent,
		                        AActor* OtherActor,
		                        UPrimitiveComponent* OtherComp,
		                        int32 OtherBodyIndex);

	// 구역 안 캐릭터별 개별 확률 판정 (게이지 비례) -> PenaltyTickInterval 주기로 호출.
	void TickPenalizeOverlappingCharacters(float DeltaTime);

	void GenerateWirePuzzle();
	void HandlePuzzleFailed();
	void HandlePuzzleSucceeded();
	void CollectWireTargets();

	UFUNCTION()
	void OnRep_DangerGauge();

	UFUNCTION()
	void OnRep_SwitchboardState();

	UPROPERTY(ReplicatedUsing = OnRep_FocusingPawn, BlueprintReadOnly, Category = "Switchboard")
	TObjectPtr<APawn> FocusingPawn;

	UFUNCTION()
	void OnRep_FocusingPawn();

	// 퍼즐 종결(성공/실패) 시 호출 - 포커스 중이던 플레이어를 로컬 UX까지 정리해서 풀어줌.
	void ReleaseFocus();


	UFUNCTION()
	void OnRep_NextCorrectStep();

	// --- 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, Category = "Switchboard")
	TObjectPtr<UBoxComponent> EffectArea;

	UPROPERTY(VisibleAnywhere, Category = "Switchboard")
	TObjectPtr<UStaticMeshComponent> SwitchboardMesh;

	// --- 게이지 ---
	UPROPERTY(EditAnywhere, Category = "Switchboard|Gauge")
	float GaugeRisePerSecond = 5.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Gauge")
	float MaxGauge = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_DangerGauge, BlueprintReadOnly, Category = "Switchboard|Gauge")
	float DangerGauge = 0.f;

	// --- 페널티 (개인별, 게이지 비례) ---
	UPROPERTY(EditAnywhere, Category = "Switchboard|Penalty")
	float PenaltyTickInterval = 2.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Penalty")
	float StunDuration = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Penalty")
	float KnockbackStrength = 800.f;

	// 스턴 종료 후 다음 판정까지 보장하는 최소 유예 (스턴락 방지 - 사용자 확정 사항).
	UPROPERTY(EditAnywhere, Category = "Switchboard|Penalty")
	float PostStunGracePeriod = 1.f;

	// --- 퍼즐 ---
// LD가 "Wire0", "Wire1"... 이름으로 자식 컴포넌트를 만들면 BeginPlay에서 자동 수집됨.
	UPROPERTY(VisibleAnywhere, Category = "Switchboard|Puzzle")
	TArray<TObjectPtr<UPrimitiveComponent>> WireTargets;

	

	// 정답 순서. 힌트가 이미 정답을 보여주는 방식이라 클라에 노출해도 무방 (BeginPlay에서 서버가 셔플).
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Switchboard|Puzzle")
	TArray<int32> WireRemovalOrder;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Switchboard|Puzzle")
	EWireHintMode HintMode = EWireHintMode::FlashSequence;

	// WireRemovalOrder 중 다음에 맞혀야 할 인덱스 (진행도 표시용).
	UPROPERTY(ReplicatedUsing = OnRep_NextCorrectStep, BlueprintReadOnly, Category = "Switchboard|Puzzle")
	int32 NextCorrectStep = 0;

	UPROPERTY(ReplicatedUsing = OnRep_SwitchboardState, BlueprintReadOnly, Category = "Switchboard")
	ESwitchboardState SwitchboardState = ESwitchboardState::PuzzleActive;

private:
	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> OverlappingCharacters;

	// 캐릭터 별 "다음 판정 가능 시각" -> 스턴 유예 보장용.
	TMap < TWeakObjectPtr<ACharacter>, float> NextPenaltyEligibleTime;

	float PenaltyTickAccumulator = 0.f;

};
