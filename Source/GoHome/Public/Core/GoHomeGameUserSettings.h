#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "GoHomeGameUserSettings.generated.h"

class USoundMix;

/**
 * 프로젝트 사용자 설정. 화면 설정은 엔진 기본 동작, 여기선 마스터 볼륨만 추가한다.
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

	virtual void ApplyNonResolutionSettings() override;
	virtual void SetToDefaults() override;

private:
	void ApplyAudioSettings();

	UPROPERTY(Config)
	float MasterVolume = 1.f;

	// 엔진 기본 사운드 클래스(Master)에 볼륨을 거는 런타임 전용 믹스 — 메인 오디오 디바이스에 한 번만 푸시한다.
	UPROPERTY(Transient)
	TObjectPtr<USoundMix> VolumeMix;

	bool bVolumeMixPushed = false;
};
