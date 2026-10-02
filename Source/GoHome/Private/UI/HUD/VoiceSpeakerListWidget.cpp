#include "UI/HUD/VoiceSpeakerListWidget.h"

#include "Components/PanelWidget.h"
#include "Core/PlayerAvatarSubsystem.h"
#include "Core/VoiceChatSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "UI/HUD/VoiceSpeakerRowWidget.h"

void UVoiceSpeakerListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UVoiceChatSubsystem* Voice = GameInstance->GetSubsystem<UVoiceChatSubsystem>())
		{
			Voice->OnTalkingStateChanged.AddUniqueDynamic(this, &UVoiceSpeakerListWidget::HandleTalkingStateChanged);
		}
		if (UPlayerAvatarSubsystem* Avatars = GameInstance->GetSubsystem<UPlayerAvatarSubsystem>())
		{
			AvatarReadyHandle = Avatars->OnAvatarReady.AddUObject(this, &UVoiceSpeakerListWidget::HandleAvatarReady);
		}
	}

	RefreshLayout();
}

void UVoiceSpeakerListWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UVoiceChatSubsystem* Voice = GameInstance->GetSubsystem<UVoiceChatSubsystem>())
		{
			Voice->OnTalkingStateChanged.RemoveDynamic(this, &UVoiceSpeakerListWidget::HandleTalkingStateChanged);
		}
		if (UPlayerAvatarSubsystem* Avatars = GameInstance->GetSubsystem<UPlayerAvatarSubsystem>())
		{
			Avatars->OnAvatarReady.Remove(AvatarReadyHandle);
		}
	}
	AvatarReadyHandle.Reset();

	Super::NativeDestruct();
}

void UVoiceSpeakerListWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;

	TArray<UVoiceSpeakerRowWidget*> ToRemove;
	for (UVoiceSpeakerRowWidget* Row : Rows)
	{
		APlayerState* PlayerState = Row ? Row->GetPlayerState() : nullptr;
		// 이탈(PlayerState 소멸·목록에서 빠짐)은 페이드 없이 즉시 — 잔류 줄 방지 우선
		const bool bGone = !IsValid(PlayerState) || (GameState && !GameState->PlayerArray.Contains(PlayerState));
		if (!Row || bGone || Row->IsRemovalFinished())
		{
			ToRemove.Add(Row);
		}
	}

	for (UVoiceSpeakerRowWidget* Row : ToRemove)
	{
		RemoveRow(Row);
	}
}

void UVoiceSpeakerListWidget::HandleTalkingStateChanged(APlayerState* Speaker, bool bIsTalking)
{
	if (!Speaker)
	{
		return;
	}

	if (bIsTalking)
	{
		AddOrKeep(Speaker);
		return;
	}

	for (UVoiceSpeakerRowWidget* Row : Rows)
	{
		if (Row && Row->GetPlayerState() == Speaker)
		{
			Row->BeginRemove();
		}
	}
}

void UVoiceSpeakerListWidget::HandleAvatarReady(const FString& NetIdKey)
{
	UGameInstance* GameInstance = GetGameInstance();
	UPlayerAvatarSubsystem* Avatars = GameInstance ? GameInstance->GetSubsystem<UPlayerAvatarSubsystem>() : nullptr;
	if (!Avatars)
	{
		return;
	}

	for (UVoiceSpeakerRowWidget* Row : Rows)
	{
		APlayerState* PlayerState = Row ? Row->GetPlayerState() : nullptr;
		if (PlayerState && UPlayerAvatarSubsystem::MakeKey(PlayerState->GetUniqueId()) == NetIdKey)
		{
			Row->SetAvatar(Avatars->GetAvatar(PlayerState->GetUniqueId()));
		}
	}
}

void UVoiceSpeakerListWidget::AddOrKeep(APlayerState* Speaker)
{
	for (UVoiceSpeakerRowWidget* Row : Rows)
	{
		if (Row && Row->GetPlayerState() == Speaker)
		{
			// 종료 페이드 중 재발화 — 같은 줄 유지
			Row->CancelRemove();
			return;
		}
	}

	if (!RowClass || !SpeakerBox)
	{
		return;
	}

	UVoiceSpeakerRowWidget* Row = CreateWidget<UVoiceSpeakerRowWidget>(this, RowClass);
	if (!Row)
	{
		return;
	}

	Row->Setup(Speaker);
	UGameInstance* GameInstance = GetGameInstance();
	UPlayerAvatarSubsystem* Avatars = GameInstance ? GameInstance->GetSubsystem<UPlayerAvatarSubsystem>() : nullptr;
	Row->SetAvatar(Avatars ? Avatars->GetAvatar(Speaker->GetUniqueId()) : nullptr);

	SpeakerBox->AddChild(Row);
	Rows.Add(Row);
	RefreshLayout();
}

void UVoiceSpeakerListWidget::RemoveRow(UVoiceSpeakerRowWidget* Row)
{
	if (Row)
	{
		Row->RemoveFromParent();
	}
	Rows.Remove(Row);
	RefreshLayout();
}

void UVoiceSpeakerListWidget::RefreshLayout()
{
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (Rows[Index])
		{
			Rows[Index]->SetShowDivider(Index > 0);
		}
	}
	SetVisibility(Rows.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
