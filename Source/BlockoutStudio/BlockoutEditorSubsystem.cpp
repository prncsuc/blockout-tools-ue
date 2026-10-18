// BlockoutStudio - 编辑器核心子系统实现
#include "BlockoutEditorSubsystem.h"
#include "BlockoutActor.h"
#include "BlockoutMeshGen.h"
#include "IndexTypes.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"

void UBlockoutEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RecoverIfNeeded();
}

void UBlockoutEditorSubsystem::Deinitialize()
{
	AutoSave(true);
	Super::Deinitialize();
}

void UBlockoutEditorSubsystem::Update(float DeltaSeconds)
{
	AutoSaveTimer += DeltaSeconds;
	if (AutoSaveTimer >= AutoSaveInterval)
	{
		AutoSaveTimer = 0.f;
		if (bDirty) AutoSave(false);
	}
}

UWorld* UBlockoutEditorSubsystem::GetEditingWorld() const
{
	return GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
}

FString UBlockoutEditorSubsystem::GetDefaultProjectPath() const
{
	if (!CurrentProjectPath.IsEmpty()) return CurrentProjectPath;
	return FPaths::ProjectSavedDir() / TEXT("BlockoutProjects/blockout_default.blockout.json");
}

// ---------------- 物体管理 ----------------

ABlockoutActor* UBlockoutEditorSubsystem::SpawnBlock(EBlockoutShape Shape, const FVector& Location, const FBlockoutShapeParams& Params)
{
	UWorld* World = GetEditingWorld();
	if (!World) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABlockoutActor* Actor = World->SpawnActor<ABlockoutActor>(Location, FRotator::ZeroRotator, P);
	if (!Actor) return nullptr;
	Actor->Data.Id = FGuid::NewGuid();
	Actor->Data.Shape = Shape;
	Actor->Data.Params = Params;
	Actor->Data.Transform = FTransform(FRotator::ZeroRotator, Location);
	Actor->Data.Name = UEnum::GetValueAsString(Shape).Replace(TEXT("EBlockoutShape::"), TEXT(""));
	Actor->Rebuild();
	AllBlocks.Add(Actor);
	NotifyDirty();
	return Actor;
}

void UBlockoutEditorSubsystem::DeleteSelected()
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("删除"));
	for (ABlockoutActor* Actor : Selection)
	{
		if (!Actor || Actor->Data.bLocked) continue;
		AllBlocks.Remove(Actor);
		Actor->Destroy();
	}
	Selection.Empty();
	NotifyDirty();
}

ABlockoutActor* UBlockoutEditorSubsystem::DuplicateActor(ABlockoutActor* Src, const FVector& Offset)
{
	if (!Src) return nullptr;
	UWorld* World = GetEditingWorld();
	if (!World) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABlockoutActor* NewActor = World->SpawnActor<ABlockoutActor>(Src->GetActorLocation() + Offset, FRotator::ZeroRotator, P);
	if (!NewActor) return nullptr;
	FBlockoutObjectData D = Src->Data;
	D.Id = FGuid::NewGuid();
	D.Name = Src->Data.Name + TEXT("_copy");
	D.Transform.SetLocation(D.Transform.GetLocation() + Offset);
	NewActor->RestoreData(D);
	AllBlocks.Add(NewActor);
	return NewActor;
}

void UBlockoutEditorSubsystem::DuplicateSelected()
{
	PushUndo(TEXT("复制"));
	TArray<ABlockoutActor*> NewSel;
	for (ABlockoutActor* Src : Selection)
	{
		if (ABlockoutActor* NewActor = DuplicateActor(Src, FVector::ZeroVector))
		{
			NewSel.Add(NewActor);
		}
	}
	Selection = NewSel;
	NotifyDirty();
}

void UBlockoutEditorSubsystem::DuplicateDragged(const FVector& Offset)
{
	PushUndo(TEXT("Ctrl+拖拽复制"));
	TArray<ABlockoutActor*> NewSel;
	for (ABlockoutActor* Src : Selection)
	{
		if (ABlockoutActor* NewActor = DuplicateActor(Src, Offset))
		{
			NewSel.Add(NewActor);
		}
	}
	Selection = NewSel;
	NotifyDirty();
}

void UBlockoutEditorSubsystem::MakeArray(int32 Count, const FVector& Spacing, bool bPolar, float AngleDeg)
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("阵列"));
	TArray<ABlockoutActor*> NewSel;
	for (ABlockoutActor* Src : Selection)
	{
		FVector Center = Src->GetActorLocation();
		for (int32 i = 1; i < Count; i++)
		{
			FVector Offset;
			FRotator Rot = FRotator::ZeroRotator;
			if (bPolar)
			{
				Rot = FRotator(0, AngleDeg * i, 0);
				Offset = Rot.RotateVector(Center) - Center;
			}
			else
			{
				Offset = Spacing * i;
			}
			if (ABlockoutActor* NewActor = DuplicateActor(Src, Offset))
			{
				NewActor->Data.Transform.SetRotation(FQuat(Rot) * NewActor->Data.Transform.GetRotation());
				NewActor->SyncTransform();
				NewSel.Add(NewActor);
			}
		}
	}
	Selection.Append(NewSel);
	NotifyDirty();
}

