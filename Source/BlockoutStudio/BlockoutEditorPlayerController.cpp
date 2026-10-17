// BlockoutStudio - 编辑器输入层实现
#include "BlockoutEditorPlayerController.h"
#include "BlockoutGizmo.h"
#include "BlockoutActor.h"
#include "BlockoutEditorSubsystem.h"
#include "BlockoutEditorUI.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CollisionQueryParams.h"

UBlockoutEditorSubsystem* GetEditorSub(UWorld* World)
{
	if (UGameInstance* GI = World ? World->GetGameInstance() : nullptr)
	{
		return GI->GetSubsystem<UBlockoutEditorSubsystem>();
	}
	return nullptr;
}

void ABlockoutEditorPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;

	// 创建 Slate 工具 UI 并挂入视口
	SlateUI = SNew(SBlockoutEditorUI).PC(this);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(SlateUI.ToSharedRef(), 100);
	}
}

void ABlockoutEditorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent) return;

	// 相机移动（Tick 中读轴值）
	InputComponent->BindAxis(TEXT("MoveForward"), this, &ABlockoutEditorPlayerController::MoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"), this, &ABlockoutEditorPlayerController::MoveRight);
	InputComponent->BindAxis(TEXT("MoveUp"), this, &ABlockoutEditorPlayerController::MoveUp);

	// 鼠标
	InputComponent->BindAxis(TEXT("MouseX"), this, &ABlockoutEditorPlayerController::OnMouseMoveX);
	InputComponent->BindAxis(TEXT("MouseY"), this, &ABlockoutEditorPlayerController::OnMouseMoveY);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ABlockoutEditorPlayerController::OnLeftMouseDown);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ABlockoutEditorPlayerController::OnLeftMouseUp);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ABlockoutEditorPlayerController::OnRightMouseDown);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ABlockoutEditorPlayerController::OnRightMouseUp);
	InputComponent->BindAxis(TEXT("MouseWheel"), this, &ABlockoutEditorPlayerController::OnMouseWheel);

	// 快捷键（编辑）
	InputComponent->BindKey(EKeys::W, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyModeTranslate);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyModeRotate);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyModeScale);
	InputComponent->BindKey(EKeys::Delete, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyDelete);
	InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyUndo);
	InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyRedo);
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyDuplicate);
	InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyToggleGridSnap);
	InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyToggleSurfaceSnap);
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyToggleCoordSpace);
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyFocus);
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeySelectAll);
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeySave);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyEscape);
	InputComponent->BindKey(EKeys::O, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyGroup);
	InputComponent->BindKey(EKeys::U, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyUngroup);
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyToggleUI);
}

void ABlockoutEditorPlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);
	UpdateCamera(DeltaSeconds);

	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld()))
	{
		Sub->Update(DeltaSeconds);
	}

	// gizmo 拖拽中：持续更新
	if (bGizmoDragging)
	{
		FVector Origin, Dir;
		if (GetMouseRay(Origin, Dir))
		{
			if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld()))
			{
				Sub->UpdateGizmoDrag(Origin, Dir);
			}
		}
	}

	// 框选中：更新矩形
	if (bBoxSelecting)
	{
		float X, Y;
		if (GetMousePosition(X, Y))
		{
			BoxEnd = FVector2D(X, Y);
		}
	}

	// 同步 gizmo 显示状态
	if (ABlockoutGizmo* Gizmo = EnsureGizmo())
	{
		if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld()))
		{
			const TArray<ABlockoutActor*>& Sel = Sub->GetSelection();
			Gizmo->SetActorHiddenInGame(Sel.Num() == 0);
			if (Sel.Num() > 0)
			{
				FBox Box(ForceInit);
				for (ABlockoutActor* A : Sel)
				{
					if (IsValid(A)) Box += A->GetComponentsBoundingBox(true);
				}
				Gizmo->Location = Box.GetCenter();
				Gizmo->Mode = Sub->GetTransformMode();
				Gizmo->CoordSpace = Sub->GetCoordSpace();
				Gizmo->bDragging = bGizmoDragging;
				if (Sel.Num() > 0 && IsValid(Sel[0]))
				{
					Gizmo->Orientation = Sel[0]->GetActorRotation();
				}
			}
		}
	}
}

