#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Title/TitleUITypes.h"
#include "TitleMenuItemWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UWidgetAnimation;

/**
 * 타이틀 메인 메뉴의 한 행. hover와 키보드/게임패드 포커스를 같은 "강조" 상태로 합친다(hover가 포커스를 가져감).
 * 색·표시 여부는 C++이 적용하고, 이동/페이드 타이밍은 BP의 HighlightAnim이 소유한다.
 */
UCLASS(Abstract)
class GOHOME_API UTitleMenuItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnMenuItemActivated, UTitleMenuItemWidget* /*Item*/);
	FOnMenuItemActivated OnActivated;

	ETitleMenuAction GetAction() const { return Action; }
	bool IsHighlighted() const { return bHighlighted; }

	UFUNCTION(BlueprintCallable, Category = "Title|Menu")
	void FocusItem();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Menu")
	ETitleMenuAction Action = ETitleMenuAction::Host;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Menu")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Menu")
	FText Marker;

	// 지정하면 텍스트 마커 대신 아이콘을 쓴다 — 폰트에 글리프가 없는 기호(⚙ 등)용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Menu")
	TObjectPtr<UTexture2D> MarkerIconTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FVector2D MarkerIconSize = FVector2D(10.f, 10.f);

	// .menu-marker 13px(×0.75). 시안의 ▶/×는 Consolas에 없거나 모양이 달라 Chrome 폴백 글리프(Segoe UI Symbol ▶, Consolas ×)로
	// 그려지므로, 행마다 Noto 글리프 크기/굵기를 그 박스에 맞춘다(▶ 8.5pt, × Bold 6pt)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	float MarkerFontSize = 9.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FName MarkerTypeface = TEXT("Regular");

	// HOST 같은 주 행동 — 강조색을 호박색으로
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Menu")
	bool bPrimary = false;

	// 설정/종료 같은 보조 항목 — 글자를 한 단계 어둡게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Menu")
	bool bSecondary = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FLinearColor AccentColor = FLinearColor::FromSRGBColor(FColor(0x9D, 0xD8, 0xD1));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FLinearColor PrimaryAccentColor = FLinearColor::FromSRGBColor(FColor(0xD9, 0xAE, 0x77));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FLinearColor LabelColor = FLinearColor::FromSRGBColor(FColor(0xE5, 0xEE, 0xEA));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FLinearColor SecondaryLabelColor = FLinearColor::FromSRGBColor(FColor(0xD0, 0xD9, 0xD3));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FLinearColor HighlightLabelColor = FLinearColor::White;

	// .menu-secondary .menu-marker — 보조 항목 마커는 라벨보다 한 단계 어둡다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FLinearColor SecondaryMarkerColor = FLinearColor::FromSRGBColor(FColor(0xA8, 0xB9, 0xB1));

	// UMG 폰트 Size는 72DPI 포인트라 CSS px × 0.75 — 주 항목 16px/600, 보조 14px/500
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	float LabelFontSize = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	float SecondaryLabelFontSize = 10.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FName LabelTypeface = TEXT("SemiBold");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	FName SecondaryLabelTypeface = TEXT("Medium");

	// .045em(16px에서 0.72px → 1px). Slate 자간은 포인트 크기 기준이고 정수 px로 잘리므로
	// em×1000을 그대로 넣으면 0이 된다 — (목표px + 0.5) × 1000 / 12pt
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Style")
	int32 LabelLetterSpacing = 125;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MarkerText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> MarkerIcon;

	// 좌측 3px 강조 막대
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HighlightBar;

	// 좌→우로 옅어지는 배경 워시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HighlightWash;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> HighlightAnim;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	void RefreshHighlight();
	void ApplyStaticStyle();

	bool bHovered = false;
	bool bFocused = false;
	bool bHighlighted = false;
};
