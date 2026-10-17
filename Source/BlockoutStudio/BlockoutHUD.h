// BlockoutStudio - HUD：gizmo 渲染、框选矩形、测量标注、状态栏
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlockoutTypes.h"
#include "BlockoutHUD.generated.h"

class ABlockoutActor;

UCLASS()
class BLOCKOUTSTUDIO_API ABlockoutHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	// 测量标注列表（测量工具写入）
	TArray<FBlockoutMeasureItem> MeasureItems;

	// T1：实时预览（MeasureStage=1 时显示 A→当前光标）
	bool bMeasurePreview = false;
	FBlockoutMeasureItem MeasurePreview;

	// 瞬时提示消息
	void ShowToast(const FString& Msg, float Duration = 3.f);

private:
	void DrawGizmo();
	void DrawBoxSelection();
	void DrawMeasureItems();
	void DrawStatusBar();
	void DrawToasts();

	FVector2D ProjectW(const FVector& WorldLoc, bool& bOutVisible) const;

	struct FToast { FString Msg; float TimeLeft; };
	TArray<FToast> Toasts;
};