// ---------- 相机 ----------
void ABlockoutEditorPlayerController::MoveForward(float Value)
{
	if (!bRotateCamera) return;
	if (FMath::Abs(Value) > KINDA_SMALL_NUMBER && GetPawn())
	{
		const FRotator Rot = GetControlRotation();
		GetPawn()->AddActorWorldOffset(FVector(Rot.Vector() * Value * CameraSpeed * GetWorld()->GetDeltaSeconds()), false);
	}
}

void ABlockoutEditorPlayerController::MoveRight(float Value)
{
	if (!bRotateCamera) return;
	if (FMath::Abs(Value) > KINDA_SMALL_NUMBER && GetPawn())
	{
		const FRotator Rot(0.f, GetControlRotation().Yaw, 0.f);
		GetPawn()->AddActorWorldOffset(FRotator(0, 90, 0).RotateVector(Rot.Vector()) * Value * CameraSpeed * GetWorld()->GetDeltaSeconds(), false);
	}
}

void ABlockoutEditorPlayerController::MoveUp(float Value)
{
	if (!bRotateCamera) return;
	if (FMath::Abs(Value) > KINDA_SMALL_NUMBER && GetPawn())
	{
		GetPawn()->AddActorWorldOffset(FVector::UpVector * Value * CameraSpeed * GetWorld()->GetDeltaSeconds(), false);
	}
}

void ABlockoutEditorPlayerController::UpdateCamera(float DeltaSeconds)
{
	if (APawn* P = GetPawn())
	{
		P->SetActorRotation(GetControlRotation());
	}
}

void ABlockoutEditorPlayerController::OnMouseWheel(float Value)
{
	if (FMath::Abs(Value) > KINDA_SMALL_NUMBER && GetPawn())
	{
		const FRotator Rot = GetControlRotation();
		GetPawn()->AddActorWorldOffset(Rot.Vector() * Value * 120.f, false);
	}
}

void ABlockoutEditorPlayerController::OnMouseMoveX(float Value) { MouseDX += Value; }

void ABlockoutEditorPlayerController::OnMouseMoveY(float Value)
{
	MouseDY += Value;
	if (bRotateCamera && GetPawn())
	{
		AddYawInput(MouseDX * 0.15f);
		AddPitchInput(-MouseDY * 0.15f);
	}
}

// ---------- 鼠标按键 ----------
void ABlockoutEditorPlayerController::OnLeftMouseDown()
{
	FVector Origin, Dir;
	if (!GetMouseRay(Origin, Dir)) return;

	// 1) gizmo 轴拾取优先
	if (ABlockoutGizmo* Gizmo = EnsureGizmo())
	{
		if (!Gizmo->IsHidden())
		{
			const int32 Axis = Gizmo->HitTestAxis(Origin, Dir);
			if (Axis >= 0)
			{
				if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld()))
				{
					Sub->BeginGizmoDrag(Origin, Dir, Axis);
					bGizmoDragging = true;
					Gizmo->HotAxis = Axis;
					return;
				}
			}
		}
	}

	// 2) 物体拾取
	ABlockoutActor* Hit = PickActor(Origin, Dir);
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld()))
	{
		const bool bAdditive = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
		if (Hit)
		{
			Sub->SelectActor(Hit, bAdditive);
		}
		else if (!bAdditive)
		{
			Sub->ClearSelection();
			// 空白处开始框选
			float X, Y;
			if (GetMousePosition(X, Y))
			{
				bBoxSelecting = true;
				BoxStart = BoxEnd = FVector2D(X, Y);
			}
		}
	}
}

