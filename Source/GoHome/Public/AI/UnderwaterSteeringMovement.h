// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "UnderwaterSteeringMovement.generated.h"

class USceneComponent;
class UFloatingPawnMovement;

UCLASS( ClassGroup=(AI), meta=(BlueprintSpawnableComponent) )
class GOHOME_API UUnderwaterSteeringMovement : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUnderwaterSteeringMovement();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// AddMovementInput에 넣는 입력 세기. 보통 1.0으로 두고 실제 속도는 MoveSpeed로 조절한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float MoveInputStrength = 1.0f;

	// 몬스터가 이동 방향을 바라보도록 회전하는 속도. 낮을수록 천천히 돈다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float TurnSpeed = 3.0f;

	// 실제 이동 방향을 보간하는 속도. 낮을수록 방향 전환이 부드럽다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
	float SwimDirectionInterpSpeed = 4.0f;

	// 목표 지점에 도착했다고 판단하는 거리.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ArriveDistance = 150.0f;

	// 벽/장애물을 검사할 Sphere Trace 길이.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	float ObstacleDetectDistance = 600.0f;

	// Sphere Trace 반지름. 몬스터 몸 크기에 맞춘다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	float ObstacleTraceRadius = 80.0f;

	// 벽검사에 사용할 Trace 채널. 벽 콜리전이 이 채널을 Block 해야 감지된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	TEnumAsByte<ECollisionChannel> ObstacleTraceChannel = ECC_Visibility;

	// 회피 방향을 한번 정한 뒤 유지하는 시간. 너무 낮으면 방향이 계속 바뀌며 떨린다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	float AvoidLockDuration = 0.25f;

	// 모든 후보 방향이 막혔을 때 뒤로 빠지는 강제 탈출 시간.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	float ForceEscapeDuration = 0.4f;

	// TraceStartComponent가 비어 있을 때 이름으로 찾을 시작점 컴포넌트명.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	FName EyePointComponentName = TEXT("EyePoint");

	// Sphere Trace 시작점. BP에서 EyePoint나 몸 중심 SceneComponent를 넣을 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	TObjectPtr<USceneComponent> TraceStartComponent = nullptr;

	// 현재 회피 방향을 잠깐 고정 중인지 여부.
	UPROPERTY(BlueprintReadOnly, Category = "Avoidance")
	bool bAvoidLocked = false;

	// 회피 방향 고정이 남은 시간.
	UPROPERTY(BlueprintReadOnly, Category = "Avoidance")
	float AvoidLockTime = 0.0f;

	// 현재 고정된 회피 방향.
	UPROPERTY(BlueprintReadOnly, Category = "Avoidance")
	FVector AvoidLockedDir = FVector::ZeroVector;

	// 사방이 막혀 뒤로 빠지는 강제 탈출 상태인지 여부.
	UPROPERTY(BlueprintReadOnly, Category = "Avoidance")
	bool bForceEscape = false;

	// 강제 탈출이 남은 시간.
	UPROPERTY(BlueprintReadOnly, Category = "Avoidance")
	float ForceEscapeTime = 0.0f;

	// 강제 탈출에 사용할 방향. 보통 몬스터의 뒤쪽 방향이다.
	UPROPERTY(BlueprintReadOnly, Category = "Avoidance")
	FVector ForceEscapeDir = FVector::ZeroVector;

	// 이번 프레임에 선택된 최종 조향 방향.
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector CurrentSteeringDir = FVector::ZeroVector;

	// 보간이 적용된 실제 이동 입력 방향.
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector SmoothedMoveDir = FVector::ZeroVector;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:

	// 앞이 막혔을 때 선택 가능한 회피 후보 방향 배열.
	TArray<FVector> CandidateDirs;

public:

	// BP에서 Tick마다 호출하는 메인 이동 함수. TargetLocation으로 가되 앞이 막히면 회피 방향으로 이동한다.
	UFUNCTION(BlueprintCallable, Category = "Underwater Movement")
	void Steering_MoveToLocation(FVector TargetLocation, float MoveSpeed, float DeltaTime);

	// 현재 위치가 TargetLocation에 ArriveDistance 이하로 가까운지 확인한다.
	UFUNCTION(BlueprintCallable, Category = "Underwater Movement")
	bool Steering_HasArrived(FVector TargetLocation) const;

	// 이동 보간값과 회피 상태를 초기화한다. 대기 상태로 들어갈 때 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Underwater Movement")
	void Steering_Stop();

	// Sphere Trace 시작점 컴포넌트를 BP에서 지정한다.
	UFUNCTION(BlueprintCallable, Category = "Underwater Movement")
	void SetTraceStartComponent(USceneComponent* NewTraceStartComponent);

private:
	// FloatingPawnMovement에 이동 입력을 넣고 Actor를 이동 방향으로 부드럽게 회전시킨다.
	void SmoothMoveAndRotate(FVector MoveDir, float MoveSpeed, float DeltaTime);

	// 목표 방향과 벽검사 결과를 합쳐 최종 이동 방향을 정한다.
	FVector GetSteeringDirection(FVector DesiredDir, float DeltaTime);

	// CheckDir 방향으로 Sphere Trace를 쏴서 벽/장애물이 있는지 확인한다.
	bool IsDirectionBlocked(FVector CheckDir) const;

	// 강제 탈출 상태가 살아 있으면 OutDir로 탈출 방향을 반환한다.
	bool UpdateForceEscape(float DeltaTime, FVector& OutDir);

	// CheckDir이 막히지 않았으면 CandidateDirs에 추가한다.
	void AddCandidateIfNotBlocked(FVector CheckDir);

	// 후보 방향이 있으면 하나 고르고, 없으면 BackDir로 강제 탈출한다.
	FVector PickCandidateOrBack(FVector BackDir);

	// Sphere Trace 시작 위치를 구한다.
	FVector GetTraceStartLocation() const;

	// Owner에 붙어 있는 FloatingPawnMovement를 찾는다.
	UFloatingPawnMovement* GetFloatingMovement() const;
		
};
