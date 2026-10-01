#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExpeditionProgressHUDWidget.generated.h"

class AGoHomeGameState;
class UBorder;
class UHorizontalBox;
class UImage;
class UProgressBar;
class UTextBlock;
class UWidget;

// 남은 시간 경고 단계 — 색·라벨·깜빡임을 고른다
enum class ETimeLimitStage : uint8
{
	Normal,
	Warning,
	Critical,
	TimeUp,
};

/**
 * 로비·탐사 공통 상시 진행도 HUD(미션 패널, Reference Pack: Docs/Dev/UI/expedition-progress-hud 후보 A).
 * 헤더 ROUND·경고 칩 / 남은 시간 + 20칸 세그먼트(탐사맵) / 할당량 한 줄 + 바 + 달성 칩(탐사맵) / 다음 관문 대비 자금 + 바 + 부족·충족 칩.
 * 값은 AGoHomeGameState 복제 미러(공유 헤더)만 읽는다 — 할당량·제한시간도 GetMapQuotaProgress()/GetTimeLimitProgress()로 받아 탐사 클래스를 모른다.
 * 생성/재생성은 AGoHomePlayerController 가 GameState 마다 하므로 이 위젯은 생성 시점의 GameState 하나에만 바인딩한다.
 * 레이아웃·스타일은 BP(WBP_ExpeditionProgressHUD), 상태·값·모션은 여기.
 */
UCLASS(Abstract)
class GOHOME_API UExpeditionProgressHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ── 헤더 ──
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Round;

	// 경고 단계 칩(테두리 색 = 단계 색). Normal·로비에선 숨김
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Chip_TimeState;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_TimeState;

	// 로비에서만 보이는 "로비" 태그
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Txt_LobbyTag;

	// ── 시간(탐사맵 + 제한시간 있을 때만) ──
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> TimeSection;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Time;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_TimeTotal;

	// 비어 있는 HorizontalBox — 세그먼트 Image 를 여기서 생성해 채운다(산소 게이지 핍과 같은 방식)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> TimeSegments;

	// ── 할당량(탐사맵만) ──
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> QuotaSection;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_QuotaValue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_QuotaMax;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_QuotaPct;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Chip_QuotaMet;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Quota;

	// ── 다음 관문 대비 자금 ──
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_CheckPointLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Funds;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_CheckPointTarget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Chip_Short;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Short;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Chip_CheckPointMet;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_CheckPoint;

	// ── 경고 단계 ──
	// 남은 비율이 이 값 이하면 경고(주황), Critical 이하면 위험(빨강 + 깜빡임)
	UPROPERTY(EditAnywhere, Category = "Progress HUD|Time Limit", meta = (ClampMin = "0", ClampMax = "1"))
	float WarningRatio = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Time Limit", meta = (ClampMin = "0", ClampMax = "1"))
	float CriticalRatio = 0.10f;

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Time Limit", meta = (ClampMin = "0.1"))
	float CriticalBlinkPeriod = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Time Limit", meta = (ClampMin = "0", ClampMax = "1"))
	float CriticalBlinkMinOpacity = 0.35f;

	// 단계가 바뀔 때 색 전환 시간(motion.md 200ms linear)
	UPROPERTY(EditAnywhere, Category = "Progress HUD|Time Limit", meta = (ClampMin = "0"))
	float StageColorBlendSeconds = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Time Limit", meta = (ClampMin = "1", ClampMax = "60"))
	int32 TimeSegmentCount = 20;

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Time Limit")
	FVector2D TimeSegmentSize = FVector2D(9.f, 6.f);

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Time Limit")
	float TimeSegmentGap = 3.f;

	// 꺼진 세그먼트 불투명도(색은 현재 단계 색)
	UPROPERTY(EditAnywhere, Category = "Progress HUD|Time Limit", meta = (ClampMin = "0", ClampMax = "1"))
	float TimeSegmentOffOpacity = 0.14f;

	// ── 색(Pack 토큰) ──
	// 산소 게이지 실측 #00E8F0 계열
	UPROPERTY(EditAnywhere, Category = "Progress HUD|Color")
	FLinearColor HudCyanColor = FLinearColor::FromSRGBColor(FColor(0x19, 0xE3, 0xEE));

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Color")
	FLinearColor InkColor = FLinearColor::FromSRGBColor(FColor(0xEE, 0xF6, 0xF6));

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Color")
	FLinearColor WarningColor = FLinearColor::FromSRGBColor(FColor(0xFF, 0xB5, 0x47));

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Color")
	FLinearColor CriticalColor = FLinearColor::FromSRGBColor(FColor(0xFF, 0x4D, 0x4D));

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Color")
	FLinearColor OkColor = FLinearColor::FromSRGBColor(FColor(0x5E, 0xF0, 0xB0));

	// 값 갱신 하이라이트 시작색(→ InkColor 로 돌아온다)
	UPROPERTY(EditAnywhere, Category = "Progress HUD|Color")
	FLinearColor ValueFlashColor = FLinearColor::White;

	// ── 모션 ──
	UPROPERTY(EditAnywhere, Category = "Progress HUD|Motion", meta = (ClampMin = "0"))
	float AppearSeconds = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Motion", meta = (ClampMin = "0"))
	float CountUpSeconds = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Motion", meta = (ClampMin = "0"))
	float ValueFlashSeconds = 0.45f;

	// ── 문구 ──
	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText RoundFormat = NSLOCTEXT("ProgressHUD", "Round", "ROUND {Current} / {Final}");

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText TimeTotalFormat = NSLOCTEXT("ProgressHUD", "TimeTotal", "/ {Total} 남음");

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText WarningStateText = NSLOCTEXT("ProgressHUD", "Warning", "▲ 경고");

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText CriticalStateText = NSLOCTEXT("ProgressHUD", "Critical", "■ 위험");

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText TimeUpStateText = NSLOCTEXT("ProgressHUD", "TimeUp", "■ 종료");

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText CheckPointLabelFormat = NSLOCTEXT("ProgressHUD", "CheckPointLabel", "T{Round} 관문");

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText AllClearedLabelText = NSLOCTEXT("ProgressHUD", "AllClearedLabel", "관문");

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText AllClearedText = NSLOCTEXT("ProgressHUD", "AllCleared", "모든 관문 통과");

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText ShortFormat = NSLOCTEXT("ProgressHUD", "Short", "−{Short}");

	UPROPERTY(EditAnywhere, Category = "Progress HUD|Text")
	FText PendingText = NSLOCTEXT("ProgressHUD", "Pending", "—");