void ABlockoutEditorPlayerController::OnLeftMouseUp()
{
	if (bGizmoDragging)
	{
		if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld()))
		{
			Sub->EndGizmoDrag();
		}
		if (ABlockoutGizmo* Gizmo = EnsureGizmo())
		{
			Gizmo->HotAxis = -1;
		}
		bGizmoDragging = false;
	}
	else if (bBoxSelecting)
	{
		bBoxSelecting = false;
		const bool bAdditive = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
		if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld()))
		{
			if (FVector2D::Distance(BoxStart, BoxEnd) > 4.f)
			{
				Sub->SelectInBox(BoxStart, BoxEnd, bAdditive);
			}
		}
	}
}

void ABlockoutEditorPlayerController::OnRightMouseDown()
{
	bRotateCamera = true;
	bShowMouseCursor = false;
}

void ABlockoutEditorPlayerController::OnRightMouseUp()
{
	bRotateCamera = false;
	bShowMouseCursor = true;
	MouseDX = MouseDY = 0.f;
}

// ---------- 射线 ----------
bool ABlockoutEditorPlayerController::GetMouseRay(FVector& OutOrigin, FVector& OutDir) const
{
	float X, Y;
	if (!GetMousePosition(X, Y)) return false;
	FVector WorldLoc, WorldDir;
	if (DeprojectScreenPositionToWorld(X, Y, WorldLoc, WorldDir))
	{
		OutOrigin = WorldLoc;
		OutDir = WorldDir.GetSafeNormal();
		return true;
	}
	return false;
}

ABlockoutActor* ABlockoutEditorPlayerController::PickActor(const FVector& RayOrigin, const FVector& RayDir) const
{
	// 自实现射线-包围盒拾取，避免通道依赖
	UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld());
	if (!Sub) return nullptr;

	ABlockoutActor* Best = nullptr;
	double BestDist = TNumericLimits<double>::Max();

	for (ABlockoutActor* A : Sub->GetAllBlocks())
	{
		if (!IsValid(A) || A->Data.bHidden || A->Data.bLocked) continue;
		const FBox Box = A->GetComponentsBoundingBox(true);

		// slab 法射线-AABB 求交，Tmin 为命中参数
		double TMin = 0.0, TMax = TNumericLimits<double>::Max();
		bool bHit = true;
		for (int32 Axis = 0; Axis < 3 && bHit; ++Axis)
		{
			const double O = RayOrigin[Axis];
			const double D = RayDir[Axis];
			const double Lo = Box.Min[Axis];
			const double Hi = Box.Max[Axis];
			if (FMath::Abs(D) < KINDA_SMALL_NUMBER)
			{
				if (O < Lo || O > Hi) bHit = false;
			}
			else
			{
				double T1 = (Lo - O) / D;
				double T2 = (Hi - O) / D;
				if (T1 > T2) { const double Tmp = T1; T1 = T2; T2 = Tmp; }
				TMin = FMath::Max(TMin, T1);
				TMax = FMath::Min(TMax, T2);
				if (TMin > TMax) bHit = false;
			}
		}
		if (bHit && TMin < BestDist)
		{
			BestDist = TMin;
			Best = A;
		}
	}
	return Best;
}

ABlockoutGizmo* ABlockoutEditorPlayerController::EnsureGizmo()
{
	if (!IsValid(GizmoActor) && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags = RF_Transient;
		GizmoActor = GetWorld()->SpawnActor<ABlockoutGizmo>(ABlockoutGizmo::StaticClass(), FTransform::Identity, Params);
	}
	return GizmoActor;
}

