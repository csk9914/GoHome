#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VoiceSpeakerListWidget.generated.h"

class APlayerState;
class UPanelWidget;
class UVoiceSpeakerRowWidget;

/**
 * 좌상단 음성 목록(Reference Pack: Docs/Dev/UI/voice-chat A안 + Steam 아바타).
 * UVoiceChatSubsystem::OnTalkingStateChanged 를 구독해 말하는 플레이어마다 한 줄 — 발화 종료 시 페이드아웃 후 제거,
 * PlayerState 가 사라지면(이탈) 즉시 제거, 아무도 안 말하면 블록 전체를 접는다.
 * 이벤트는 이 머신의 OSS voice 가 알리는 상태 그대로(근접 뮤트로 안 들리는 사람은 안 뜬다) — 복제 상태 없음.
 * 레이아웃·스타일은 BP(WBP_VoiceSpeakerList), 줄 클래스는 RowClass.
 */
UCLASS(Abstract)
class GOHOME_API UVoiceSpeakerListWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 줄을 쌓는 컨테이너(VerticalBox)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SpeakerBox;

	UPROPERTY(EditAnywhere, Category = "Voice List")
	TSubclassOf<UVoiceSpeakerRowWidget> RowClass;

private:
	UFUNCTION()
	void HandleTalkingStateChanged(APlayerState* Speaker, bool bIsTalking);

	void HandleAvatarReady(const FString& NetIdKey);

	void AddOrKeep(APlayerState* Speaker);
	void RemoveRow(UVoiceSpeakerRowWidget* Row);
	void RefreshLayout();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UVoiceSpeakerRowWidget>> Rows;

	FDelegateHandle AvatarReadyHandle;
};