void UBlockoutEditorSubsystem::MirrorSelected(bool bX, bool bY, bool bZ)
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("镜像"));
	FVector Center = Selection[0]->GetActorLocation();
	for (ABlockoutActor* Actor : Selection)
	{
		FTransform T = Actor->Data.Transform;
		FVector L = T.GetLocation();
		if (bX) L.X = 2 * Center.X - L.X;
		if (bY) L.Y = 2 * Center.Y - L.Y;
		if (bZ) L.Z = 2 * Center.Z - L.Z;
		T.SetLocation(L);
		Actor->Data.Transform = T;
		Actor->SyncTransform();
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::AlignSelected(int32 Mode)
{
	if (Selection.Num() < 2) return;
	PushUndo(TEXT("对齐"));
	FBox Total(ForceInit);
	for (ABlockoutActor* Actor : Selection)
	{
		Total += Actor->GetComponentsBoundingBox(true);
	}
	FVector Target = Total.GetCenter();
	for (ABlockoutActor* Actor : Selection)
	{
		FBox B = Actor->GetComponentsBoundingBox(true);
		FVector L = Actor->Data.Transform.GetLocation();
		switch (Mode)
		{
		case 0: L.X += Total.Min.X - B.Min.X; break;
		case 1: L.X += Total.Max.X - B.Max.X; break;
		case 2: L.Y += Total.Min.Y - B.Min.Y; break;
		case 3: L.Y += Total.Max.Y - B.Max.Y; break;
		case 4: L.Z += Total.Max.Z - B.Max.Z; break;
		case 5: L.Z += Total.Min.Z - B.Min.Z; break;
		case 6: L.X += Target.X - B.GetCenter().X; break;
		case 7: L.Z += Target.Z - B.GetCenter().Z; break;
		default: break;
		}
		Actor->Data.Transform.SetLocation(L);
		Actor->SyncTransform();
	}
	NotifyDirty();
}

// ---------------- 选择 ----------------

void UBlockoutEditorSubsystem::SelectActor(ABlockoutActor* Actor, bool bAdditive)
{
	if (!bAdditive)
	{
		ClearSelection();
	}
	if (Actor && !Selection.Contains(Actor))
	{
		Selection.Add(Actor);
		Actor->SetHighlighted(true);
	}
}

void UBlockoutEditorSubsystem::SelectInBox(const FVector2D& ScreenA, const FVector2D& ScreenB, bool bAdditive)
{
	if (!bAdditive) ClearSelection();
	UWorld* World = GetEditingWorld();
	if (!World) return;
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;
	FVector2D Min(FMath::Min(ScreenA.X, ScreenB.X), FMath::Min(ScreenA.Y, ScreenB.Y));
	FVector2D Max(FMath::Max(ScreenA.X, ScreenB.X), FMath::Max(ScreenA.Y, ScreenB.Y));
	for (ABlockoutActor* Actor : AllBlocks)
	{
		if (!Actor || Actor->Data.bHidden || Actor->Data.bLocked) continue;
		FVector2D Screen;
		if (PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), Screen, true))
		{
			if (Screen.X >= Min.X && Screen.X <= Max.X && Screen.Y >= Min.Y && Screen.Y <= Max.Y)
			{
				SelectActor(Actor, true);
			}
		}
	}
}

void UBlockoutEditorSubsystem::SelectAll()
{
	for (ABlockoutActor* Actor : AllBlocks)
	{
		SelectActor(Actor, true);
	}
}

void UBlockoutEditorSubsystem::ClearSelection()
{
	for (ABlockoutActor* Actor : Selection)
	{
		if (Actor) Actor->SetHighlighted(false);
	}
	Selection.Empty();
}

bool UBlockoutEditorSubsystem::IsSelected(ABlockoutActor* Actor) const
{
	return Selection.Contains(Actor);
}

// ---------------- 变换 ----------------

void UBlockoutEditorSubsystem::ToggleCoordSpace()
{
	CoordSpace = (CoordSpace == EBlockoutCoordSpace::World) ? EBlockoutCoordSpace::Local : EBlockoutCoordSpace::World;
}

void UBlockoutEditorSubsystem::SetPivotMode(EBlockoutPivotMode Mode)
{
	PivotMode = Mode;
}

