

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RadarSensorComponent.generated.h"

// 핑의 상대 높이. 위젯은 이 값으로 화살표만 고르면 됨
UENUM(BlueprintType)
enum class ERadarDepth : uint8
{
	Same,
	Above,
	Below
};

// 레이더에 찍힌 점 하나. 막대가 몬스터를 스친 "그 순간"의 스냅샷, 이후 움직여도 갱신 X
USTRUCT(BlueprintType)
struct FRadarPing
{
	GENERATED_BODY()

	// 레이더 원 안의 화면 좌표. (0,0)이 중심, 길이 1이 탐지 반경 가장자리.

	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	FVector2D NormalizedPos = FVector2D::ZeroVector;

	// 플레이어 기준 높이차(cm). 양수면 몬스터가 위에 있음
	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	float HeightDelta = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	ERadarDepth Depth = ERadarDepth::Same;
};

// 위젯이 매 프레임 통째로 받아가는 레이더 상태.
USTRUCT(BlueprintType)
struct FRadarSnapshot
{
	GENERATED_BODY()

	// 레이더가 지금 동작 중인가. false면 위젯을 통째로 숨김
	// 손에서 내렸거나 다른 슬롯으로 바꿨을 때 마지막 화면이 남지 않게 하기 위한 신호.
	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	TArray<FRadarPing> Pings;

	// 막대 각도(도). 북쪽 0°, 시계방향.
	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	float SweepAngle = 0.f;

	// false면 대기(페이드) 구간이므로 막대를 숨김
	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	bool bSweeping = false;

	// 핑 투명도. 스윕 중 1, 대기 중 1 -> 0으로 전부 같이 흐려짐
	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	float PingAlpha = 1.f;

	// 대기 구간 진행도 0~1. 충전 게이지용.
	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	float RechargeProgress = 0.f;

	// 시야 콘 회전 각도(도). 북쪽 0° 기준 시계방향.
	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	float ViewYaw = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRadarUpdated, const FRadarSnapshot&, Snapshot);

UCLASS(ClassGroup = (Item), meta = (BlueprintSpawnableComponent))
class GOHOME_API URadarSensorComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	URadarSensorComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// 위젯이 여기에 바인딩, 작동 중이면 매 프레임 스냅샷 하나가 밀려옴
	UPROPERTY(BlueprintAssignable, Category = "Radar")
	FOnRadarUpdated OnRadarUpdated;

protected:

	// 레이더에 잡힐 액터 클래스. 여기 넣은 클래스의 "자식까지" 전부 탐지됨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radar|Detection")
	TArray<TSubclassOf<AActor>> DetectableClasses;

	// 수평 탐지 반경(cm). 이 거리가 레이더 원의 가장자리가 됨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radar|Detection")
	float DetectRadius = 4000.f;

	// 수직 탐지 범위(cm). 위아래로 이만큼 벗어난 몬스터는 잡지 않음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radar|Detection")
	float DetectHeightRange = 2000.f;

	// 이 높이차(cm) 안쪽이면 같은 높이로 봄(화살표 없음)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radar|Detection")
	float DepthArrowThreshold = 300.f;

	// 막대가 한 바퀴 도는 데 걸리는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radar|Cycle")
	float SweepDuration = 1.5f;

	// 한 바퀴 돈 뒤 핑이 전부 사라지기까지의 시간(초). 끝나면 다음 스윕이 시작됨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radar|Cycle")
	float FadeDuration = 2.5f;

private:

	// 새 사이클 시작 - 핑을 비우고 이번 스윕이 훑을 몬스터 목록을 스냅샷
	void BeginSweep();

	// [FromAngle, ToAngle) 구간을 막대가 지나간 것으로 보고 해당 몬스터의 핑을 찍음
	void CapturePings(float FromAngle, float ToAngle);

	bool IsOperational() const;

	// 레이더를 들고 있는(또는 탑승 중인) 폰
	APawn* GetOperatorPawn() const;

	// 시야 콘 각도. 1인칭이라 액터 회전이 아닌 컨트롤 회전을 씀
	float GetViewYaw() const;

	void ResetCycle();

	float CycleTime = 0.f;

	// 지난 프레임의 막대 각도. 구간 검사의 시작점
	float LastSweepAngle = 0.f;

	// 사이클이 돌고 있는 중인가. 꺼졌다 켜지면 항상 새 사이클로 시작하기 위한 플래그
	bool bCycleActive = false;

	// 이번 스윕에서 아직 핑을 안 찍은 몬스터들
	TArray<TWeakObjectPtr<AActor>> PendingMonsters;

	TArray<FRadarPing> Pings;
};