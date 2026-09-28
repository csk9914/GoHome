

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Player/SocketProvider.h"
#include "AI/NoiseType.h"
#include "Player/Stunnable.h"
#include "GoHomeCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USkeletalMeshComponent;
class UOxygenComponent;
class ACoopCarryObjectBase;
class AElectricSwitchboardActor;
class UInventoryComponent;
class UPrimitiveComponent;
class USpotLightComponent;

UCLASS()
class GOHOME_API AGoHomeCharacter : public ACharacter, public ISocketProvider, public IStunnable
{
	GENERATED_BODY()

public:
	AGoHomeCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 캐릭터 이동이 뭔가 막힐 때마다 호출(물리 시뮬레이션과 무관 - 스윕 이동 블로킹 히트).
	// 인벤토리 파손 아이템 판정을 여기서 건다.
	virtual void NotifyHit(UPrimitiveComponent* MyComp, 
		                   AActor* Other, 
		                   UPrimitiveComponent* OtherComp, 
		                   bool bSelfMoved, 
		                   FVector HitLocation, 
		                   FVector HitNormal, 
		                   FVector NormalImpulse, 
		                   const FHitResult& Hit) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void MoveUpDown(const FInputActionValue& Value);
	
	// 이동 입력을 뗐을 때(Completed/Canceled) 호출 - 운반 중이면 정체된 LastCarryInputWorld를 0으로 리셋.
	void StopCarryInput();

	void StartSprint();
	void StopSprint();
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);
	void ApplySprintState(bool bNewSprinting);

	// 협동 운반 시작 시 원격 클라이언트에게 스프린트를 강제로 끄라고 알림.
    // 서버 상태만 바꿔선 클라 로컬 예측이 안 따라옴.
	UFUNCTION(Client, Reliable)
	void Client_ForceStopSprint();

	// 스턴을 로컬 클라에 강제 -> 서버 상태만 바꾸면 이동 입력 예측이 안 멈춤.
	UFUNCTION(Client, Reliable)
	void Client_ApplyStun(float Duration, FVector KnockbackImpulse);

	void EndStun();

	UFUNCTION()
	void OnRep_IsStunned();

	// MoveAction의 Started(눌리는 순간 1회) 이벤트 전용 - 포커스 중 커서 이동/예-아니오 토글.
	void HandleFocusMoveStarted(const FInputActionValue& Value);

	void MoveHighlightedKey(int32 RowDelta, int32 ColDelta);


	void Look(const FInputActionValue& Value);
	void StartTalking();
	void StopTalking();
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> FirstPersonArmsMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// 손전등(기본 장비) - 전원 동일 스펙 지급, 슬롯 불필요. Spine_03에 항상 부착.
	UPROPERTY(VisibleAnywhere, Category = "Flashlight")
	TObjectPtr<USpotLightComponent> FlashlightSpotLight;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveUpDownAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PushToTalkAction;
	
