#include "UI/Title/TitleSettingsPanel.h"

#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Core/GoHomeGameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

void UTitleSettingsPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	FullscreenButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleFullscreenClicked);
	WindowedButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleWindowedClicked);
	ResolutionPrevButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleResolutionPrevClicked);
	ResolutionNextButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleResolutionNextClicked);
	MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleVolumeChanged);
	if (FieldOfViewSlider)
	{
		FieldOfViewSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleFieldOfViewChanged);
	}
	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleMouseSensitivityChanged);
	}
	ApplyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleApplyClicked);
}

void UTitleSettingsPanel::OnPanelOpening()
{
	const UGoHomeGameUserSettings* Settings = UGoHomeGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	PendingWindowMode = Settings->GetFullscreenMode() == EWindowMode::Windowed ? EWindowMode::Windowed : EWindowMode::WindowedFullscreen;
	PendingVolume = Settings->GetMasterVolume();
	PendingFieldOfView = Settings->GetFieldOfView();
	PendingMouseSensitivity = Settings->GetMouseSensitivity();

	Resolutions.Reset();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);

	const FIntPoint Current = Settings->GetScreenResolution();
	Resolutions.AddUnique(Current);
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X * A.Y < B.X * B.Y; });
	ResolutionIndex = Resolutions.IndexOfByKey(Current);

	MasterVolumeSlider->SetValue(PendingVolume);
	if (FieldOfViewSlider)
	{
		FieldOfViewSlider->SetValue((PendingFieldOfView - 70.f) / 40.f);
	}
	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetValue((PendingMouseSensitivity - 0.2f) / 1.8f);
	}
	RefreshView();
}

UWidget* UTitleSettingsPanel::GetInitialFocus() const
{
	return MasterVolumeSlider;
}

void UTitleSettingsPanel::HandleFullscreenClicked()
{
	PendingWindowMode = EWindowMode::WindowedFullscreen;
	RefreshView();
}

void UTitleSettingsPanel::HandleWindowedClicked()
{
	PendingWindowMode = EWindowMode::Windowed;
	RefreshView();
}

void UTitleSettingsPanel::HandleResolutionPrevClicked()
{
	StepResolution(-1);
}

void UTitleSettingsPanel::HandleResolutionNextClicked()
{
	StepResolution(1);
}

void UTitleSettingsPanel::HandleVolumeChanged(float Value)
{
	PendingVolume = Value;
	RefreshView();
}

void UTitleSettingsPanel::HandleFieldOfViewChanged(float Value)
{
	// 시안 단계: 1° 단위
	PendingFieldOfView = FMath::RoundToFloat(FMath::Lerp(70.f, 110.f, Value));
	RefreshView();
}

void UTitleSettingsPanel::HandleMouseSensitivityChanged(float Value)
{
	// 시안 단계: 0.05× 단위
	PendingMouseSensitivity = FMath::GridSnap(FMath::Lerp(0.2f, 2.f, Value), 0.05f);
	RefreshView();
}

void UTitleSettingsPanel::HandleApplyClicked()
{
	UGoHomeGameUserSettings* Settings = UGoHomeGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	Settings->SetFullscreenMode(PendingWindowMode);
	if (Resolutions.IsValidIndex(ResolutionIndex))
	{
		Settings->SetScreenResolution(Resolutions[ResolutionIndex]);
	}
	Settings->SetMasterVolume(PendingVolume);
	Settings->SetFieldOfView(PendingFieldOfView);
	Settings->SetMouseSensitivity(PendingMouseSensitivity);
	Settings->ApplySettings(false);
	Settings->SaveSettings();

	RequestToast(AppliedMessage);
}

void UTitleSettingsPanel::StepResolution(int32 Delta)
{
	if (Resolutions.Num() == 0)
	{
		return;
	}

	ResolutionIndex = FMath::Clamp(ResolutionIndex + Delta, 0, Resolutions.Num() - 1);
	RefreshView();
}

void UTitleSettingsPanel::RefreshView()
{
	const bool bFullscreen = PendingWindowMode != EWindowMode::Windowed;
	FullscreenButton->SetBackgroundColor(bFullscreen ? SelectedSegmentColor : FLinearColor::Transparent);
	WindowedButton->SetBackgroundColor(bFullscreen ? FLinearColor::Transparent : SelectedSegmentColor);
	if (FullscreenLabel)
	{
		FullscreenLabel->SetColorAndOpacity(FSlateColor(bFullscreen ? SegmentActiveTextColor : SegmentInactiveTextColor));
	}
	if (WindowedLabel)
	{
		WindowedLabel->SetColorAndOpacity(FSlateColor(bFullscreen ? SegmentInactiveTextColor : SegmentActiveTextColor));
	}

	if (Resolutions.IsValidIndex(ResolutionIndex))
	{
		const FIntPoint& Res = Resolutions[ResolutionIndex];
		ResolutionText->SetText(FText::FromString(FString::Printf(TEXT("%d × %d"), Res.X, Res.Y)));
	}
	ResolutionPrevButton->SetIsEnabled(ResolutionIndex > 0);
	ResolutionNextButton->SetIsEnabled(ResolutionIndex < Resolutions.Num() - 1);

	MasterVolumeText->SetText(FText::AsNumber(FMath::RoundToInt(PendingVolume * 100.f)));
	if (MasterVolumeFill)
	{
		MasterVolumeFill->SetPercent(PendingVolume);
	}
	const float FieldOfViewAlpha = (PendingFieldOfView - 70.f) / 40.f;
	if (FieldOfViewText)
	{
		FieldOfViewText->SetText(FText::FromString(FString::Printf(TEXT("%d°"), FMath::RoundToInt(PendingFieldOfView))));
	}
	if (FieldOfViewFill)
	{
		FieldOfViewFill->SetPercent(FieldOfViewAlpha);
	}

	const float SensitivityAlpha = (PendingMouseSensitivity - 0.2f) / 1.8f;
	if (MouseSensitivityText)
	{
		MouseSensitivityText->SetText(FText::FromString(FString::Printf(TEXT("%.2f×"), PendingMouseSensitivity)));
	}
	if (MouseSensitivityFill)
	{
		MouseSensitivityFill->SetPercent(SensitivityAlpha);
	}
}
