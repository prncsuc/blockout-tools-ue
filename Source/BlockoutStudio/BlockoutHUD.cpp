// BlockoutStudio - HUD 实现
#include "BlockoutHUD.h"
#include "BlockoutGizmo.h"
#include "BlockoutActor.h"
#include "BlockoutEditorSubsystem.h"
#include "BlockoutEditorPlayerController.h"
#include "BlockoutMeasureLib.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

void ABlockoutHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	// toast 倒计时
	const float Dt = FApp::GetDeltaTime();
	for (FToast& T : Toasts) { T.TimeLeft -= Dt; }

	DrawGizmo();
	DrawBoxSelection();
	DrawMeasureItems();
	DrawStatusBar();
	DrawToasts();
}

FVector2D ABlockoutHUD::ProjectW(const FVector& WorldLoc, bool& bOutVisible) const
{
	FVector2D Out = FVector2D::ZeroVector;
	APlayerController* PC = GetOwningPlayerController();
	bOutVisible = PC && PC->ProjectWorldLocationToScreen(WorldLoc, Out, false);
	return Out;
}

void ABlockoutHUD::DrawGizmo()
{
	ABlockoutEditorPlayerController* PC = Cast<ABlockoutEditorPlayerController>(GetOwningPlayerController());
	if (!PC) return;

	// 查找 gizmo（transient actor）
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABlockoutGizmo::StaticClass(), Found);
	if (Found.Num() == 0) return;
	ABlockoutGizmo* Gizmo = Cast<ABlockoutGizmo>(Found[0]);
	if (!Gizmo || Gizmo->IsHidden()) return;

	const FVector Origin = Gizmo->Location;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const FVector Dir = Gizmo->GetAxisVector(Axis).GetSafeNormal();
		const FVector End = Origin + Dir * ABlockoutGizmo::AxisLength * Gizmo->DisplayScale;

		bool bVisA = false, bVisB = false;
		const FVector2D SA = ProjectW(Origin, bVisA);
		const FVector2D SB = ProjectW(End, bVisB);
		if (!bVisA || !bVisB) continue;

		const bool bHot = (Gizmo->HotAxis == Axis) || Gizmo->bDragging && Gizmo->HotAxis == Axis;
		FLinearColor Col = (Axis == 0) ? FLinearColor::Red : (Axis == 1) ? FLinearColor::Green : FLinearColor::Blue;
		if (Gizmo->HotAxis == Axis) Col = FLinearColor::Yellow;

		DrawLine(SA.X, SA.Y, SB.X, SB.Y, Col, bHot ? 4.f : 2.f);

		// 轴向箭头/方块标记
		const float M = bHot ? 6.f : 4.f;
		DrawRect(Col, SB.X - M, SB.Y - M, M * 2, M * 2);
	}

	// 中心标记
	bool bVis = false;
	const FVector2D SC = ProjectW(Origin, bVis);
	if (bVis)
	{
		DrawRect(FLinearColor::White, SC.X - 3, SC.Y - 3, 6, 6);
	}
}

void ABlockoutHUD::DrawBoxSelection()
{
	ABlockoutEditorPlayerController* PC = Cast<ABlockoutEditorPlayerController>(GetOwningPlayerController());
	if (!PC || !PC->IsBoxSelecting()) return;

	const FVector2D A = PC->GetBoxStart();
	const FVector2D B = PC->GetBoxEnd();
	const float X = FMath::Min(A.X, B.X), Y = FMath::Min(A.Y, B.Y);
	const float W = FMath::Abs(B.X - A.X), H = FMath::Abs(B.Y - A.Y);

	DrawRect(FLinearColor(0.2f, 0.6f, 1.f, 0.15f), X, Y, W, H);
	DrawLine(X, Y, X + W, Y, FLinearColor(0.3f, 0.7f, 1.f), 1.5f);
	DrawLine(X + W, Y, X + W, Y + H, FLinearColor(0.3f, 0.7f, 1.f), 1.5f);
	DrawLine(X + W, Y + H, X, Y + H, FLinearColor(0.3f, 0.7f, 1.f), 1.5f);
	DrawLine(X, Y + H, X, Y, FLinearColor(0.3f, 0.7f, 1.f), 1.5f);
}

void ABlockoutHUD::DrawMeasureItems()
{
	for (const FBlockoutMeasureItem& Item : MeasureItems)
	{
		bool bA = false, bB = false;
		const FVector2D SA = ProjectW(Item.Start, bA);
		const FVector2D SB = ProjectW(Item.End, bB);
		if (bA && bB)
		{
			DrawLine(SA.X, SA.Y, SB.X, SB.Y, FLinearColor::Yellow, 2.f);
			DrawRect(FLinearColor::Yellow, SA.X - 3, SA.Y - 3, 6, 6);
			DrawRect(FLinearColor::Yellow, SB.X - 3, SB.Y - 3, 6, 6);
		}
		if (!Item.Label.IsEmpty())
		{
			const FVector2D Mid = (SA + SB) * 0.5f;
			DrawText(Item.Label, FLinearColor::Yellow, Mid.X + 6, Mid.Y - 8);
		}
	}
}

void ABlockoutHUD::DrawStatusBar()
{
	ABlockoutEditorPlayerController* PC = Cast<ABlockoutEditorPlayerController>(GetOwningPlayerController());
	UBlockoutEditorSubsystem* Sub = nullptr;
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		Sub = GI->GetSubsystem<UBlockoutEditorSubsystem>();
	}

	const FString ModeStr = Sub
		? (Sub->GetTransformMode() == EBlockoutTransformMode::Translate ? TEXT("移动")
			: Sub->GetTransformMode() == EBlockoutTransformMode::Rotate ? TEXT("旋转") : TEXT("缩放"))
		: TEXT("-");

	const FString SnapStr = Sub
		? FString::Printf(TEXT("网格[%s %g] 表面[%s] 坐标[%s]"),
			Sub->GetGridSnap() ? TEXT("ON") : TEXT("OFF"), Sub->GetSnapStep(),
			Sub->GetSurfaceSnap() ? TEXT("ON") : TEXT("OFF"),
			Sub->GetCoordSpace() == EBlockoutCoordSpace::World ? TEXT("世界") : TEXT("本地"))
		: TEXT("");

	const FString SelStr = Sub
		? FString::Printf(TEXT("选中 %d / 共 %d"), Sub->GetSelection().Num(), Sub->GetAllBlocks().Num())
		: TEXT("");

	const FString Line = FString::Printf(TEXT("BlockoutStudio  |  模式:%s  |  %s  |  %s  |  W/E/R 变换  右键旋转视角  Tab 隐藏UI"),
		*ModeStr, *SnapStr, *SelStr);

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0, Canvas->SizeY - 34, Canvas->SizeX, 34);
	DrawText(Line, FLinearColor::White, 12, Canvas->SizeY - 26);
}

void ABlockoutHUD::DrawToasts()
{
	float Y = 60.f;
	for (int32 i = Toasts.Num() - 1; i >= 0; --i)
	{
		if (Toasts[i].TimeLeft <= 0.f) { Toasts.RemoveAt(i); continue; }
	}
	for (FToast& T : Toasts)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), Canvas->SizeX * 0.5f - 220, Y, 440, 30);
		DrawText(T.Msg, FLinearColor::White, Canvas->SizeX * 0.5f - 210, Y + 5);
		Y += 36.f;
	}
}

void ABlockoutHUD::ShowToast(const FString& Msg, float Duration)
{
	FToast T;
	T.Msg = Msg;
	T.TimeLeft = Duration;
	Toasts.Add(T);
}