public:	
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float CurrentPitch = 0.f;
	
	// 핸들 소켓 선언.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName RightHandSocketName = "Hand_R";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName LeftHandSocketName = "Hand_L";

	// 아이템 오른손 소켓에 부착, 애니메이션 상태 전환
	// ItemActorBase에서 픽업 확정 시 호출
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void AttachItemToRightHand(UStaticMeshComponent* ItemMeshComponent);

	// F키 : 손전등 온/오프. 인벤토리와 무관 -> 전원 항상 보유.
	UFUNCTION(BlueprintCallable, Category = "Flashlight")
	void ToggleFlashlight();

	UFUNCTION(BlueprintPure, Category = "Flashlight")
	bool IsFlashlightOn() const { return bIsFlashlightOn; }

	virtual FName GetRightHandSocketName() const override { return RightHandSocketName; }
	virtual FName GetLeftHandSocketName() const override { return LeftHandSocketName; }

	// 조준 트레이스 시작점으로 쓸 카메라 월드 위치. Camera는 메시에 고정 부착이라
	// 서버도 그 캐릭터의 권위 있는 액터/본 트랜스폼으로 정확한 값을 얻을 수 있음.
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FVector GetCameraWorldLocation() const;

	virtual void SetHoldingItem(bool bHolding) override;

	// 스턴 + 넉백 적용 (IStunnable 구현). 배전반 등 환경 위해요소가 캐릭터 타입을 몰라도 호출 가능.
	virtual void ApplyStun_Implementation(float Duration, FVector KnockbackImpulse, AActor* InInstigator) override;

	UFUNCTION(BlueprintPure, Category = "Stun")
	bool IsStunned() const { return bIsStunned; }

	// 아이템을 오른손에서 떼고 애니메이션 상태를 원복
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void DetachItemFromRightHand();

	// AnimBP가 읽는 홀드 상태 (Layered Blend per Bone의 알파 보간용).
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsHoldingItem() const { return bIsHoldingItem; }

	// 협동 운반(CoopCarryObject) 관련.
	// 현재 협동 운반 중인지.
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsCoopCarrying() const { return CurrentCarryObject != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	ACoopCarryObjectBase* GetCurrentCarryObject() const { return CurrentCarryObject; }

	// ACoopCarryObjectBase가 잡기/놓기 시 호출(서버 권위).
	void SetCoopCarryObject(ACoopCarryObjectBase* NewCarryObject);

	// 서버 전용 : ACoopCarryObjectBase가 매 틱 평균 낸 이동 벡터를 세팅.
	void SetCombinedCarryInput(const FVector& NewInput);

	// 협동 운반 중 서버가 두 캐리어 입력을 평균 낼 때 사용할, 이 캐릭터의 최신 월드 스페이스 이동 의도.
	FVector GetLastCarryInputWorld() const { return LastCarryInputWorld; }

	UFUNCTION(BlueprintPure, Category = "Switchboard")
	bool IsFocusingSwitchboard() const { return FocusedSwitchboard != nullptr; }

	// 배전반 OnInteract(서버)에서 호출. 호스트/원격 공용 실제 처리부.
	void EnterSwitchboardFocus(AElectricSwitchboardActor* Switchboard);
	void ExitSwitchboardFocus();

	UFUNCTION(Client, Reliable)
	void Client_EnterSwitchboardFocus(AElectricSwitchboardActor* Switchboard);

	UFUNCTION(Client, Reliable)
	void Client_ExitSwitchboardFocus();

	void PressHighlightedKey();
	TArray<int32> EnteredPasswordDigits;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(Server, Unreliable)
	void ServerUpdatePitch(float NewPitch);
	
	UFUNCTION()
	void OnRep_IsHoldingItem();

	UFUNCTION(Server, Reliable)
	void ServerToggleFlashlight();

	UFUNCTION()
	void OnRep_IsFlashlightOn();

	void UpdateFlashlightVisual(bool bNewIsOn);
	
	UFUNCTION()
	void OnRep_ReplicatedPitch();
	
	UPROPERTY(ReplicatedUsing = OnRep_IsHoldingItem, BlueprintReadOnly, Category = "Interaction")
	bool bIsHoldingItem = false;
	
	UPROPERTY(ReplicatedUsing = OnRep_IsStunned, BlueprintReadOnly, Category = "Stun")
	bool bIsStunned = false;

	FTimerHandle StunTimerHandle;

	UPROPERTY()
	TObjectPtr<AElectricSwitchboardActor> FocusedSwitchboard;

	int32 HighlightedWireIndex = -1;

	UPROPERTY(ReplicatedUsing = OnRep_IsFlashlightOn)
	bool bIsFlashlightOn = false;
	
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedPitch)
	float ReplicatedPitch = 0.f;
	
	// CoopCarryObject 관련
	// ---------------------------------------------------
	UPROPERTY(ReplicatedUsing = OnRep_CurrentCarryObject, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<ACoopCarryObjectBase> CurrentCarryObject;

	// 협동 운반 중 서버가 계산한 "이미 CarrySpeedScale까지 반영된" 합산 이동 벡터.
	// 각 캐릭터가 이 값을 자기 자신에게 로컬로 AddMovementInput 하는 방식 -> ServerMove 충돌 회피.
	UPROPERTY(Replicated)
	FVector CombinedCarryInput = FVector::ZeroVector;

	UFUNCTION()
	void OnRep_CurrentCarryObject();

	UFUNCTION(Server, Unreliable)
	void Server_UpdateCarryInput(FVector WorldIntent);

	// 피격/사망 시 강제로 운반 해제하기 위한 구독 핸들러.
	UFUNCTION()
	void HandleHPChanged(float CurrentHP, float MaxHP);

	void HandleForcedCarryRelease();
    // ---------------------------------------------------

    void UpdateWireHighlight(int32 OldIndex, int32 NewIndex);

	// 인벤토리 파손 아이템 충돌 감지(캐릭터 이동 충돌 기반).
	// ---------------------------------------------------
	// 서버 전용. 상대 속도(나 - 상대) 기준으로 소지 아이템(활성 + 비활성 슬롯 전체)에 파손 판정을 적용.
	void HandleInventoryBreakOnHit(AActor* OtherActor);

	// 캐릭터 단위 쿨다운(초). 실제로 하나라도 깨졌을 때만 시작
	// 좁은 통로에서 서로 밀며 반복 충돌할 때 매 틱 판정되는 것 방지.
	// 약하게 스친 것만으로는 소모되지 않음.
	UPROPERTY(EditDefaultsOnly, Category = "Item", meta = (ClampMin = "0.0"))
	float ItemBreakCooldownSeconds = 0.5;

	// 다음 판정 가능 시간. -1 = 아직 한 번도 깨진 적 없음.
	float NextItemBreakEligibleTime = -1.f;
	// ---------------------------------------------------

	// 수영, 소음 등급
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	ENoiseType SwimNoiseType = ENoiseType::Small;
	
	// 수영, 소음 반경
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	float SwimNoiseRadius = 800.f;
	
	// 소음 몇 초 간격으로 쏠 지
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	float SwimNoiseInterval = 1.5f;

	// 마지막 소음 발생 이후 누적 시간
	float TimeSinceLastSwimNoise = 0.f;

	void StartHintPlayback();
	void AdvanceHintPlayback();
	void SetWireColor(UMeshComponent* Wire, const FLinearColor& Color);
	void SetAllWiresOff();

	bool bPlayingHint = false;
	int32 HintPlaybackStep = -1;
	bool bHintShowingGap = false;
	FTimerHandle HintPlaybackTimerHandle;


	// 스프린트 관련
private:

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SprintSpeedMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SprintOxygenDrainMultiplier = 1.5f;

	bool bIsSprinting = false;
	float DefaultMaxSwimSpeed = 0.f;

	UPROPERTY()
	TObjectPtr<UOxygenComponent> CachedOxygenComponent;

	UPROPERTY()
	TObjectPtr<UInventoryComponent> CachedInventoryComponent;

	FVector LastCarryInputWorld = FVector::ZeroVector;
	float LastKnownHP = -1.f; // -1 = 아직 초기화 안됨(최초 값으로는 감소 판정 안 하기 위함).
};

