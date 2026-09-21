

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RewardEntranceActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UItemDataAsset;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
enum class ERewardGrade : uint8
{
	High,  // 퍼즐 성공 + 차단기 조작 - 최고 보상.
	Risky  // 퍼즐 실패 - 일반 오브젝트보단 낫지만 High보단 낮은 보상 (고위험 상태에서 획득).
};

UENUM(BlueprintType)
enum class EEntranceState : uint8
{
	Sealed,
	OpenedHigh,
	OpenedRisky
};

// 등급별 보상 구성. Pool에서 Count개를 (중복 허용) 무작위로 뽑아 스폰한다.
USTRUCT(BlueprintType)
struct FRewardTierConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TArray<TObjectPtr<UItemDataAsset>> Pool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0"))
	int32 Count = 1;
};

// 배전반 퍼즐 결과로 열리는 보상 입구.
// 권위: EntranceState 하나가 "열림 여부"의 유일한 진실 - 서버만 ServerOpen으로 바꾸고, 클라는 OnRep으로 연출만 재생.
// 보상 아이템은 열리는 순간 서버가 스폰 (등급은 결과가 나와야 확정되므로)

UCLASS()
class GOHOME_API ARewardEntranceActor : public AActor
{
	GENERATED_BODY()
	
public:	

	ARewardEntranceActor();


	bool IsOpened() const { return EntranceState != EEntranceState::Sealed; }

	// 서버 전용. Sealed일 때만 1회 동작(이미 열렸으면 무시 -> 차단기 중복 조작에도 안전).
	void ServerOpen(ERewardGrade Grade);

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 루트(앵커). GapGlow/Reward*가 이 아래에 붙으므로 움직이지 않는다 - 메시는 비워 둘 것(문은 DoorMesh).
	UPROPERTY(VisibleAnywhere, Category = "RewardEntrance")
	TObjectPtr<UStaticMeshComponent> BlockerMesh;

	// 실제 문. 열릴 때 이 컴포넌트만 움직인다 (루트를 움직이면 빛과 보상 스폰 위치까지 딸려감).
	// 틈(slit)이 뚫린 메시를 넣고, 콜리전은 틈까지 막히게 단순 박스로 - 빛만 새어 나오게.
	UPROPERTY(VisibleAnywhere, Category = "RewardEntrance")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	// 틈으로 새어 나오는 빛. 위치는 BP에서 문 안쪽에 배치. 컴포넌트의 Intensity가 배율 1.0의 기준.
	UPROPERTY(VisibleAnywhere, Category = "RewardEntrance")
	TObjectPtr<UPointLightComponent> GapGlow;

	// --- 보상 (LD가 입구마다 조정) ---
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Reward")
	FRewardTierConfig HighReward;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Reward")
	FRewardTierConfig RiskyReward;

	// --- 글로우: 봉인 = 중립, High = 금색 강하게, Risky = 붉은 경고색 ---
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	FLinearColor SealedGlowColor = FLinearColor(0.6f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	float SealedGlowScale = 1.f;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	FLinearColor HighGlowColor = FLinearColor(1.f, 0.75f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	float HighGlowScale = 4.f;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	FLinearColor RiskyGlowColor = FLinearColor(1.f, 0.15f, 0.1f);

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Glow")
	float RiskyGlowScale = 2.f;

	// --- 코스메틱 (비어 있으면 재생 안 함) ---
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|FX")
	TObjectPtr<UNiagaraSystem> HighOpenEffect;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|FX")
	TObjectPtr<USoundBase> HighOpenSound;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|FX")
	TObjectPtr<UNiagaraSystem> RiskyOpenEffect;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|FX")
	TObjectPtr<USoundBase> RiskyOpenSound;

	// --- 문 개방 연출 (입구별로 BP에서 조정) ---
// 닫힌 자세 기준 이동량 (위로 밀기: +Z 문 높이만큼). 부모(루트) 스케일이 1이 아니면 그 배율로 움직임.
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Door")
	FVector DoorOpenLocationOffset = FVector::ZeroVector;

	// 닫힌 자세 기준 로컬 회전량 (경첩식: 피벗이 경첩 위치일 때 Yaw = 90 또는 -90).
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Door")
	FRotator DoorOpenRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Door", meta = (ClampMin = "0.0"))
	float DoorOpenDuration = 1.2f;

	// 이징 세기: 1 = 선형, 2 = 부드러운 가감속, 클수록 시작/끝이 더 완만.
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Door", meta = (ClampMin = "1.0"))
	float DoorEaseExponent = 2.f;

	// 연출이 끝난 뒤 문을 숨길지. 콜리전은 문과 함께 남으므로, 열린 자세가 벽/천장 속일 때만 true (아니면 보이지 않는 벽이 됨).
	UPROPERTY(EditAnywhere, Category = "RewardEntrance|Door")
	bool bHideDoorAfterOpen = false;

private:	

	UPROPERTY(ReplicatedUsing = OnRep_EntranceState)
	EEntranceState EntranceState = EEntranceState::Sealed;

	UFUNCTION()
	void OnRep_EntranceState();

	// bPlayEffects = false면 상태 반영(글로우/블로커)만 (늦게 접속한 클라의 동기화용).
	void ApplyEntranceState(bool bPlayEffects);

	// 서버 전용. "Reward0", "Reward1"... 이름의 씬 컴포넌트 위치에 Pool에서 뽑은 아이템을 스폰.
	void SpawnRewards(const FRewardTierConfig& Config);

	// GapGlow의 원본 Intensity (첫 ApplyEntranceState에서 캡처). -1 = 아직 캡처 전.
	float BaseGlowIntensity = -1.f;

	// 닫힌 문 자세를 첫 ApplyEntranceState에서 캡처 (문이 한 번도 안 움직인 시점).
	void CaptureDoorClosedPose();

	// 0~1 진행도에 맞춰 문의 상대 위치/회전을 설정.
	void SetDoorOpenAlpha(float Alpha);

	FVector DoorClosedLocation = FVector::ZeroVector;
	FQuat DoorClosedRotation = FQuat::Identity;
	bool bDoorClosedPoseCaptured = false;
	float DoorOpenElapsed = 0.f;

};