// ---------- 快捷键动作 ----------
void ABlockoutEditorPlayerController::OnKeyModeTranslate()
{
	if (bRotateCamera) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->SetTransformMode(EBlockoutTransformMode::Translate);
}
void ABlockoutEditorPlayerController::OnKeyModeRotate()
{
	if (bRotateCamera) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->SetTransformMode(EBlockoutTransformMode::Rotate);
}
void ABlockoutEditorPlayerController::OnKeyModeScale()
{
	if (bRotateCamera) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->SetTransformMode(EBlockoutTransformMode::Scale);
}
void ABlockoutEditorPlayerController::OnKeyDelete()
{
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->DeleteSelected();
}
void ABlockoutEditorPlayerController::OnKeyUndo()
{
	if (!IsInputKeyDown(EKeys::LeftControl) && !IsInputKeyDown(EKeys::RightControl)) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->Undo();
}
void ABlockoutEditorPlayerController::OnKeyRedo()
{
	if (!IsInputKeyDown(EKeys::LeftControl) && !IsInputKeyDown(EKeys::RightControl)) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->Redo();
}
void ABlockoutEditorPlayerController::OnKeyDuplicate()
{
	if (!IsInputKeyDown(EKeys::LeftControl) && !IsInputKeyDown(EKeys::RightControl)) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->DuplicateSelected();
}
void ABlockoutEditorPlayerController::OnKeyToggleGridSnap()
{
	if (bRotateCamera) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->SetGridSnap(!Sub->GetGridSnap());
}
void ABlockoutEditorPlayerController::OnKeyToggleSurfaceSnap()
{
	if (bRotateCamera) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->SetSurfaceSnap(!Sub->GetSurfaceSnap());
}
void ABlockoutEditorPlayerController::OnKeyToggleCoordSpace()
{
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->ToggleCoordSpace();
}
void ABlockoutEditorPlayerController::OnKeyFocus()
{
	FocusSelection();
}
void ABlockoutEditorPlayerController::OnKeySelectAll()
{
	if (!IsInputKeyDown(EKeys::LeftControl) && !IsInputKeyDown(EKeys::RightControl)) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->SelectAll();
}
void ABlockoutEditorPlayerController::OnKeySave()
{
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->SaveProject(Sub->GetDefaultProjectPath());
}
void ABlockoutEditorPlayerController::OnKeyEscape()
{
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->ClearSelection();
}
void ABlockoutEditorPlayerController::OnKeyGroup()
{
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->GroupSelected();
}
void ABlockoutEditorPlayerController::OnKeyUngroup()
{
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->UngroupSelected();
}
void ABlockoutEditorPlayerController::OnKeyToggleUI()
{
	ToggleUIShow();
}

// ---------- 对外动作 ----------
void ABlockoutEditorPlayerController::CreateBlockAtCursor(EBlockoutShape Shape)
{
	UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld());
	if (!Sub) return;
	FVector Origin, Dir;
	FVector SpawnLoc = FVector::ZeroVector;
	if (GetMouseRay(Origin, Dir))
	{
		SpawnLoc = Origin + Dir * 600.f;
	}
	FBlockoutShapeParams Params;
	ABlockoutActor* A = Sub->SpawnBlock(Shape, SpawnLoc, Params);
	if (A) Sub->SelectActor(A, false);
}

void ABlockoutEditorPlayerController::FocusSelection()
{
	UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld());
	if (!Sub || Sub->GetSelection().Num() == 0 || !GetPawn()) return;
	FBox Box(ForceInit);
	for (ABlockoutActor* A : Sub->GetSelection())
	{
		if (IsValid(A)) Box += A->GetComponentsBoundingBox(true);
	}
	const FVector Center = Box.GetCenter();
	const float Radius = FMath::Max(Box.GetExtent().Size(), 100.f);
	const FRotator Rot = GetControlRotation();
	GetPawn()->SetActorLocation(Center - Rot.Vector() * Radius * 2.5f);
}

void ABlockoutEditorPlayerController::ToggleUIShow()
{
	if (SlateUI.IsValid())
	{
		bUIMode = !bUIMode;
		SlateUI->SetVisibility(bUIMode ? EVisibility::Collapsed : EVisibility::Visible);
	}
}
