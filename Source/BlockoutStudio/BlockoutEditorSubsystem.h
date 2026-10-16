// BlockoutStudio - 编辑器核心子系统：物体管理、选择、变换、吸附、撤销、自动保存
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BlockoutTypes.h"
#include "BlockoutEditorSubsystem.generated.h"

class ABlockoutActor;
class UWorld;

UCLASS()
class BLOCKOUTSTUDIO_API UBlockoutEditorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---------- 生命周期 ----------
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 每帧驱动（由 HUD tick 调用）
	void Update(float DeltaSeconds);

	// ---------- 物体管理 ----------
	ABlockoutActor* SpawnBlock(EBlockoutShape Shape, const FVector& Location, const FBlockoutShapeParams& Params = FBlockoutShapeParams());
	void DeleteSelected();
	ABlockoutActor* DuplicateActor(ABlockoutActor* Src, const FVector& Offset);
	void DuplicateSelected();                         // E7 Ctrl+D
	void DuplicateDragged(const FVector& Offset);     // E7 Ctrl+拖拽复制
	void MakeArray(int32 Count, const FVector& Spacing, bool bPolar, float AngleDeg); // E7 阵列
	void MirrorSelected(bool bX, bool bY, bool bZ);   // E7 镜像
	void AlignSelected(int32 Mode);                   // E7 对齐：0左 1右 2前 3后 4顶 5底 6水平居中 7垂直居中

	// ---------- 选择 ----------
	void SelectActor(ABlockoutActor* Actor, bool bAdditive);
	void SelectInBox(const FVector2D& ScreenA, const FVector2D& ScreenB, bool bAdditive); // E1 框选
	void SelectAll();
	void ClearSelection();
	const TArray<ABlockoutActor*>& GetSelection() const { return Selection; }
	bool IsSelected(ABlockoutActor* Actor) const;

	// ---------- 变换（E2/E3/E11） ----------
	void SetTransformMode(EBlockoutTransformMode Mode) { TransformMode = Mode; }
	EBlockoutTransformMode GetTransformMode() const { return TransformMode; }
	void ToggleCoordSpace();                          // E3 世界/本地切换
	EBlockoutCoordSpace GetCoordSpace() const { return CoordSpace; }
	void SetPivotMode(EBlockoutPivotMode Mode);       // E4
	void NudgeSelection(const FVector& WorldDelta);   // 数值/键盘微移
	void RotateSelection(float Pitch, float Yaw, float Roll);
	void ScaleSelection(const FVector& Scale);
	void SetSelectionTransform(const FTransform& NewTransform);

	// gizmo 拖拽（由输入层调用）
	void BeginGizmoDrag(const FVector& RayOrigin, const FVector& RayDir, int32 AxisIndex);
	void UpdateGizmoDrag(const FVector& RayOrigin, const FVector& RayDir);
	void EndGizmoDrag();

	// ---------- 吸附（E5/E6） ----------
	void SetGridSnap(bool bOn) { bGridSnap = bOn; }
	bool GetGridSnap() const { return bGridSnap; }
	void SetSnapStep(float Step) { SnapStep = Step; }
	float GetSnapStep() const { return SnapStep; }
	void SetSurfaceSnap(bool bOn) { bSurfaceSnap = bOn; }
	bool GetSurfaceSnap() const { return bSurfaceSnap; }
	FVector ApplySnapping(const FVector& InLocation, ABlockoutActor* Moving) const;

	// ---------- 物体属性（E9/E10/E11） ----------
	void RenameActor(ABlockoutActor* Actor, const FString& NewName);
	void SetHidden(ABlockoutActor* Actor, bool bHidden);
	void SetLocked(ABlockoutActor* Actor, bool bLocked);
	void GroupSelected();
	void UngroupSelected();
	void SetColor(const FLinearColor& Color);
	void SetSurfaceType(EBlockoutSurfaceType Type);
	void SetTexturePattern(EBlockoutTexturePattern Pattern, float Tiling);

	// ---------- 参数化 ----------
	void SetShapeParams(ABlockoutActor* Actor, const FBlockoutShapeParams& Params);

	// ---------- 撤销/重做（E8） ----------
	void PushUndo(const FString& Description); // 调用前记录快照
	bool Undo();
	bool Redo();

	// ---------- 存档（D1/D2） ----------
	bool SaveProject(const FString& FilePath);
	bool LoadProject(const FString& FilePath);
	bool AutoSave(bool bSnapshot);
	void RecoverIfNeeded();
	FString GetDefaultProjectPath() const;

	// ---------- 查询 ----------
	TArray<ABlockoutActor*> GetAllBlocks() const;
	ABlockoutActor* GetActorById(const FGuid& Id) const;
	UWorld* GetEditingWorld() const;

	// 状态
	FString CurrentProjectPath;
	bool bDirty = false;

private:
	UPROPERTY() TArray<ABlockoutActor*> Selection;
	UPROPERTY() TArray<ABlockoutActor*> AllBlocks;
	EBlockoutTransformMode TransformMode = EBlockoutTransformMode::Translate;
	EBlockoutCoordSpace CoordSpace = EBlockoutCoordSpace::World;
	EBlockoutPivotMode PivotMode = EBlockoutPivotMode::Center;
	bool bGridSnap = true;
	bool bSurfaceSnap = true;
	float SnapStep = 10.f;

	// undo
	TArray<FBlockoutUndoOp> UndoStack;
	TArray<FBlockoutUndoOp> RedoStack;
	TArray<FBlockoutObjectData> SnapshotAll() const;
	void RestoreAll(const TArray<FBlockoutObjectData>& Datas);
	bool bSnapshotValid = false;
	TArray<FBlockoutObjectData> PendingBefore;

	// gizmo 拖拽状态
	bool bDragging = false;
	int32 DragAxis = 0;
	FVector DragStartLocation = FVector::ZeroVector;
	FVector DragPlaneOrigin = FVector::ZeroVector;
	FVector DragPlaneNormal = FVector::ForwardVector;
	FVector DragStartHitPoint = FVector::ZeroVector;
	TArray<FTransform> DragStartTransforms;

	// 自动保存
	float AutoSaveTimer = 0.f;
	static constexpr float AutoSaveInterval = 180.f;

	void NotifyDirty();
};
