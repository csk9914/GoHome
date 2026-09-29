#include "Core/GoHomeGameUserSettings.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Sound/AudioSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

UGoHomeGameUserSettings* UGoHomeGameUserSettings::Get()
{
	return GEngine ? Cast<UGoHomeGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UGoHomeGameUserSettings::SetMasterVolume(float InVolume)
{
	MasterVolume = FMath::Clamp(InVolume, 0.f, 1.f);
}

void UGoHomeGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	ApplyAudioSettings();
}

void UGoHomeGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	MasterVolume = 1.f;
}

void UGoHomeGameUserSettings::ApplyAudioSettings()
{
	if (!GEngine)
	{
		return;
	}

	FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice();
	if (!AudioDevice.IsValid())
	{
		return;
	}

	USoundClass* MasterClass = Cast<USoundClass>(GetDefault<UAudioSettings>()->DefaultSoundClassName.TryLoad());
	if (!MasterClass)
	{
		return;
	}

	if (!VolumeMix)
	{
		VolumeMix = NewObject<USoundMix>(this, TEXT("GoHomeVolumeMix"));
	}

	AudioDevice->SetSoundMixClassOverride(VolumeMix, MasterClass, MasterVolume, 1.f, 0.f, true);

	if (!bVolumeMixPushed)
	{
		AudioDevice->PushSoundMixModifier(VolumeMix);
		bVolumeMixPushed = true;
	}
}
