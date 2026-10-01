#include "UI/HUD/VoiceSpeakerRowWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerState.h"

void UVoiceSpeakerRowWidget::Setup(APlayerState* InPlayerState)
{
	BoundPlayerState = InPlayerState;
	LastName.Reset();
	AppearElapsed = 0.f;
	PulseElapsed = 0.f;
	bRemoving = false;
	RemoveElapsed = 0.f;
	RefreshName();
	ApplyAvatarBrush(1.f);
}

void UVoiceSpeakerRowWidget::SetAvatar(UTexture2D* Texture)
{
	AvatarTexture = Texture;
	if (InitialText)
	{
		InitialText->SetVisibility(Texture ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	ApplyAvatarBrush(1.f);
}

void UVoiceSpeakerRowWidget::SetShowDivider(bool bShow)
{
	if (Divider)
	{
		Divider->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UVoiceSpeakerRowWidget::BeginRemove()
{
	if (!bRemoving)
	{
		bRemoving = true;
		RemoveElapsed = 0.f;
	}
}

void UVoiceSpeakerRowWidget::CancelRemove()
{
	bRemoving = false;
	RemoveElapsed = 0.f;
	SetRenderOpacity(1.f);
}

void UVoiceSpeakerRowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshName();

	if (bRemoving)
	{
		RemoveElapsed += InDeltaTime;
		const float T = RemoveSeconds > 0.f ? FMath::Clamp(RemoveElapsed / RemoveSeconds, 0.f, 1.f) : 1.f;
		SetRenderOpacity(1.f - T);
		ApplyAvatarBrush(PulseMinOpacity);
		return;
	}

	if (AppearElapsed < AppearSeconds)
	{
		AppearElapsed += InDeltaTime;
		const float T = FMath::Clamp(AppearElapsed / AppearSeconds, 0.f, 1.f);
		const float Eased = 1.f - FMath::Square(1.f - T);
		SetRenderOpacity(Eased);
		SetRenderTranslation(FVector2D(0.f, AppearOffsetY * (1.f - Eased)));
	}

	PulseElapsed = FMath::Fmod(PulseElapsed + InDeltaTime, PulsePeriod);
	// 0 → 반주기에 최저 → 1 (ease-in-out 코사인)
	const float Wave = 0.5f + 0.5f * FMath::Cos(2.f * PI * PulseElapsed / PulsePeriod);
	ApplyAvatarBrush(FMath::Lerp(PulseMinOpacity, 1.f, Wave));
}

void UVoiceSpeakerRowWidget::ApplyAvatarBrush(float RingOpacity)
{
	if (!AvatarImage)
	{
		return;
	}

	// RoundedBox + HalfHeightRadius = 텍스처를 원으로 잘라 그리고, 외곽선을 링으로 쓴다
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.ImageSize = FVector2D(AvatarSize, AvatarSize);
	if (AvatarTexture)
	{
		Brush.SetResourceObject(AvatarTexture);
		Brush.TintColor = FSlateColor(FLinearColor::White);
	}
	else
	{
		Brush.TintColor = FSlateColor(FallbackFillColor);
	}
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	Brush.OutlineSettings.Width = RingWidth;
	FLinearColor Ring = RingColor;
	Ring.A *= RingOpacity;
	Brush.OutlineSettings.Color = FSlateColor(Ring);
	AvatarImage->SetBrush(Brush);
}

void UVoiceSpeakerRowWidget::RefreshName()
{
	const APlayerState* PlayerState = BoundPlayerState.Get();
	const FString Name = PlayerState ? PlayerState->GetPlayerName() : FString();
	if (Name == LastName && !LastName.IsEmpty())
	{
		return;
	}
	LastName = Name;

	if (NameText)
	{
		NameText->SetText(FText::FromString(Name));
	}
	if (InitialText)
	{
		InitialText->SetText(FText::FromString(Name.IsEmpty() ? FString(TEXT("?")) : Name.Left(1).ToUpper()));
	}
}