void UBlockoutEditorSubsystem::NudgeSelection(const FVector& WorldDelta)
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("移动"));
	for (ABlockoutActor* Actor : Selection)
	{
		FTransform T = Actor->Data.Transform;
		FVector Delta = (CoordSpace == EBlockoutCoordSpace::Local) ? T.GetRotation().RotateVector(WorldDelta) : WorldDelta;
		T.SetLocation(T.GetLocation() + Delta);
		Actor->Data.Transform = T;
		Actor->SyncTransform();
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::RotateSelection(float Pitch, float Yaw, float Roll)
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("旋转"));
	for (ABlockoutActor* Actor : Selection)
	{
		FTransform T = Actor->Data.Transform;
		T.SetRotation(FQuat(FRotator(Pitch, Yaw, Roll)) * T.GetRotation());
		Actor->Data.Transform = T;
		Actor->SyncTransform();
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::ScaleSelection(const FVector& Scale)
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("缩放"));
	for (ABlockoutActor* Actor : Selection)
	{
		FTransform T = Actor->Data.Transform;
		T.SetScale3D(T.GetScale3D() * Scale);
		Actor->Data.Transform = T;
		Actor->SyncTransform();
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::SetSelectionTransform(const FTransform& NewTransform)
{
	if (Selection.Num() == 0) return;
	for (ABlockoutActor* Actor : Selection)
	{
		Actor->Data.Transform = NewTransform;
		Actor->SyncTransform();
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::BeginGizmoDrag(const FVector& RayOrigin, const FVector& RayDir, int32 AxisIndex)
{
	if (Selection.Num() == 0) return;
	bDragging = true;
	DragAxis = AxisIndex;
	DragStartTransforms.Empty();
	for (ABlockoutActor* Actor : Selection)
	{
		DragStartTransforms.Add(Actor->Data.Transform);
	}
	FVector Axis = (AxisIndex == 0) ? FVector::RightVector : (AxisIndex == 1 ? FVector::UpVector : FVector::ForwardVector);
	// 拖拽平面：包含拖拽轴，法线垂直于轴与视线
	FVector PlaneN = FVector::CrossProduct(FVector::CrossProduct(RayDir.GetSafeNormal(), Axis), Axis);
	if (PlaneN.IsNearlyZero()) PlaneN = RayDir.GetSafeNormal();
	DragPlaneNormal = PlaneN.GetSafeNormal();
	DragPlaneOrigin = Selection[0]->GetActorLocation();
	FPlane Plane(DragPlaneOrigin, DragPlaneNormal);
	DragStartHitPoint = FMath::LinePlaneIntersection(RayOrigin, RayOrigin + RayDir * 1e6f, Plane);
	DragStartLocation = Selection[0]->GetActorLocation();
}

void UBlockoutEditorSubsystem::UpdateGizmoDrag(const FVector& RayOrigin, const FVector& RayDir)
{
	if (!bDragging) return;
	FPlane Plane(DragPlaneOrigin, DragPlaneNormal);
	float T = 0.f;
	FVector Hit = FMath::LinePlaneIntersection(RayOrigin, RayOrigin + RayDir * 1e6f, Plane);
	FVector Delta = Hit - DragStartHitPoint;
	FVector Axis = (DragAxis == 0) ? FVector::RightVector : (DragAxis == 1 ? FVector::UpVector : FVector::ForwardVector);
	float Amount = FVector::DotProduct(Delta, Axis);
	if (bGridSnap) Amount = FMath::GridSnap(Amount, SnapStep);
	FVector Move = Axis * Amount;

	for (int32 i = 0; i < Selection.Num() && i < DragStartTransforms.Num(); i++)
	{
		FTransform T0 = DragStartTransforms[i];
		FTransform T1 = T0;
		if (TransformMode == EBlockoutTransformMode::Translate)
		{
			T1.SetLocation(T0.GetLocation() + Move);
			T1.SetLocation(ApplySnapping(T1.GetLocation(), Selection[i]));
		}
		else if (TransformMode == EBlockoutTransformMode::Rotate)
		{
			float Angle = (Amount / 100.f) * 90.f;
			T1.SetRotation(FQuat(FRotator(
				DragAxis == 1 ? Angle : 0,
				DragAxis == 2 ? Angle : 0,
				DragAxis == 0 ? Angle : 0)) * T0.GetRotation());
		}
		else
		{
			FVector S = T0.GetScale3D();
			float ScaleDelta = 1.f + Amount / 100.f;
			if (DragAxis == 0) S.X *= ScaleDelta;
			else if (DragAxis == 1) S.Z *= ScaleDelta;
			else S.Y *= ScaleDelta;
			T1.SetScale3D(S);
		}
		Selection[i]->Data.Transform = T1;
		Selection[i]->SyncTransform();
	}
	bDirty = true;
}

void UBlockoutEditorSubsystem::EndGizmoDrag()
{
	if (bDragging)
	{
		PushUndo(TEXT("Gizmo拖拽"));
		NotifyDirty();
	}
	bDragging = false;
}

// ---------------- 吸附 ----------------

FVector UBlockoutEditorSubsystem::ApplySnapping(const FVector& InLocation, ABlockoutActor* Moving) const
{
	FVector Result = InLocation;
	if (bGridSnap)
	{
		Result.X = FMath::GridSnap(Result.X, SnapStep);
		Result.Y = FMath::GridSnap(Result.Y, SnapStep);
		Result.Z = FMath::GridSnap(Result.Z, SnapStep);
	}
	// 组件面吸附（E6）：与其它体块包围盒面贴合
	if (bSurfaceSnap && Moving)
	{
		FBox MB = Moving->GetComponentsBoundingBox(true);
		FVector Ext = MB.GetSize() * 0.5;
		const float SnapDist = 5.f;
		for (ABlockoutActor* Other : AllBlocks)
		{
			if (!Other || Other == Moving || Other->Data.bHidden) continue;
			FBox OB = Other->GetComponentsBoundingBox(true);
			for (int32 AxisIdx = 0; AxisIdx < 3; AxisIdx++)
			{
				double& C = Result[AxisIdx];
				double MovingMin = C - Ext[AxisIdx], MovingMax = C + Ext[AxisIdx];
				// 目标 Min 贴到本体 Max 或 目标 Max 贴到本体 Min
				if (FMath::Abs(MovingMin - OB.Max[AxisIdx]) < SnapDist) C += OB.Max[AxisIdx] - MovingMin;
				else if (FMath::Abs(MovingMax - OB.Min[AxisIdx]) < SnapDist) C += OB.Min[AxisIdx] - MovingMax;
			}
		}
	}
	return Result;
}

// ---------------- 属性 ----------------

void UBlockoutEditorSubsystem::RenameActor(ABlockoutActor* Actor, const FString& NewName)
{
	if (!Actor) return;
	PushUndo(TEXT("重命名"));
	Actor->Data.Name = NewName;
	NotifyDirty();
}

void UBlockoutEditorSubsystem::ReorderBlock(ABlockoutActor* Actor, ABlockoutActor* TargetBefore)
{
	if (!Actor || Actor == TargetBefore || !AllBlocks.Contains(Actor)) return;
	PushUndo(TEXT("场景树排序"));
	AllBlocks.Remove(Actor);
	if (TargetBefore && AllBlocks.Contains(TargetBefore))
	{
		AllBlocks.Insert(Actor, AllBlocks.IndexOfByKey(TargetBefore));
	}
	else
	{
		AllBlocks.Add(Actor);
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::AssignGroupToActor(ABlockoutActor* Actor, const FGuid& GroupId)
{
	if (!Actor) return;
	PushUndo(TEXT("拖拽编组"));
	Actor->Data.GroupId = GroupId;
	NotifyDirty();
}

void UBlockoutEditorSubsystem::SetHidden(ABlockoutActor* Actor, bool bHidden)
{
	if (!Actor) return;
	PushUndo(bHidden ? TEXT("隐藏") : TEXT("取消隐藏"));
	Actor->Data.bHidden = bHidden;
	Actor->ApplyAppearance();
	NotifyDirty();
}

void UBlockoutEditorSubsystem::SetLocked(ABlockoutActor* Actor, bool bLocked)
{
	if (!Actor) return;
	PushUndo(bLocked ? TEXT("锁定") : TEXT("解锁"));
	Actor->Data.bLocked = bLocked;
	Actor->ApplyAppearance();
	NotifyDirty();
}

void UBlockoutEditorSubsystem::GroupSelected()
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("编组"));
	FGuid GroupId = FGuid::NewGuid();
	for (ABlockoutActor* Actor : Selection)
	{
		Actor->Data.GroupId = GroupId;
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::UngroupSelected()
{
	PushUndo(TEXT("解组"));
	for (ABlockoutActor* Actor : Selection)
	{
		Actor->Data.GroupId.Invalidate();
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::SetColor(const FLinearColor& Color)
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("改色"));
	for (ABlockoutActor* Actor : Selection)
	{
		Actor->Data.Color = Color;
		Actor->ApplyAppearance();
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::SetSurfaceType(EBlockoutSurfaceType Type)
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("表面类型"));
	for (ABlockoutActor* Actor : Selection)
	{
		Actor->Data.SurfaceType = Type;
		// 表面类型映射默认颜色
		switch (Type)
		{
		case EBlockoutSurfaceType::Solid: Actor->Data.Color = FLinearColor(0.6f, 0.6f, 0.6f, 1.f); break;
		case EBlockoutSurfaceType::Glass: Actor->Data.Color = FLinearColor(0.6f, 0.8f, 0.9f, 0.35f); break;
		case EBlockoutSurfaceType::Metal: Actor->Data.Color = FLinearColor(0.55f, 0.6f, 0.65f, 1.f); break;
		case EBlockoutSurfaceType::Wood: Actor->Data.Color = FLinearColor(0.55f, 0.4f, 0.25f, 1.f); break;
		case EBlockoutSurfaceType::Concrete: Actor->Data.Color = FLinearColor(0.65f, 0.64f, 0.6f, 1.f); break;
		case EBlockoutSurfaceType::Grass: Actor->Data.Color = FLinearColor(0.35f, 0.55f, 0.3f, 1.f); break;
		case EBlockoutSurfaceType::Dirt: Actor->Data.Color = FLinearColor(0.45f, 0.35f, 0.22f, 1.f); break;
		case EBlockoutSurfaceType::Water: Actor->Data.Color = FLinearColor(0.25f, 0.45f, 0.7f, 0.7f); break;
		default: break;
		}
		Actor->ApplyAppearance();
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::SetTexturePattern(EBlockoutTexturePattern Pattern, float Tiling)
{
	if (Selection.Num() == 0) return;
	PushUndo(TEXT("纹理"));
	for (ABlockoutActor* Actor : Selection)
	{
		Actor->Data.TexturePattern = Pattern;
		Actor->Data.Tiling = Tiling;
		Actor->ApplyAppearance();
	}
	NotifyDirty();
}

void UBlockoutEditorSubsystem::SetShapeParams(ABlockoutActor* Actor, const FBlockoutShapeParams& Params)
{
	if (!Actor) return;
	PushUndo(TEXT("参数化调整"));
	Actor->Data.Params = Params;
	Actor->Rebuild();
	NotifyDirty();
}

// ---------------- 撤销/重做 ----------------

TArray<FBlockoutObjectData> UBlockoutEditorSubsystem::SnapshotAll() const
{
	TArray<FBlockoutObjectData> Result;
	for (ABlockoutActor* Actor : AllBlocks)
	{
		if (Actor) Result.Add(Actor->Data);
	}
	return Result;
}

void UBlockoutEditorSubsystem::RestoreAll(const TArray<FBlockoutObjectData>& Datas)
{
	UWorld* World = GetEditingWorld();
	if (!World) return;
	for (ABlockoutActor* Actor : AllBlocks)
	{
		if (Actor) Actor->Destroy();
	}
	AllBlocks.Empty();
	Selection.Empty();
	for (const FBlockoutObjectData& D : Datas)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ABlockoutActor* Actor = World->SpawnActor<ABlockoutActor>(D.Transform.GetLocation(), FRotator::ZeroRotator, P);
		if (Actor)
		{
			Actor->RestoreData(D);
			AllBlocks.Add(Actor);
		}
	}
}

void UBlockoutEditorSubsystem::PushUndo(const FString& Description)
{
	FBlockoutUndoOp Op;
	Op.OpType = Description;
	Op.Timestamp = FDateTime::Now();
	Op.Before = SnapshotAll();
	UndoStack.Add(MoveTemp(Op));
	if (UndoStack.Num() > 150)
	{
		UndoStack.RemoveAt(0, UndoStack.Num() - 150);
	}
	RedoStack.Empty();
}

bool UBlockoutEditorSubsystem::Undo()
{
	if (UndoStack.Num() == 0) return false;
	FBlockoutUndoOp Op = UndoStack.Pop();
	Op.After = SnapshotAll();
	RestoreAll(Op.Before);
	RedoStack.Add(Op);
	bDirty = true;
	return true;
}

bool UBlockoutEditorSubsystem::Redo()
{
	if (RedoStack.Num() == 0) return false;
	FBlockoutUndoOp Op = RedoStack.Pop();
	RestoreAll(Op.After);
	UndoStack.Add(Op);
	bDirty = true;
	return true;
}

void UBlockoutEditorSubsystem::NotifyDirty()
{
	bDirty = true;
}

// ---------------- 存档 ----------------

bool UBlockoutEditorSubsystem::SaveProject(const FString& FilePath)
{
	TArray<FBlockoutObjectData> Datas = SnapshotAll();
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("version"), TEXT("1.0"));
	Root->SetStringField(TEXT("savedAt"), FDateTime::Now().ToIso8601());
	TArray<TSharedPtr<FJsonValue>> Blocks;
	for (const FBlockoutObjectData& D : Datas)
	{
		FString BlockJson;
		if (FJsonObjectConverter::UStructToJsonObjectString(D, BlockJson))
		{
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BlockJson);
			TSharedPtr<FJsonObject> Obj;
			if (FJsonSerializer::Deserialize(Reader, Obj) && Obj.IsValid())
			{
				Blocks.Add(MakeShared<FJsonValueObject>(Obj));
			}
		}
	}
	Root->SetArrayField(TEXT("blocks"), Blocks);
	FString Out;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	// 原子写入：先写 .tmp 再移动
	FString TmpPath = FilePath + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Out, *TmpPath))
	{
		return false;
	}
	if (IFileManager::Get().FileExists(*FilePath))
	{
		IFileManager::Get().Delete(*FilePath, false, true, true);
	}
	if (!IFileManager::Get().Move(*FilePath, *TmpPath))
	{
		return false;
	}
	CurrentProjectPath = FilePath;
	bDirty = false;
	return true;
}

bool UBlockoutEditorSubsystem::LoadProject(const FString& FilePath)
{
	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *FilePath)) return false;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;
	const TArray<TSharedPtr<FJsonValue>>* Blocks;
	if (!Root->TryGetArrayField(TEXT("blocks"), Blocks)) return false;
	TArray<FBlockoutObjectData> Datas;
	for (const TSharedPtr<FJsonValue>& Val : *Blocks)
	{
		TSharedPtr<FJsonObject> Obj = Val->AsObject();
		if (!Obj.IsValid()) continue;
		// 手动逐字段解析：对数值/枚举/结构体格式差异更稳健（JsonObjectToUStruct 对生成器脚本产出的 JSON 会整体失败）
		FBlockoutObjectData D;
		FString IdStr, ShapeStr, SurfStr, PatStr;
		if (Obj->TryGetStringField(TEXT("Id"), IdStr)) FGuid::Parse(IdStr, D.Id);
		Obj->TryGetStringField(TEXT("Name"), D.Name);
		if (Obj->TryGetStringField(TEXT("Shape"), ShapeStr))
		{
			FString S = ShapeStr;
			int32 Colon = S.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (Colon != INDEX_NONE) S = S.Mid(Colon + 2);
			{ int64 V = StaticEnum<EBlockoutShape>()->GetValueByNameString(S); if (V != INDEX_NONE) D.Shape = (EBlockoutShape)V; }
		}
		if (Obj->TryGetStringField(TEXT("SurfaceType"), SurfStr))
		{
			FString S = SurfStr;
			int32 Colon = S.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (Colon != INDEX_NONE) S = S.Mid(Colon + 2);
			{ int64 V = StaticEnum<EBlockoutSurfaceType>()->GetValueByNameString(S); if (V != INDEX_NONE) D.SurfaceType = (EBlockoutSurfaceType)V; }
		}
		if (Obj->TryGetStringField(TEXT("TexturePattern"), PatStr))
		{
			FString S = PatStr;
			int32 Colon = S.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (Colon != INDEX_NONE) S = S.Mid(Colon + 2);
			{ int64 V = StaticEnum<EBlockoutTexturePattern>()->GetValueByNameString(S); if (V != INDEX_NONE) D.TexturePattern = (EBlockoutTexturePattern)V; }
		}
		const TSharedPtr<FJsonObject>* PObj = nullptr;
		if (Obj->TryGetObjectField(TEXT("Params"), PObj) && PObj && PObj->IsValid())
		{
			const TSharedPtr<FJsonObject>& P = *PObj;
			double Num = 0; int32 Int = 0; bool Bool = false;
			if (P->TryGetNumberField(TEXT("Length"), Num)) D.Params.Length = (float)Num;
			if (P->TryGetNumberField(TEXT("Width"), Num)) D.Params.Width = (float)Num;
			if (P->TryGetNumberField(TEXT("Height"), Num)) D.Params.Height = (float)Num;
			if (P->TryGetNumberField(TEXT("Thickness"), Num)) D.Params.Thickness = (float)Num;
			if (P->TryGetNumberField(TEXT("AngleDeg"), Num)) D.Params.AngleDeg = (float)Num;
			if (P->TryGetNumberField(TEXT("Steps"), Int)) D.Params.Steps = Int;
			if (P->TryGetNumberField(TEXT("Segments"), Int)) D.Params.Segments = Int;
			if (P->TryGetNumberField(TEXT("HoleWidth"), Num)) D.Params.HoleWidth = (float)Num;
			if (P->TryGetNumberField(TEXT("HoleHeight"), Num)) D.Params.HoleHeight = (float)Num;
			if (P->TryGetNumberField(TEXT("HoleOffsetZ"), Num)) D.Params.HoleOffsetZ = (float)Num;
			if (P->TryGetNumberField(TEXT("ArrayCount"), Int)) D.Params.ArrayCount = Int;
			if (P->TryGetNumberField(TEXT("ArraySpacing"), Num)) D.Params.ArraySpacing = (float)Num;
			if (P->TryGetBoolField(TEXT("bThroughHole"), Bool)) D.Params.bThroughHole = Bool;
		}
		const TSharedPtr<FJsonObject>* TObj = nullptr;
		if (Obj->TryGetObjectField(TEXT("Transform"), TObj) && TObj && TObj->IsValid())
		{
			const TSharedPtr<FJsonObject>& T = *TObj;
			const TSharedPtr<FJsonObject>* V = nullptr;
			FVector Loc(0, 0, 0), Sc(1, 1, 1); FQuat Q = FQuat::Identity;
			if (T->TryGetObjectField(TEXT("Translation"), V) && V && V->IsValid())
			{
				double X, Y, Z;
				if ((*V)->TryGetNumberField(TEXT("X"), X) && (*V)->TryGetNumberField(TEXT("Y"), Y) && (*V)->TryGetNumberField(TEXT("Z"), Z))
					Loc = FVector(X, Y, Z);
			}
			if (T->TryGetObjectField(TEXT("Rotation"), V) && V && V->IsValid())
			{
				double X, Y, Z, W = 1.0;
				if ((*V)->TryGetNumberField(TEXT("X"), X) && (*V)->TryGetNumberField(TEXT("Y"), Y) && (*V)->TryGetNumberField(TEXT("Z"), Z))
				{
					(*V)->TryGetNumberField(TEXT("W"), W);
					Q = FQuat(X, Y, Z, W);
				}
			}
			if (T->TryGetObjectField(TEXT("Scale3D"), V) && V && V->IsValid())
			{
				double X, Y, Z;
				if ((*V)->TryGetNumberField(TEXT("X"), X) && (*V)->TryGetNumberField(TEXT("Y"), Y) && (*V)->TryGetNumberField(TEXT("Z"), Z))
					Sc = FVector(X, Y, Z);
			}
			D.Transform = FTransform(Q, Loc, Sc);
		}
		const TSharedPtr<FJsonObject>* CObj = nullptr;
		if (Obj->TryGetObjectField(TEXT("Color"), CObj) && CObj && CObj->IsValid())
		{
			double R = 0.6, G = 0.6, B = 0.6, A = 1.0;
			(*CObj)->TryGetNumberField(TEXT("R"), R);
			(*CObj)->TryGetNumberField(TEXT("G"), G);
			(*CObj)->TryGetNumberField(TEXT("B"), B);
			(*CObj)->TryGetNumberField(TEXT("A"), A);
			D.Color = FLinearColor((float)R, (float)G, (float)B, (float)A);
		}
		double Tiling = 1.0;
		if (Obj->TryGetNumberField(TEXT("Tiling"), Tiling)) D.Tiling = (float)Tiling;
		bool BFlag = false;
		if (Obj->TryGetBoolField(TEXT("bHidden"), BFlag)) D.bHidden = BFlag;
		if (Obj->TryGetBoolField(TEXT("bLocked"), BFlag)) D.bLocked = BFlag;
		if (Obj->TryGetBoolField(TEXT("bFreeform"), BFlag)) D.bFreeform = BFlag;
		FString GrpStr;
		if (Obj->TryGetStringField(TEXT("GroupId"), GrpStr)) FGuid::Parse(GrpStr, D.GroupId);
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Obj->TryGetArrayField(TEXT("Vertices"), Arr) && Arr)
		{
			for (const TSharedPtr<FJsonValue>& Vv : *Arr)
			{
				const TSharedPtr<FJsonObject>* Vo = nullptr;
				if (Vv->TryGetObject(Vo) && Vo && Vo->IsValid())
				{
					double X, Y, Z;
					if ((*Vo)->TryGetNumberField(TEXT("X"), X) && (*Vo)->TryGetNumberField(TEXT("Y"), Y) && (*Vo)->TryGetNumberField(TEXT("Z"), Z))
						D.Vertices.Add(FVector(X, Y, Z));
				}
			}
		}
		if (Obj->TryGetArrayField(TEXT("Triangles"), Arr) && Arr)
		{
			for (const TSharedPtr<FJsonValue>& Tv : *Arr)
			{
				double Ix = 0;
				if (Tv->TryGetNumber(Ix)) D.Triangles.Add((int32)Ix);
			}
		}
		Datas.Add(D);
	}
	LastLoadParsed = Datas.Num();
	RestoreAll(Datas);
	LastLoadSpawned = AllBlocks.Num();
	CurrentProjectPath = FilePath;
	bDirty = false;
	return true;
}

