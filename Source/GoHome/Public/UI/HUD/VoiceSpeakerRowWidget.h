#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VoiceSpeakerRowWidget.generated.h"

class APlayerState;
class UImage;
class UTextBlock;
class UTexture2D;
class UWidget;

/**
 * 음성 목록 한 줄: 원형 아바타(Steam 없으면 이니셜) + 이름. 발화 중 아바타 외곽 링이 맥동한다.
 * 등장(위로 4px 페이드인)·퇴장(페이드아웃 후 IsRemovalFinished) 모션도 여기서 — 목록은 추가/제거만 결정한다.
 * Reference Pack: Docs/Dev/UI/voice-chat (A안 + 아바타).
 */
UCLASS(Abstract)
class GOHOME_API UVoiceSpeakerRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(APlayerState* InPlayerState);
	void SetAvatar(UTexture2D* Texture);
	void SetShowDivider(bool bShow);

	// 발화 종료: 페이드아웃 시작. 끝나기 전에 다시 말하면 CancelRemove
	void BeginRemove();
	void CancelRemove();
	bool IsRemoving() const { return bRemoving; }
	bool IsRemovalFinished() const { return bRemoving && RemoveElapsed >= RemoveSeconds; }

	APlayerState* GetPlayerState() const { return BoundPlayerState.Get(); }

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AvatarImage;

	// 아바타가 없을 때 원 위에 이름 첫 글자
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InitialText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	// 두 번째 줄부터 위쪽 구분선
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Divider;

	UPROPERTY(EditAnywhere, Category = "Voice Row|Style")
	float AvatarSize = 22.f;

	UPROPERTY(EditAnywhere, Category = "Voice Row|Style")
	float RingWidth = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Voice Row|Style")
	FLinearColor RingColor = FLinearColor::FromSRGBColor(FColor(0x64, 0xD8, 0xC1));

	// 아바타 없음(이니셜) 원 채움색
	UPROPERTY(EditAnywhere, Category = "Voice Row|Style")
	FLinearColor FallbackFillColor = FLinearColor::FromSRGBColor(FColor(0x16, 0x30, 0x2F));

	// motion.md: 발화 중 링 밝기 .4 ↔ 1, 1150ms 주기
	UPROPERTY(EditAnywhere, Category = "Voice Row|Motion", meta = (ClampMin = "0.1"))
	float PulsePeriod = 1.15f;

	UPROPERTY(EditAnywhere, Category = "Voice Row|Motion", meta = (ClampMin = "0", ClampMax = "1"))
	float PulseMinOpacity = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Voice Row|Motion", meta = (ClampMin = "0"))
	float AppearSeconds = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Voice Row|Motion")
	float AppearOffsetY = 4.f;

	UPROPERTY(EditAnywhere, Category = "Voice Row|Motion", meta = (ClampMin = "0"))
	float RemoveSeconds = 0.1f;

private:
	void ApplyAvatarBrush(float RingOpacity);
	void RefreshName();

	TWeakObjectPtr<APlayerState> BoundPlayerState;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> AvatarTexture;

	FString LastName;
	float PulseElapsed = 0.f;
	float AppearElapsed = 0.f;
	float RemoveElapsed = 0.f;
	bool bRemoving = false;
};
