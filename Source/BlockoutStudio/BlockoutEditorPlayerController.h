// BlockoutStudio - 编辑器输入层：相机飞行、拾取、框选、gizmo 拖拽、快捷键
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlockoutTypes.h"
#include "BlockoutRecordLib.h"
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

	// ---------- P4 测试模式（FPS/TPS 跑图） ----------
	void ToggleTestMode();
	bool IsTestMode() const { return bTestMode; }
	FString GetTestStatusText() const;

	// ---------- R1-R3 录制 ----------
	void StartRecording();
	void StopRecording();
	bool IsRecording() const { return RecSession.bRecording; }
	void StartPlayback();
	void StopPlayback();
	bool IsPlayingBack() const { return bPlayingBack; }
	// 保存录制产物（轨迹 JSON + MJPEG-AVI）
	bool SaveRecording(const FString& BaseName);

	// ---------- M3 点线面编辑 ----------
	// 子对象选择模式：0=物体 1=顶点 2=面
	void SetSubEditMode(int32 Mode) { SubEditMode = Mode; }
	int32 GetSubEditMode() const { return SubEditMode; }
	const TArray<int32>& GetSelectedVerts() const { return SelectedVerts; }
	const TArray<int32>& GetSelectedFaces() const { return SelectedFaces; }
	ABlockoutActor* GetEditTarget() const { return EditTarget; }
	// 编辑动作
	void MeshExtrudeFaces(float Distance);
	void MeshBevelVerts(float Amount);
	void MeshCollapseVerts();
	void MeshDeleteFaces();
	void MeshMoveVerts(const FVector& Delta);
	// ProBuilder 补齐动作
	void MeshWeldVerts();
	void MeshInsetFaces();
	void MeshSubdivideFaces();
	void MeshFlipFaces();
	void MeshFillHoles();

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
	void OnKeyToggleRecord();
	void OnKeyToggleSubEdit();
	void OnKeyExtrude();
	void OnKeyBevel();
	void OnKeyCollapse();
	void OnKeyInset();
	void OnKeySubdivide();
	void OnKeyFlipFaces();
	void OnKeyFillHoles();
	void OnKeyWeld();

public:
	// T1 测距：UI 入口
	void OnKeyToggleMeasure();

private:
	// ---------- T1 两点测距 ----------
	void OnKeyMeasureConfirm();            // Enter 放置标注
	bool PickWorldPoint(FVector& OutPoint) const; // 射线拾取世界点（网格命中→地面兜底）
	bool bMeasureMode = false;   // 测距模式开关
	int32 MeasureStage = 0;      // 0=待取第一点 1=已取第一点待确认
	FVector MeasurePointA = FVector::ZeroVector;

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

	// ---------- R1-R3 录制状态 ----------
	FBlockoutRecordSession RecSession;
	bool bPlayingBack = false;
	double PlayStartTime = 0.0;
	double LastFrameCaptureTime = 0.0;
	// 逐帧采样相机（轨迹帧 + MJPEG 视口帧）
	void CaptureRecordFrame();
	// 视口 JPEG 采集（MJPEG-AVI 用）
	void CaptureViewportJpeg();

	// ---------- 场景捕获（SceneCapture 读回，Shipping 下可靠） ----------
	UPROPERTY() class USceneCaptureComponent2D* DemoCapture = nullptr;
	UPROPERTY() class UTextureRenderTarget2D* DemoRT = nullptr;
	bool EnsureDemoCapture();
	bool CaptureScenePixels(TArray<FColor>& OutPixels, int32& OutW, int32& OutH);

	// ---------- P5 自动演示（-AutoDemoLevel= / -AutoDemoOut=） ----------
	bool bAutoDemo = false;
	bool bAutoDemoLoaded = false;
	FString AutoDemoLevelPath;
	FString AutoDemoOutDir;
	int32 AutoDemoPhase = 0;      // 0=等待载入 1=鸟瞰截图 2=跑图录制 3=收尾
	float AutoDemoTimer = 0.0f;
	int32 AutoDemoShotIdx = 0;
	int32 AutoDemoPlacedShot = -1;   // 已摆好机位的 ShotIdx（-1=未摆），避免依赖帧时序
	bool bDemoFollowCam = false;     // 跑图阶段用第三人称跟随相机
	FVector DemoFollowLoc;
	FRotator DemoFollowRot;
	float AutoDemoRunT = 0.0f;
	TArray<FVector> AutoDemoWaypoints;
	void AutoDemoTick(float DeltaSeconds);
	void SaveViewportPNG(const FString& Path);

	// ---------- M3 子对象编辑状态 ----------
	int32 SubEditMode = 0;              // 0=物体 1=顶点 2=面
	UPROPERTY() ABlockoutActor* EditTarget = nullptr;
	TArray<int32> SelectedVerts;
	TArray<int32> SelectedFaces;

	// ---------- P4 测试模式状态 ----------
	bool bTestMode = false;
	UPROPERTY() class ABlockoutTestPawn* TestPawn = nullptr;
	UPROPERTY() APawn* SavedPawn = nullptr;
	FVector SavedCamLoc = FVector::ZeroVector;
	FRotator SavedCamRot = FRotator::ZeroRotator;
	// 测试模式按键
	void OnTestJump();
	void OnTestToggleRun();
	void OnTestCrouch();
	void OnTestToggleView();
	void OnTestPresetCS();
	void OnTestPresetSouls();
	void OnTestPresetUncharted();
	void OnTestRoll();
	void BindEditorKeyBindings();
	void BindTestKeyBindings();
};