bool UBlockoutEditorSubsystem::AutoSave(bool bSnapshot)
{
	FString Dir = FPaths::ProjectSavedDir() / TEXT("BlockoutSnapshots");
	IFileManager::Get().MakeDirectory(*Dir, true);
	FString Path;
	if (bSnapshot)
	{
		Path = Dir / FString::Printf(TEXT("snapshot_%s.blockout.json"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	}
	else
	{
		Path = Dir / TEXT("autosave.blockout.json");
	}
	const bool bOk = SaveProject(Path);
	if (bOk && bSnapshot)
	{
		PruneSnapshots(20);
	}
	return bOk;
}

// D1：快照滚动保留最近 MaxKeep 个（按文件名时间戳排序，删旧保新）
void UBlockoutEditorSubsystem::PruneSnapshots(int32 MaxKeep)
{
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("BlockoutSnapshots");
	TArray<FString> Files = GetSnapshotFiles();
	// GetSnapshotFiles 按新→旧排序，超出部分删除
	for (int32 i = MaxKeep; i < Files.Num(); ++i)
	{
		IFileManager::Get().Delete(*Files[i], false, true, true);
	}
}

// D1：快照列表（新→旧）
TArray<FString> UBlockoutEditorSubsystem::GetSnapshotFiles() const
{
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("BlockoutSnapshots");
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Dir / TEXT("snapshot_*.blockout.json")), true, false);
	// 文件名含 YYYYMMDD_HHMMSS，字典序即时间序
	Files.Sort([](const FString& A, const FString& B) { return A > B; });
	TArray<FString> Full;
	for (const FString& F : Files) Full.Add(Dir / F);
	return Full;
}

