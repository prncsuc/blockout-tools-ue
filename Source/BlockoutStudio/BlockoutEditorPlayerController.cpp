// BlockoutStudio - 编辑器输入层实现
#include "BlockoutEditorPlayerController.h"
#include "BlockoutGizmo.h"
#include "BlockoutActor.h"
#include "BlockoutEditorSubsystem.h"
#include "BlockoutEditorUI.h"
#include "BlockoutMeshEditLib.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CollisionQueryParams.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Slate/SceneViewport.h"

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
	// R1-R3 录制 / M3 子对象编辑
	InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyToggleRecord);
	InputComponent->BindKey(EKeys::F8, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyToggleSubEdit);
	InputComponent->BindKey(EKeys::X, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyExtrude);
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyBevel);
	InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ABlockoutEditorPlayerController::OnKeyCollapse);
}

void ABlockoutEditorPlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);
	UpdateCamera(DeltaSeconds);

	// R1：录制中逐帧采集轨迹 + 视口 JPEG
	if (RecSession.bRecording)
	{
		CaptureRecordFrame();
	}

	// R2：回放中按轨迹驱动相机
	if (bPlayingBack && GetPawn())
	{
		const double PlayTime = GetWorld()->GetTimeSeconds() - PlayStartTime;
		FBlockoutRecordFrame Frame;
		if (FBlockoutRecordLib::SamplePlayback(RecSession.Frames, PlayTime, Frame))
		{
			GetPawn()->SetActorLocation(Frame.Location);
			SetControlRotation(Frame.Rotation);
		}
		if (PlayTime > RecSession.Frames.Last().Time + 0.5)
		{
			StopPlayback();
		}
	}

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

	// 2) 子对象拾取（M3：顶点/面模式）
	if (SubEditMode > 0 && IsValid(EditTarget))
	{
		const bool bAdditive = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
		if (SubEditMode == 1)
		{
			const int32 Vid = FBlockoutMeshEditLib::PickVertex(EditTarget, Origin, Dir, 40.f);
			if (!bAdditive) SelectedVerts.Reset();
			if (Vid >= 0) SelectedVerts.AddUnique(Vid);
		}
		else
		{
			const int32 Tid = FBlockoutMeshEditLib::PickFace(EditTarget, Origin, Dir);
			if (!bAdditive) SelectedFaces.Reset();
			if (Tid >= 0) SelectedFaces.AddUnique(Tid);
		}
		return;
	}

	// 3) 物体拾取
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
	// M3：面编辑模式下删选中面，否则删物体
	if (SubEditMode == 2 && SelectedFaces.Num() > 0 && IsValid(EditTarget))
	{
		MeshDeleteFaces();
		return;
	}
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

// ==================== R1-R3 录制 ====================
void ABlockoutEditorPlayerController::StartRecording()
{
	if (RecSession.bRecording) return;
	const double Now = GetWorld()->GetTimeSeconds();
	FBlockoutRecordLib::StartRecording(RecSession, Now, 30);
	LastFrameCaptureTime = -1.0;
}

void ABlockoutEditorPlayerController::StopRecording()
{
	if (!RecSession.bRecording) return;
	FBlockoutRecordLib::StopRecording(RecSession, GetWorld()->GetTimeSeconds());
}

bool ABlockoutEditorPlayerController::SaveRecording(const FString& BaseName)
{
	if (RecSession.Frames.Num() == 0) return false;
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Records");
	IFileManager::Get().MakeDirectory(*Dir, true);

	const FString Base = BaseName.IsEmpty()
		? FString::Printf(TEXT("record_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")))
		: BaseName;

	bool bOk = FBlockoutRecordLib::SaveTrajectoryJSON(RecSession, Dir / (Base + TEXT(".json")));
	if (RecSession.JpegFrames.Num() > 0)
	{
		// R3：MJPEG-AVI 导出（备选录制方案）
		bOk = FBlockoutRecordLib::WriteMjpegAVI(RecSession, Dir / (Base + TEXT(".avi"))) || bOk;
	}
	return bOk;
}

void ABlockoutEditorPlayerController::StartPlayback()
{
	if (RecSession.Frames.Num() == 0) return;
	bPlayingBack = true;
	PlayStartTime = GetWorld()->GetTimeSeconds();
}

void ABlockoutEditorPlayerController::StopPlayback()
{
	bPlayingBack = false;
}

void ABlockoutEditorPlayerController::CaptureRecordFrame()
{
	const double Now = GetWorld()->GetTimeSeconds();
	// 30fps 节流
	if (LastFrameCaptureTime >= 0.0 && Now - LastFrameCaptureTime < 1.0 / 30.0) return;
	LastFrameCaptureTime = Now;

	FBlockoutRecordFrame Frame;
	Frame.Time = (float)(Now - RecSession.StartTime);
	if (APawn* P = GetPawn())
	{
		Frame.Location = P->GetActorLocation();
		Frame.Rotation = GetControlRotation();
	}
	FBlockoutRecordLib::AddFrame(RecSession, Frame);
	CaptureViewportJpeg();
}

