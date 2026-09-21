

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "ElectricSwitchboardActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ACharacter;
class UMeshComponent;
class UCameraComponent;
class UWidgetComponent;
class USwitchboardPasswordWidget;
class USwitchboardScreenWidget;
class ARewardEntranceActor;
class UAudioComponent;
class UPointLightComponent;
class UNiagaraSystem;
class USoundBase;
class USoundAttenuation;

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

	// 퍼즐 성공 상태이고 아직 차단기를 안 내렸을 때만 true (차단기 액터가 클라에서도 읽으므로 bBreakerFlipped는 복제).
	bool CanFlipBreaker() const { return SwitchboardState == ESwitchboardState::Resolved && !bBreakerFlipped; }
	bool IsBreakerFlipped() const { return bBreakerFlipped; }

	// AElectricBreakerActor::OnInteract에서 호출.
	bool TryResolveViaBreaker();

	UFUNCTION(Server, Reliable)
	void ServerSubmitPassword(const TArray<int32>& EnteredDigits, AActor* InInstigator);

	int32 GetPasswordLength() const { return PasswordLength; }
	int32 GetKeypadElementCount() const { return WireTargets.Num() + 2; } // 숫자 10 + Back + Enter
	UMeshComponent* GetKeypadElementMesh(int32 Index) const;
	void UpdatePasswordDisplay(const TArray<int32>& EnteredDigits);



	virtual bool CanInteract(APawn* InstigatorPawn) const override;
	virtual void OnInteract(APawn* InstigatorPawn) override; // 포커스 모드 진입 트리거
	virtual FText GetInteractionPromptText_Implementation() const override;

	int32 GetWireTargetCount() const { return WireTargets.Num(); }

	UMeshComponent* GetWireTarget(int32 Index) const { return WireTargets.IsValidIndex(Index) ? WireTargets[Index] : nullptr; }

	const TArray<FLinearColor>& GetHintColorPalette() const { return HintColorPalette; }
	float GetHintRoundDuration() const { return HintRoundDuration; }
	float GetHintGapDuration() const { return HintGapDuration; }
	int32 GetCorrectWireIndexForStep(int32 Step) const { return PasswordDigits.IsValidIndex(Step) ? PasswordDigits[Step] : -1; }

	// 힌트 설정값
	UPROPERTY(EditAnywhere, Category = "Switchboard|Puzzle|Hint")
	TArray<FLinearColor> HintColorPalette = { FLinearColor::Red, FLinearColor::Blue, FLinearColor::Green, FLinearColor::Yellow, FLinearColor::White };

	UPROPERTY(EditAnywhere, Category ="Switchboard|Puzzle|Hint")
	float HintRoundDuration = 1.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Puzzle|Hint")
	float HintGapDuration = 0.5f;




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
	void CollectKeypadComponents();

	UFUNCTION()
	void OnRep_DangerGauge();

	UFUNCTION()
	void OnRep_SwitchboardState();

	UPROPERTY(ReplicatedUsing = OnRep_FocusingPawn, BlueprintReadOnly, Category = "Switchboard")
	TObjectPtr<APawn> FocusingPawn;

	UFUNCTION()
	void OnRep_FocusingPawn();

	UPROPERTY(VisibleAnywhere, Category = "Switchboard")
	TObjectPtr<UCameraComponent> FocusCamera;

	// 퍼즐 종결(성공/실패) 시 호출 - 포커스 중이던 플레이어를 로컬 UX까지 정리해서 풀어줌.
	void ReleaseFocus();


	// --- 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, Category = "Switchboard")
	TObjectPtr<UBoxComponent> EffectArea;

	UPROPERTY(VisibleAnywhere, Category = "Switchboard")
	TObjectPtr<UStaticMeshComponent> SwitchboardMesh;

	// --- 게이지 ---
	UPROPERTY(EditAnywhere, Category = "Switchboard|Gauge")
	float GaugeRisePerSecond = 1.f;

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
	TArray<TObjectPtr<UMeshComponent>> WireTargets;

	// 정답 순서. 힌트가 이미 정답을 보여주는 방식이라 클라에 노출해도 무방 (BeginPlay에서 서버가 셔플).
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Switchboard|Puzzle")
	TArray<int32> PasswordDigits;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Puzzle")
	int32 PasswordLength = 5;

	UPROPERTY(VisibleAnywhere, Category = "Switchboard")
	TObjectPtr<UMeshComponent> BackButtonMesh;

	UPROPERTY(VisibleAnywhere, Category = "Switchboard")
	TObjectPtr<UMeshComponent> EnterButtonMesh;

	UPROPERTY(VisibleAnywhere, Category = "Switchboard")
	TObjectPtr<UWidgetComponent> MainScreenWidget;
	
	UPROPERTY(VisibleAnywhere, Category = "Switchboard")
	TObjectPtr<UWidgetComponent> PasswordDisplayWidget;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Switchboard|Puzzle")
	EWireHintMode HintMode = EWireHintMode::FlashSequence;

	UPROPERTY(ReplicatedUsing = OnRep_SwitchboardState, BlueprintReadOnly, Category = "Switchboard")
	ESwitchboardState SwitchboardState = ESwitchboardState::PuzzleActive;

	UPROPERTY(Replicated)
	bool bBreakerFlipped = false;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Puzzle")
	float LockedResetDuration = 30.f;

	// 퍼즐 결과에 따라 열어줄 보상 입구. 레벨에서 연결 (다른 방/여러 개 가능).
	UPROPERTY(EditInstanceOnly, Category = "Switchboard|Reward")
	TArray<TObjectPtr<ARewardEntranceActor>> LinkedEntrances;

	// --- 위험 피드백 (코스메틱) ---
    // 게이지 비율(0~1)로 스파크/험/조명을 구동. 서버·클라 모두 로컬에서 돌고(전용 서버 제외) 리플리케이트하지 않음.
    // 효과 세기가 실제 위험과 같이 움직이도록 Resolved(게이지·페널티 정지 상태)면 0으로 취급.
	void UpdateDangerFeedback(float DeltaTime);
	void SpawnSpark(float Ratio);

	// 루프 사운드(Sound)와 Attenuation은 BP 컴포넌트 Details에서 지정.
	UPROPERTY(VisibleAnywhere, Category = "Switchboard|Feedback")
	TObjectPtr<UAudioComponent> HumAudio;

	// 위치/Attenuation Radius/기준 Intensity는 BP 컴포넌트 Details에서 지정 (기준 Intensity가 배율 1.0).
	UPROPERTY(VisibleAnywhere, Category = "Switchboard|Feedback")
	TObjectPtr<UPointLightComponent> WarningLight;

	// 게이지 -> 효과 비율 보간 속도 (클라의 계단식 리플리케이션 완화 + 차단기 조작 시 부드럽게 꺼짐).
	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback")
	float FeedbackRatioInterpSpeed = 3.f;

	// --- 스파크 (에셋 슬롯이 비어 있으면 해당 효과만 건너뜀) ---
	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Spark")
	TObjectPtr<UNiagaraSystem> SparkEffect;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Spark")
	TObjectPtr<USoundBase> SparkSound;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Spark")
	TObjectPtr<USoundAttenuation> SparkSoundAttenuation;

	// 스파크 간격: 게이지가 낮을 때(Max) -> 높을 때(Min). 실제 간격은 이 값의 70~130% 랜덤.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Spark")
	float SparkIntervalMax = 2.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Spark")
	float SparkIntervalMin = 0.15f;

	// 스파크 발생 범위 = 배전반 메시 바운딩 박스 * 이 값.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Spark")
	float SparkAreaScale = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Spark")
	float SparkVolumeMin = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Spark")
	float SparkVolumeMax = 1.f;

	// --- 험 루프 ---
	// 이 비율 이상일 때만 재생. 이 지점에서 0, 비율 1.0에서 1이 되도록 볼륨/피치를 Lerp.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Hum", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float HumStartRatio = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Hum")
	float HumVolumeMin = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Hum")
	float HumVolumeMax = 1.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Hum")
	float HumPitchMin = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Hum")
	float HumPitchMax = 1.4f;

	// --- 경고 조명: 호박색(낮음) -> 붉은색(높음), 게이지가 높을수록 세기가 커지고 깜빡임이 깊어짐 ---
	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Light")
	FLinearColor WarningLightColorLow = FLinearColor(1.f, 0.55f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Light")
	FLinearColor WarningLightColorHigh = FLinearColor(1.f, 0.05f, 0.02f);

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Light")
	float WarningLightScaleMin = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Light")
	float WarningLightScaleMax = 3.f;

	// 깜빡일 때 세기가 최대 (1 - Depth * 비율) 배까지 떨어짐. 0 = 깜빡임 없음.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Feedback|Light", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightFlickerDepth = 0.8f;

	// --- 알람 소음 (서버 전용): 게이지가 오르는 동안 몬스터를 끌어들임 ---
    // 서버 권위 Tick에서 호출. 게이지 비율이 시작 임계값 이상이면 주기적으로 GenerateNoise 발생.
    // 누적 알람 시간이 MaxAlarmSeconds에 닿으면 이후 무음(회로 소손) - 실패 후 무한 사이렌으로 몬스터를 붙잡아 두는 어뷰징 방지.
	void TickAlarmNoise(float DeltaTime);

	// 이 비율 이상일 때만 소음 발생.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AlarmStartRatio = 0.3f;

	// 이 비율 이상이면 Large 등급.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AlarmLargeRatio = 0.6f;

	// 이 비율 이상이면 Alarm 등급.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AlarmPeakRatio = 0.9f;

	// 등급별 소음 반경. ENoiseType 문서 범위: Medium 800~1200, Large 1500~2000, Alarm 2500~3000.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm")
	float AlarmRadiusMedium = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm")
	float AlarmRadiusLarge = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm")
	float AlarmRadiusPeak = 3500.f;

	// 발생 간격: 게이지가 낮을 때(Max) -> 높을 때(Min).
	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm")
	float AlarmIntervalMax = 4.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm")
	float AlarmIntervalMin = 1.2f;

	// 누적 알람 상한(초). 알람이 실제로 활성인 시간만큼 소진되고, 0이 되면 이후 소음 없음. 0 이하 = 소음 끔.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm")
	float MaxAlarmSeconds = 60.f;

	// --- 알람 사운드: 소음이 발생하는 순간 모든 머신에서 재생 ---
    // 소음은 서버에서만 발생하고 알람 예산도 서버 전용이라, 서버가 발생 시점에 Multicast로 알린다
    // (클라가 자체 타이머로 재생하면 예산 소진 시점을 몰라 어긋남).
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAlarmSound(float VolumeMultiplier, float PitchMultiplier);

	// 짧은 원샷 사운드 (Sound Wave의 Looping은 꺼둘 것).
	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm|Sound")
	TObjectPtr<USoundBase> AlarmSound;

	// 가청 거리를 소음 반경(Peak 기준)에 맞춘 감쇠 에셋.
	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm|Sound")
	TObjectPtr<USoundAttenuation> AlarmSoundAttenuation;

	// 등급별 볼륨/피치 배율 (사운드 에셋 하나를 공유).
	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm|Sound")
	float AlarmVolumeMedium = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm|Sound")
	float AlarmPitchMedium = 1.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm|Sound")
	float AlarmVolumeLarge = 0.85f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm|Sound")
	float AlarmPitchLarge = 1.1f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm|Sound")
	float AlarmVolumePeak = 1.f;

	UPROPERTY(EditAnywhere, Category = "Switchboard|Alarm|Sound")
	float AlarmPitchPeak = 1.25f;

	void ResetPuzzle();

private:
	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> OverlappingCharacters;

	// 캐릭터 별 "다음 판정 가능 시각" -> 스턴 유예 보장용.
	TMap < TWeakObjectPtr<ACharacter>, float> NextPenaltyEligibleTime;

	float PenaltyTickAccumulator = 0.f;

	FTimerHandle LockedResetTimerHandle;

	// 피드백 상태 (로컬 코스메틱 전용)
	float SmoothedFeedbackRatio = 0.f;
	float SparkTimer = 0.f;
	float LightFlickerTimer = 0.f;
	float LightFlickerFactor = 1.f;
	float BaseLightIntensity = 1.f;

	// 알람 소음 상태 (서버 전용)
	float AlarmTimer = 0.f;
	float AlarmSecondsUsed = 0.f;

};
