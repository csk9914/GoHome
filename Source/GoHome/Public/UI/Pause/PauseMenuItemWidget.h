#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Pause/PauseUIStyle.h"
#include "PauseMenuItemWidget.generated.h"

class UButton;
class UHairlineWidget;
class UImage;
class UTextBlock;

UENUM(BlueprintType)
enum class EPauseMenuAction : uint8
{
	Resume,
	Settings,
	LeaveToTitle,
	QuitGame,
};

/**
 * 인게임 시스템 메뉴의 한 행(.menu-action). hover와 키보드/게임패드 포커스를 같은 "강조" 상태로 합친다(hover가 포커스를 가져감) —
 * 마우스와 키보드가 서로 다른 행을 동시에 강조하지 않게. 색·표시 여부는 C++, 레이아웃·폰트는 BP.
 */
UCLASS(Abstract)
class GOHOME_API UPauseMenuItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPauseMenuItemActivated, UPauseMenuItemWidget* /*Item*/);
	FOnPauseMenuItemActivated OnActivated;

	EPauseMenuAction GetAction() const { return Action; }
	bool IsHighlighted() const { return bHighlighted; }

	void FocusItem();

	// 위/아래 순환 이동을 메뉴가 명시 규칙으로 걸 때 쓰는 실제 포커스 대상
	UWidget* GetFocusTarget() const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Menu")
	EPauseMenuAction Action = EPauseMenuAction::Resume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Menu")
	FText Label;

	// 오른쪽 끝 기호(.action-arrow) — 복귀는 ↵, 나머지는 →
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Menu")
	FText Glyph = FText::FromString(TEXT("→"));

	// --- 시안 색(sRGB hex → 리니어) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Style")
	FLinearColor LabelColor = PauseUIStyle::SRGBA(0xE8, 0xEE, 0xE9);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Style")
	FLinearColor MarkerColor = PauseUIStyle::SRGBA(0xAD, 0xBA, 0xAF);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Style")
	FLinearColor AccentColor = PauseUIStyle::SRGBA(0xD9, 0xAE, 0x77);

	// .action-arrow #a5b8ae, opacity .7
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Style")
	FLinearColor GlyphColor = PauseUIStyle::SRGBA(0xA5, 0xB8, 0xAE, 0.7f);

	// rgba(149,194,177,.1) — UE는 리니어 공간 합성이라 CSS 알파보다 낮춰야 같은 밝기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Style")
	FLinearColor WashColor = PauseUIStyle::SRGBA(149, 194, 177, 0.05f);

	// 행 구분선 rgba(205,226,216,.16) / 강조 rgba(160,207,192,.38)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Style")
	FLinearColor LineColor = PauseUIStyle::SRGBA(205, 226, 216, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pause|Style")
	FLinearColor HighlightLineColor = PauseUIStyle::SRGBA(160, 207, 192, 0.22f);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MarkerText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GlyphText;

	// 좌측 2px 호박색 막대(inset box-shadow)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HighlightBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HighlightWash;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHairlineWidget> BottomLine;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	void ApplyStaticStyle();
	void RefreshHighlight();

	bool bHovered = false;
	bool bFocused = false;
	bool bHighlighted = false;
};