// D1：回滚到指定快照
bool UBlockoutEditorSubsystem::RestoreSnapshot(const FString& Path)
{
	return IFileManager::Get().FileExists(*Path) && LoadProject(Path);
}

// ---------------- T3 比例尺参考体 ----------------
namespace
{
	// 向自由网格追加一个轴对齐盒子
	void AppendBox(TArray<FVector>& V, TArray<int32>& T, const FVector& Center, const FVector& Half)
	{
		const int32 B = V.Num();
		for (int32 dz = -1; dz <= 1; dz += 2)
		for (int32 dy = -1; dy <= 1; dy += 2)
		for (int32 dx = -1; dx <= 1; dx += 2)
		{
			V.Add(Center + FVector(Half.X * dx, Half.Y * dy, Half.Z * dz));
		}
		// 顶点序号：dx 最快，其次 dy，最后 dz（0..7）
		const int32 Faces[6][4] = {
			{0, 2, 3, 1}, // -Z
			{4, 5, 7, 6}, // +Z
			{0, 1, 5, 4}, // -Y
			{2, 6, 7, 3}, // +Y
			{0, 4, 6, 2}, // -X
			{1, 3, 7, 5}  // +X
		};
		for (const auto& F : Faces)
		{
			T.Add(B + F[0]); T.Add(B + F[1]); T.Add(B + F[2]);
			T.Add(B + F[0]); T.Add(B + F[2]); T.Add(B + F[3]);
		}
	}
}

