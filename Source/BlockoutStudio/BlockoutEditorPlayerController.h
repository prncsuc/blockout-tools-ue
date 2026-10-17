// BlockoutStudio - 编辑器输入层：相机飞行、拾取、框选、gizmo 拖拽、快捷键
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlockoutTypes.h"
#include "BlockoutEditorPlayerController.generated.h"

class ABlockoutActor;
class ABlockoutGizmo;
class SBlockoutEditorUI;

UCLASS()
class BLOCKOUTSTUDIO_API ABlockoutEditorPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	// 屏幕射线（世界空间）
	bool GetMouseRay(FVector& OutOrigin, FVector& OutDir) const;

	// 状态查询（HUD/UI 读取）
	bool IsBoxSelecting() const { return bBoxSelecting; }
	FVector2D GetBoxStart() const { return BoxStart; }
	FVector2D GetBoxEnd() const { return BoxEnd; }
	bool IsUIMode() const { return bUIMode; }

	// 对外动作（Slate UI 按钮调用）
	void CreateBlockAtCursor(EBlockoutShape Shape);
	void FocusSelection();
	void ToggleUIShow();

private:
	// ---------- 相机 ----------
	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);
	void UpdateCamera(float DeltaSeconds);
	FVector CamVelocity = FVector::ZeroVector;
	bool bRotateCamera = false;
	float CameraSpeed = 800.f;

	// ---------- 鼠标 ----------
	void OnLeftMouseDown();
	void OnLeftMouseUp();
	void OnRightMouseDown();
	void OnRightMouseUp();
	void OnMouseMoveX(float Value);
	void OnMouseMoveY(float Value);
	void OnMouseWheel(float Value);
	float MouseDX = 0.f;
	float MouseDY = 0.f;

	// ---------- 快捷键 ----------
	void OnKeyModeTranslate();
	void OnKeyModeRotate();
	void OnKeyModeScale();
	void OnKeyDelete();
	void OnKeyUndo();
	void OnKeyRedo();
	void OnKeyDuplicate();
	void OnKeyToggleGridSnap();
	void OnKeyToggleSurfaceSnap();
	void OnKeyToggleCoordSpace();
	void OnKeyFocus();
	void OnKeySelectAll();
	void OnKeySave();
	void OnKeyEscape();
	void OnKeyGroup();
	void OnKeyUngroup();
	void OnKeyToggleUI();

	// ---------- 拾取 ----------
	ABlockoutActor* PickActor(const FVector& RayOrigin, const FVector& RayDir) const;

	// 交互状态
	bool bBoxSelecting = false;
	bool bGizmoDragging = false;
	bool bUIMode = false;
	FVector2D BoxStart = FVector2D::ZeroVector;
	FVector2D BoxEnd = FVector2D::ZeroVector;
	FVector2D LastMousePos = FVector2D::ZeroVector;

	// gizmo 引用
	UPROPERTY() ABlockoutGizmo* GizmoActor = nullptr;
	ABlockoutGizmo* EnsureGizmo();

	// Slate 工具 UI
	TSharedPtr<SBlockoutEditorUI> SlateUI;
};
