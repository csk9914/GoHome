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

	Resolutions.Reset();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);

	const FIntPoint Current = Settings->GetScreenResolution();
	Resolutions.AddUnique(Current);
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X * A.Y < B.X * B.Y; });
	ResolutionIndex = Resolutions.IndexOfByKey(Current);

	MasterVolumeSlider->SetValue(PendingVolume);
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
}