ABlockoutActor* UBlockoutEditorSubsystem::SpawnScaleReference(int32 Type, const FVector& Location)
{
	TArray<FVector> V;
	TArray<int32> T;
	FString Name;
	if (Type == 0) // 1m 立方体：100uu
	{
		AppendBox(V, T, FVector(0, 0, 50), FVector(50, 50, 50));
		Name = TEXT("ScaleRef_1m");
	}
	else if (Type == 1) // 2m 立方体：200uu
	{
		AppendBox(V, T, FVector(0, 0, 100), FVector(100, 100, 100));
		Name = TEXT("ScaleRef_2m");
	}
	else // 2：1.8m 人形（头/躯干/双腿拼盒）
	{
		AppendBox(V, T, FVector(0, 0, 170), FVector(10, 10, 10));   // 头
		AppendBox(V, T, FVector(0, 0, 115), FVector(18, 10, 45));   // 躯干
		AppendBox(V, T, FVector(-10, 0, 37.5), FVector(8, 8, 37.5)); // 左腿
		AppendBox(V, T, FVector(10, 0, 37.5), FVector(8, 8, 37.5));  // 右腿
		Name = TEXT("ScaleRef_1.8m_Human");
	}
	return SpawnReferenceMesh(V, T, Name, Location);
}

// ---------------- D4 参考体网格（灰色锁定自由网格） ----------------
ABlockoutActor* UBlockoutEditorSubsystem::SpawnReferenceMesh(const TArray<FVector>& Verts, const TArray<int32>& Tris, const FString& Name, const FVector& Location)
{
	if (Verts.Num() == 0 || Tris.Num() < 3) return nullptr;
	UWorld* World = GetEditingWorld();
	if (!World) return nullptr;

	PushUndo(TEXT("添加参考体"));
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABlockoutActor* Actor = World->SpawnActor<ABlockoutActor>(Location, FRotator::ZeroRotator, P);
	if (!Actor) return nullptr;

	Actor->Data.Id = FGuid::NewGuid();
	Actor->Data.Shape = EBlockoutShape::Cube;
	Actor->Data.bFreeform = true;
	Actor->Data.Vertices = Verts;
	Actor->Data.Triangles = Tris;
	Actor->Data.Color = FLinearColor(0.65f, 0.65f, 0.65f);
	Actor->Data.Name = Name;
	Actor->Data.bLocked = true;
	Actor->Data.Transform = FTransform(FRotator::ZeroRotator, Location);
	Actor->Rebuild();
	AllBlocks.Add(Actor);
	NotifyDirty();
	return Actor;
}

