#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "GoHomeGameUserSettings.generated.h"

class USoundMix;

// ApplySettings 직후(FOV·감도 등 게임플레이 설정 변경) — 로컬 컨트롤러가 즉시 재적용한다
DECLARE_MULTICAST_DELEGATE(FOnGameplaySettingsApplied);

/**
 * 프로젝트 사용자 설정. 엔진 화면 설정에 마스터 볼륨, 시야각, 마우스 감도를 추가한다.
 * DefaultEngine.ini [/Script/Engine.Engine] GameUserSettingsClassName으로 지정.
 */
UCLASS(Config = GameUserSettings, ConfigDoNotCheckDefaults)
class GOHOME_API UGoHomeGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static UGoHomeGameUserSettings* Get();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMasterVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetFieldOfView(float InFieldOfView);

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetFieldOfView() const { return FieldOfView; }

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMouseSensitivity(float InMouseSensitivity);

	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetMouseSensitivity() const { return MouseSensitivity; }

	FOnGameplaySettingsApplied OnGameplaySettingsApplied;

	virtual void ApplyNonResolutionSettings() override;
	virtual void SetToDefaults() override;

private:
	void ApplyAudioSettings();

	UPROPERTY(Config)
	float MasterVolume = 1.f;

	UPROPERTY(Config)
	float FieldOfView = 90.f;

	UPROPERTY(Config)
	float MouseSensitivity = 1.f;

	// 엔진 기본 사운드 클래스(Master)에 볼륨을 거는 런타임 전용 믹스 — 메인 오디오 디바이스에 한 번만 푸시한다.
	UPROPERTY(Transient)
	TObjectPtr<USoundMix> VolumeMix;

	bool bVolumeMixPushed = false;
};
