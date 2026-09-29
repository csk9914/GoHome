#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"
#include "UI/Title/TitlePanelBase.h"
#include "TitleSettingsPanel.generated.h"

class UButton;
class UProgressBar;
class USlider;
class UTextBlock;

/**
 * 설정 패널: 화면 모드 / 해상도 / 마스터 볼륨. 값은 패널 안에만 모았다가 "적용"에서 한 번에 적용·저장한다.
 * 닫으면 적용 안 한 변경은 버린다(다음에 열 때 저장된 값으로 다시 채움).
 */
UCLASS(Abstract)
class GOHOME_API UTitleSettingsPanel : public UTitlePanelBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnPanelOpening() override;
	virtual UWidget* GetInitialFocus() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> FullscreenButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> WindowedButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResolutionPrevButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResolutionNextButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResolutionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MasterVolumeSlider;

	// USlider엔 채움 트랙이 없어 뒤에 깐 ProgressBar로 값만큼 채운다(range accent-color 채움)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> MasterVolumeFill;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MasterVolumeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(EditAnywhere, Category = "Title|Settings|Style")
	FLinearColor SelectedSegmentColor = FLinearColor(0.29f, 0.54f, 0.45f, 0.2f);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FullscreenLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WindowedLabel;

	// .segment.active / .segment
	UPROPERTY(EditAnywhere, Category = "Title|Settings|Style")
	FLinearColor SegmentActiveTextColor = FLinearColor::FromSRGBColor(FColor(0xE9, 0xF0, 0xEC));

	UPROPERTY(EditAnywhere, Category = "Title|Settings|Style")
	FLinearColor SegmentInactiveTextColor = FLinearColor::FromSRGBColor(FColor(0x9E, 0xAF, 0xA7));

	UPROPERTY(EditAnywhere, Category = "Title|Settings|Text")
	FText AppliedMessage = NSLOCTEXT("Title", "SettingsApplied", "설정을 적용했습니다.");

private:
	UFUNCTION()
	void HandleFullscreenClicked();

	UFUNCTION()
	void HandleWindowedClicked();

	UFUNCTION()
	void HandleResolutionPrevClicked();

	UFUNCTION()
	void HandleResolutionNextClicked();

	UFUNCTION()
	void HandleVolumeChanged(float Value);

	UFUNCTION()
	void HandleApplyClicked();

	void StepResolution(int32 Delta);
	void RefreshView();

	TArray<FIntPoint> Resolutions;
	int32 ResolutionIndex = INDEX_NONE;
	EWindowMode::Type PendingWindowMode = EWindowMode::WindowedFullscreen;
	float PendingVolume = 1.f;
};