private:
	UFUNCTION()
	void HandleProgressChanged();

	// 델리게이트 시점: 목표값·표시 여부·칩을 갱신하고 카운트업을 시작한다
	void Refresh();

	// 매 틱: 남은 시간·세그먼트·단계 색·깜빡임
	void TickTimeLimit(float DeltaTime);

	// 매 틱: 카운트업 보간·하이라이트·등장 페이드
	void TickValues(float DeltaTime);

	void ApplyDisplayedValues();
	void BuildTimeSegments();
	FLinearColor StageColor(ETimeLimitStage Stage) const;

	TWeakObjectPtr<AGoHomeGameState> BoundGameState;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> SegmentImages;

	// 카운트업: 표시값이 From→To 로 CountUpSeconds 동안 이동
	struct FCountUp
	{
		float From = 0.f;
		float To = 0.f;
		float Displayed = 0.f;
		float Elapsed = 0.f;
		float FlashElapsed = 0.f;
		bool bInitialized = false;

		void SetTarget(float NewTarget, float InCountUpSeconds);
		bool Tick(float DeltaTime, float InCountUpSeconds, float InFlashSeconds);
	};
	FCountUp FundsValue;
	FCountUp DeliveredValue;

	bool bPending = true;
	bool bHasQuota = false;
	int32 MapQuota = 0;
	int32 CheckPointQuota = 0;
	bool bHasCheckPoint = false;

	float AppearElapsed = 0.f;

	bool bTimeSectionVisible = true;
	int32 LastDisplayedSeconds = INDEX_NONE;
	ETimeLimitStage LastStage = ETimeLimitStage::Normal;
	FLinearColor CurrentStageColor = FLinearColor::Transparent;
	int32 LastSegmentsOn = INDEX_NONE;
};