void UBlockoutEditorSubsystem::RecoverIfNeeded()
{
	// 崩溃恢复：存在 autosave 且比项目文件新时自动加载
	FString AutoPath = FPaths::ProjectSavedDir() / TEXT("BlockoutSnapshots/autosave.blockout.json");
	if (IFileManager::Get().FileExists(*AutoPath))
	{
		LoadProject(AutoPath);
	}
}

// ---------------- 查询 ----------------

TArray<ABlockoutActor*> UBlockoutEditorSubsystem::GetAllBlocks() const
{
	return AllBlocks;
}

ABlockoutActor* UBlockoutEditorSubsystem::GetActorById(const FGuid& Id) const
{
	for (ABlockoutActor* Actor : AllBlocks)
	{
		if (Actor && Actor->Data.Id == Id) return Actor;
	}
	return nullptr;
}

// ---------------- 数据 <-> 网格转换（M2/M4 辅助） ----------------
namespace
{
	void DataToMesh(const FBlockoutObjectData& D, FDynamicMesh3& Out)
	{
		Out.Clear();
		if (D.bFreeform && D.Vertices.Num() > 0 && D.Triangles.Num() >= 3)
		{
			TArray<int32> Map; Map.Reserve(D.Vertices.Num());
			for (const FVector& V : D.Vertices) Map.Add(Out.AppendVertex(FVector3d(V)));
			for (int32 i = 0; i + 2 < D.Triangles.Num(); i += 3)
			{
				Out.AppendTriangle(Map[D.Triangles[i]], Map[D.Triangles[i + 1]], Map[D.Triangles[i + 2]]);
			}
		}
		else
		{
			FBlockoutMeshGen::BuildShape(D.Shape, D.Params, Out);
		}
	}

	void MeshToData(const FDynamicMesh3& M, FBlockoutObjectData& D)
	{
		D.bFreeform = true;
		D.Vertices.Reset();
		D.Triangles.Reset();
		// 顶点索引可能不连续，需重映射
		TMap<int32, int32> Map;
		for (int32 vid : M.VertexIndicesItr())
		{
			Map.Add(vid, D.Vertices.Add(FVector(M.GetVertex(vid))));
		}
		for (int32 tid : M.TriangleIndicesItr())
		{
			UE::Geometry::FIndex3i T = M.GetTriangle(tid);
			if (const int32* A = Map.Find(T.A))
			{
				if (const int32* B = Map.Find(T.B))
				{
					if (const int32* C = Map.Find(T.C))
					{
						D.Triangles.Add(*A); D.Triangles.Add(*B); D.Triangles.Add(*C);
					}
				}
			}
		}
	}

	bool IsWallLike(EBlockoutShape S)
	{
		return S == EBlockoutShape::Wall || S == EBlockoutShape::DoorWall
			|| S == EBlockoutShape::WindowWall || S == EBlockoutShape::Arch
			|| S == EBlockoutShape::Floor || S == EBlockoutShape::Cube;
	}
}

// ---------------- M2 手柄拖拽改尺寸 ----------------

void UBlockoutEditorSubsystem::GetResizeHandles(TArray<FVector>& OutPositions, TArray<int32>& OutAxes, TArray<int32>& OutSigns) const
{
	OutPositions.Reset(); OutAxes.Reset(); OutSigns.Reset();
	if (Selection.Num() != 1) return;
	ABlockoutActor* Actor = Selection[0];
	if (!IsValid(Actor)) return;
	const FBox Box = Actor->GetComponentsBoundingBox(true);
	const FVector C = Box.GetCenter();
	const FVector E = Box.GetExtent();
	static const FVector Ax[3] = { FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector };
	for (int32 A = 0; A < 3; ++A)
	{
		OutPositions.Add(C + Ax[A] * E[A]); OutAxes.Add(A); OutSigns.Add(1);
		OutPositions.Add(C - Ax[A] * E[A]); OutAxes.Add(A); OutSigns.Add(-1);
	}
}

bool UBlockoutEditorSubsystem::BeginResizeDrag(const FVector& RayOrigin, const FVector& RayDir, int32 AxisIndex, int32 Sign)
{
	if (Selection.Num() != 1 || AxisIndex < 0 || AxisIndex > 2) return false;
	ABlockoutActor* Actor = Selection[0];
	if (!IsValid(Actor)) return false;

	PushUndo(TEXT("M2 手柄改尺寸"));
	bResizeDragging = true;
	ResizeDragHandle = AxisIndex * 2 + (Sign > 0 ? 0 : 1);
	ResizeAxis = AxisIndex;
	ResizeSign = Sign;
	ResizeTarget = Actor;
	ResizeStartTransform = Actor->Data.Transform;
	ResizeStartParams = Actor->Data.Params;
	ResizeStartVerts = Actor->Data.Vertices;

	const FBox Box = Actor->GetComponentsBoundingBox(true);
	const FVector E = Box.GetExtent();
	ResizeStartSize = (float)(E[AxisIndex] * 2.0); // 该轴当前总长

	// 拖拽平面：含拖拽轴，法线垂直于轴与视线
	static const FVector Ax[3] = { FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector };
	const FVector Axis = Ax[AxisIndex];
	FVector PlaneN = FVector::CrossProduct(FVector::CrossProduct(RayDir.GetSafeNormal(), Axis), Axis);
	if (PlaneN.IsNearlyZero()) PlaneN = RayDir.GetSafeNormal();
	ResizePlaneNormal = PlaneN.GetSafeNormal();
	ResizePlaneOrigin = Box.GetCenter();
	FPlane Plane(ResizePlaneOrigin, ResizePlaneNormal);
	ResizeStartHit = FMath::LinePlaneIntersection(RayOrigin, RayOrigin + RayDir * 1e6f, Plane);
	return true;
}