void ABlockoutEditorPlayerController::CaptureViewportJpeg()
{
	// 读取视口像素并 JPEG 压缩（MJPEG-AVI 帧源）
	if (!GEngine || !GEngine->GameViewport) return;
	FViewport* Viewport = GEngine->GameViewport->GetGameViewport();
	if (!Viewport) return;

	FIntPoint Size = Viewport->GetSizeXY();
	if (Size.X <= 0 || Size.Y <= 0) return;

	TArray<FColor> Bitmap;
	if (!Viewport->ReadPixels(Bitmap)) return;

	// 缩到 1280x720 内以控制体积
	int32 W = Size.X, H = Size.Y;
	const int32 MaxW = 1280;
	if (W > MaxW)
	{
		H = FMath::RoundToInt((float)H * MaxW / W);
		W = MaxW;
	}
	TArray<FColor> Scaled;
	Scaled.SetNumUninitialized(W * H);
	for (int32 Y = 0; Y < H; ++Y)
	{
		const int32 SrcY = Y * Size.Y / H;
		for (int32 X = 0; X < W; ++X)
		{
			const int32 SrcX = X * Size.X / W;
			Scaled[Y * W + X] = Bitmap[SrcY * Size.X + SrcX];
		}
	}

	// JPEG 压缩（FColor 内存布局即 BGRA，直接喂）
	IImageWrapperModule& ImageWrapperModule =
		FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> JpegWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
	if (!JpegWrapper.IsValid()) return;
	if (!JpegWrapper->SetRaw(Scaled.GetData(), Scaled.Num() * sizeof(FColor), W, H, ERGBFormat::BGRA, 8)) return;

	TArray64<uint8> Compressed = JpegWrapper->GetCompressed(80);
	if (Compressed.Num() == 0) return;
	TArray<uint8> Jpeg;
	Jpeg.Append(Compressed.GetData(), (int32)Compressed.Num());

	if (RecSession.JpegFrames.Num() == 0)
	{
		RecSession.Width = W;
		RecSession.Height = H;
	}
	FBlockoutRecordLib::AddJpegFrame(RecSession, Jpeg);
}

void ABlockoutEditorPlayerController::OnKeyToggleRecord()
{
	if (bRotateCamera) return;
	if (RecSession.bRecording)
	{
		StopRecording();
		const bool bSaved = SaveRecording(FString());
		UE_LOG(LogTemp, Log, TEXT("BlockoutRecord: stopped, frames=%d jpeg=%d saved=%d"),
			RecSession.Frames.Num(), RecSession.JpegFrames.Num(), bSaved ? 1 : 0);
	}
	else
	{
		StartRecording();
	}
}

// ==================== M3 点线面编辑 ====================
void ABlockoutEditorPlayerController::OnKeyToggleSubEdit()
{
	if (bRotateCamera) return;
	// 循环：物体 → 顶点 → 面 → 物体
	SetSubEditMode((SubEditMode + 1) % 3);
	SelectedVerts.Reset();
	SelectedFaces.Reset();

	if (SubEditMode == 0)
	{
		EditTarget = nullptr;
		return;
	}

	// 以当前选中物体为编辑目标
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld()))
	{
		const TArray<ABlockoutActor*>& Sel = Sub->GetSelection();
		EditTarget = (Sel.Num() > 0 && IsValid(Sel[0])) ? Sel[0] : nullptr;
	}
}

void ABlockoutEditorPlayerController::MeshExtrudeFaces(float Distance)
{
	if (!IsValid(EditTarget) || SelectedFaces.Num() == 0) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->PushUndo(TEXT("ExtrudeFaces"));
	FBlockoutMeshEditLib::ExtrudeFaces(EditTarget, SelectedFaces, Distance);
	SelectedFaces.Reset();
}

void ABlockoutEditorPlayerController::MeshBevelVerts(float Amount)
{
	if (!IsValid(EditTarget) || SelectedVerts.Num() == 0) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->PushUndo(TEXT("BevelVertices"));
	FBlockoutMeshEditLib::BevelVertices(EditTarget, SelectedVerts, Amount);
	SelectedVerts.Reset();
}

void ABlockoutEditorPlayerController::MeshCollapseVerts()
{
	if (!IsValid(EditTarget) || SelectedVerts.Num() < 2) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->PushUndo(TEXT("CollapseVertices"));
	FBlockoutMeshEditLib::CollapseVertices(EditTarget, SelectedVerts);
	SelectedVerts.Reset();
}

void ABlockoutEditorPlayerController::MeshDeleteFaces()
{
	if (!IsValid(EditTarget) || SelectedFaces.Num() == 0) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->PushUndo(TEXT("DeleteFaces"));
	FBlockoutMeshEditLib::DeleteFaces(EditTarget, SelectedFaces);
	SelectedFaces.Reset();
}

void ABlockoutEditorPlayerController::MeshMoveVerts(const FVector& Delta)
{
	if (!IsValid(EditTarget) || SelectedVerts.Num() == 0) return;
	if (UBlockoutEditorSubsystem* Sub = GetEditorSub(GetWorld())) Sub->PushUndo(TEXT("MoveVertices"));
	FBlockoutMeshEditLib::MoveVertices(EditTarget, SelectedVerts, Delta);
}

void ABlockoutEditorPlayerController::OnKeyExtrude()
{
	if (bRotateCamera || SubEditMode != 2) return;
	MeshExtrudeFaces(50.f);
}
void ABlockoutEditorPlayerController::OnKeyBevel()
{
	if (bRotateCamera || SubEditMode != 1) return;
	MeshBevelVerts(0.25f);
}
void ABlockoutEditorPlayerController::OnKeyCollapse()
{
	if (bRotateCamera || SubEditMode != 1) return;
	MeshCollapseVerts();
}
