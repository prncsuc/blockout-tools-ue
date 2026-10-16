// BlockoutStudio - 编辑器核心子系统实现
#include "BlockoutEditorSubsystem.h"
#include "BlockoutActor.h"
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
		FBlockoutObjectData D;
		if (FJsonObjectConverter::JsonObjectToUStruct(Val->AsObject().ToSharedRef(), &D, 0, 0))
		{
			Datas.Add(D);
		}
	}
	RestoreAll(Datas);
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
	return SaveProject(Path);
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