void UBlockoutEditorSubsystem::UpdateResizeDrag(const FVector& RayOrigin, const FVector& RayDir)
{
	if (!bResizeDragging || !IsValid(ResizeTarget)) return;
	FPlane Plane(ResizePlaneOrigin, ResizePlaneNormal);
	const FVector Hit = FMath::LinePlaneIntersection(RayOrigin, RayOrigin + RayDir * 1e6f, Plane);
	static const FVector Ax[3] = { FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector };
	const FVector Axis = Ax[ResizeAxis];
	float Amount = (float)FVector::DotProduct(Hit - ResizeStartHit, Axis) * (float)ResizeSign;
	if (bGridSnap) Amount = FMath::GridSnap(Amount, SnapStep);

	ABlockoutActor* Actor = ResizeTarget;
	FBlockoutObjectData& D = Actor->Data;
	const float NewSize = FMath::Max(10.f, ResizeStartSize + Amount * 2.f);

	if (D.bFreeform && ResizeStartVerts.Num() > 0)
	{
		// 自由网格：按轴缩放顶点（对面固定：以 Sign 侧为基准缩放）
		const float Factor = NewSize / FMath::Max(ResizeStartSize, 1.f);
		D.Vertices = ResizeStartVerts;
		float MinV = 1e30f, MaxV = -1e30f;
		for (const FVector& V : D.Vertices) { MinV = FMath::Min(MinV, (float)V[ResizeAxis]); MaxV = FMath::Max(MaxV, (float)V[ResizeAxis]); }
		const float Anchor = (ResizeSign > 0) ? MinV : MaxV;
		for (FVector& V : D.Vertices)
		{
			V[ResizeAxis] = Anchor + (V[ResizeAxis] - Anchor) * Factor;
		}
	}
	else
	{
		// 参数化体块：轴 → 参数映射
		FBlockoutShapeParams P = ResizeStartParams;
		const EBlockoutShape S = D.Shape;
		if (ResizeAxis == 0) P.Length = NewSize;
		else if (ResizeAxis == 1)
		{
			const bool bThin = (S == EBlockoutShape::Wall || S == EBlockoutShape::DoorWall
				|| S == EBlockoutShape::WindowWall || S == EBlockoutShape::Arch || S == EBlockoutShape::Pipe);
			if (bThin) P.Thickness = NewSize; else P.Width = NewSize;
		}
		else P.Height = NewSize;
		D.Params = P;
	}

	// 位置补偿：保持对面固定
	const FVector WorldAxis = Actor->GetActorRotation().RotateVector(Axis);
	const float HalfDelta = (NewSize - ResizeStartSize) * 0.5f;
	const FVector NewLoc = ResizeStartTransform.GetLocation() + WorldAxis * (HalfDelta * (float)ResizeSign);
	D.Transform = ResizeStartTransform;
	D.Transform.SetLocation(NewLoc);
	Actor->Rebuild();
	Actor->SyncTransform();
	bDirty = true;
}

void UBlockoutEditorSubsystem::EndResizeDrag()
{
	if (bResizeDragging) NotifyDirty();
	bResizeDragging = false;
	ResizeTarget = nullptr;
	ResizeDragHandle = -1;
}

// ---------------- M4 Auto-Align 自动开洞 ----------------

int32 UBlockoutEditorSubsystem::TryAutoAlignPiece(ABlockoutActor* Piece)
{
	if (!IsValid(Piece)) return 0;
	const EBlockoutShape S = Piece->Data.Shape;
	// 门/窗/拱构件对重叠墙类自动开洞
	if (S != EBlockoutShape::DoorPiece && S != EBlockoutShape::WindowPiece && S != EBlockoutShape::Arch) return 0;

	const FBox PieceBox = Piece->GetComponentsBoundingBox(true);
	int32 CutCount = 0;
	for (ABlockoutActor* Wall : AllBlocks)
	{
		if (!IsValid(Wall) || Wall == Piece) continue;
		if (!IsWallLike(Wall->Data.Shape)) continue;
		const FBox WallBox = Wall->GetComponentsBoundingBox(true);
		FBox Overlap = WallBox.Overlap(PieceBox);
		if (!Overlap.IsValid) continue;
		if (CutHoleIntoWall(Wall, Piece)) ++CutCount;
	}
	if (CutCount > 0) NotifyDirty();
	return CutCount;
}

bool UBlockoutEditorSubsystem::CutHoleIntoWall(ABlockoutActor* Wall, ABlockoutActor* Piece)
{
	if (!IsValid(Wall) || !IsValid(Piece)) return false;

	// 墙体网格
	FDynamicMesh3 WallMesh;
	DataToMesh(Wall->Data, WallMesh);

	// 洞体：按构件形状构建（在墙本地空间定位）
	const FBlockoutShapeParams& PP = Piece->Data.Params;
	FDynamicMesh3 Hole;
	if (Piece->Data.Shape == EBlockoutShape::Arch)
	{
		const double HoleH = PP.Height * 0.5;
		FDynamicMesh3 Rect;
		FBlockoutMeshGen::BuildRectPrismHole(Rect, PP.Length, HoleH, 2000.0);
		FDynamicMesh3 Circ;
		FBlockoutMeshGen::BuildCirclePrismHole(Circ, PP.Length, 2000.0, 24);
		FBlockoutMeshGen::BooleanOp(Rect, Circ, FTransform(FVector(0, 0, HoleH * 0.5)), false);
		Hole = MoveTemp(Rect);
	}
	else
	{
		FBlockoutMeshGen::BuildRectPrismHole(Hole, PP.Length, PP.Height, 2000.0);
	}

	// 构件中心 → 墙本地坐标，洞体沿墙 Y 贯穿
	const FTransform WallT = Wall->GetActorTransform();
	const FVector Local = WallT.InverseTransformPosition(Piece->GetActorLocation());
	FTransform HoleT(FRotator::ZeroRotator, FVector(Local.X, 0.0, Local.Z));
	if (!FBlockoutMeshGen::BooleanOp(WallMesh, Hole, HoleT, true)) return false;

	MeshToData(WallMesh, Wall->Data);
	Wall->Rebuild();
	return true;
}

bool UBlockoutEditorSubsystem::CutCustomHole(ABlockoutActor* Wall, const TArray<FVector2D>& PolygonLocalXZ)
{
	if (!IsValid(Wall) || PolygonLocalXZ.Num() < 3) return false;
	PushUndo(TEXT("M4 自定义开洞"));
	FDynamicMesh3 WallMesh;
	DataToMesh(Wall->Data, WallMesh);
	FDynamicMesh3 Hole;
	FBlockoutMeshGen::BuildPolygonPrismHole(Hole, PolygonLocalXZ, 2000.0);
	if (!FBlockoutMeshGen::BooleanOp(WallMesh, Hole, FTransform::Identity, true)) return false;
	MeshToData(WallMesh, Wall->Data);
	Wall->Rebuild();
	NotifyDirty();
	return true;
}
