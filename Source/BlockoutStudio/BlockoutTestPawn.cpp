// BlockoutTestPawn.cpp - 测试模式跑图角色：UE 官方 FPS/TPS 模板（Manny/Quinn + Enhanced Input）
#include "BlockoutTestPawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 官方模板资产路径（Content\Characters\Mannequins + Content\UEFirstPerson）
	static const TCHAR* MannyMeshPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	static const TCHAR* QuinnMeshPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple");
	static const TCHAR* UnarmedABPPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C");
	static const TCHAR* FPArmsMeshPath = TEXT("/Game/UEFirstPerson/FirstPersonArms/Character/Mesh/SK_Mannequin_Arms.SK_Mannequin_Arms");
	static const TCHAR* FPArmsABPPath = TEXT("/Game/UEFirstPerson/FirstPersonArms/Animations/FirstPerson_AnimBP.FirstPerson_AnimBP_C");
	static const TCHAR* IMC_DefaultPath = TEXT("/Game/UEFirstPerson/FirstPerson/Input/IMC_Default.IMC_Default");
	static const TCHAR* IA_MovePath = TEXT("/Game/UEFirstPerson/FirstPerson/Input/Actions/IA_Move.IA_Move");
	static const TCHAR* IA_LookPath = TEXT("/Game/UEFirstPerson/FirstPerson/Input/Actions/IA_Look.IA_Look");
	static const TCHAR* IA_JumpPath = TEXT("/Game/UEFirstPerson/FirstPerson/Input/Actions/IA_Jump.IA_Jump");
}

ABlockoutTestPawn::ABlockoutTestPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// 官方模板胶囊：半径 34 / 半高 88（1.75m 角色）
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	// 官方小白人 Manny + 官方 Unarmed 动画蓝图（ABP 自驱动走/跑/跳/蹲姿态）
	if (GetMesh())
	{
		if (USkeletalMesh* Manny = LoadObject<USkeletalMesh>(nullptr, MannyMeshPath))
		{
			GetMesh()->SetSkeletalMesh(Manny);
		}
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, UnarmedABPPath))
		{
			GetMesh()->SetAnimInstanceClass(AnimClass);
		}
	}

	// 官方第一人称手臂（挂相机上，FPS 视角可见；官方 FPS 模板结构）
	FPArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArms"));
	FPArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FPArms->CastShadow = false;

	// 官方 ThirdPerson 模板相机：SpringArm 400 / SocketOffset Z50 / 防穿模碰撞拉近
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.f;
	SpringArm->SocketOffset = FVector(0.f, 0.f, 50.f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	FPArms->SetupAttachment(Camera);

	// 官方输入资产（缺失时保持 legacy 行为，不崩溃）
	MoveIMC = LoadObject<UInputMappingContext>(nullptr, IMC_DefaultPath);
	MoveIA = LoadObject<UInputAction>(nullptr, IA_MovePath);
	LookIA = LoadObject<UInputAction>(nullptr, IA_LookPath);
	JumpIA = LoadObject<UInputAction>(nullptr, IA_JumpPath);

	// 运行时构造 Shift 冲刺 / Ctrl 蹲（官方模板同类操作键位）
	AuxIMC = NewObject<UInputMappingContext>(this, TEXT("IMC_BlockoutAux"), RF_Transient);
	SprintIA = NewObject<UInputAction>(this, TEXT("IA_Sprint"), RF_Transient);
	SprintIA->ValueType = EInputActionValueType::Boolean;
	CrouchIA = NewObject<UInputAction>(this, TEXT("IA_Crouch"), RF_Transient);
	CrouchIA->ValueType = EInputActionValueType::Boolean;
	if (AuxIMC && SprintIA && CrouchIA)
	{
		AuxIMC->MapKey(SprintIA, EKeys::LeftShift);
		AuxIMC->MapKey(CrouchIA, EKeys::LeftControl);
	}

	// 官方模板移动参数（走/跑/蹲/跳）
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->bUseControllerDesiredRotation = false;
	Move->RotationRate = FRotator(0.f, 500.f, 0.f);
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->MaxWalkSpeed = 500.f;
	Move->MaxWalkSpeedCrouched = 250.f;
	Move->JumpZVelocity = 420.f;
	Move->AirControl = 0.2f;

	// 默认第三人称（官方 TPS 模板），ApplyPreset 仅覆盖移动参数尺度
	ApplyPreset(EBlockoutTestPreset::CS);
	ApplyViewMode();
}

void ABlockoutTestPawn::ApplyPreset(EBlockoutTestPreset InPreset)
{
	Preset = InPreset;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	switch (Preset)
	{
	case EBlockoutTestPreset::CS:
		// 类 CS:GO：快节奏、高跳
		Move->MaxWalkSpeed = 500.f;
		Move->MaxWalkSpeedCrouched = 130.f;
		Move->JumpZVelocity = 420.f;
		Move->GravityScale = 1.0f;
		Move->AirControl = 0.65f;
		Move->BrakingDecelerationWalking = 2000.f;
		break;
	case EBlockoutTestPreset::Souls:
		// 类魂：低速、重力感强、空中控制弱
		Move->MaxWalkSpeed = 380.f;
		Move->MaxWalkSpeedCrouched = 110.f;
		Move->JumpZVelocity = 340.f;
		Move->GravityScale = 1.8f;
		Move->AirControl = 0.25f;
		Move->BrakingDecelerationWalking = 1200.f;
		break;
	case EBlockoutTestPreset::Uncharted:
		// 类神海：跑酷节奏、高跳、强空中控制
		Move->MaxWalkSpeed = 560.f;
		Move->MaxWalkSpeedCrouched = 140.f;
		Move->JumpZVelocity = 520.f;
		Move->GravityScale = 1.1f;
		Move->AirControl = 0.85f;
		Move->BrakingDecelerationWalking = 1800.f;
		break;
	}
}

void ABlockoutTestPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	PushInputMapping();
	ApplyViewMode();
}

void ABlockoutTestPawn::UnPossessed()
{
	PopInputMapping();
	Super::UnPossessed();
}

void ABlockoutTestPawn::PushInputMapping()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (MoveIMC) Sub->AddMappingContext(MoveIMC, 0);
			if (AuxIMC) Sub->AddMappingContext(AuxIMC, 1);
		}
	}
}

void ABlockoutTestPawn::PopInputMapping()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (MoveIMC) Sub->RemoveMappingContext(MoveIMC);
			if (AuxIMC) Sub->RemoveMappingContext(AuxIMC);
		}
	}
}

void ABlockoutTestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 官方模板操作：IA_Move(WASD) / IA_Look(鼠标) / IA_Jump(空格)
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveIA)
		{
			EIC->BindAction(MoveIA, ETriggerEvent::Triggered, this, &ABlockoutTestPawn::InputMove);
		}
		if (LookIA)
		{
			EIC->BindAction(LookIA, ETriggerEvent::Triggered, this, &ABlockoutTestPawn::InputLook);
		}
		if (JumpIA)
		{
			EIC->BindAction(JumpIA, ETriggerEvent::Started, this, &ABlockoutTestPawn::InputJump);
			EIC->BindAction(JumpIA, ETriggerEvent::Completed, this, &ABlockoutTestPawn::InputJumpStop);
		}
		if (SprintIA)
		{
			EIC->BindAction(SprintIA, ETriggerEvent::Started, this, &ABlockoutTestPawn::InputSprint);
			EIC->BindAction(SprintIA, ETriggerEvent::Completed, this, &ABlockoutTestPawn::InputSprintStop);
		}
		if (CrouchIA)
		{
			EIC->BindAction(CrouchIA, ETriggerEvent::Started, this, &ABlockoutTestPawn::InputCrouch);
		}
	}
}

void ABlockoutTestPawn::InputMove(const FInputActionValue& Value)
{
	// 官方 FirstPerson/ThirdPerson 模板移动逻辑：X=右 / Y=前
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller)
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Axis.X);
	}
}

void ABlockoutTestPawn::InputLook(const FInputActionValue& Value)
{
	// 官方模板视角：X=Yaw / Y=Pitch
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ABlockoutTestPawn::InputJump(const FInputActionValue& Value)
{
	DoJump();
}

void ABlockoutTestPawn::InputJumpStop(const FInputActionValue& Value)
{
	StopJumping();
}

void ABlockoutTestPawn::InputSprint(const FInputActionValue& Value)
{
	SetRunning(true);
}

void ABlockoutTestPawn::InputSprintStop(const FInputActionValue& Value)
{
	SetRunning(false);
}

void ABlockoutTestPawn::InputCrouch(const FInputActionValue& Value)
{
	ToggleCrouch();
}

void ABlockoutTestPawn::ApplyViewMode()
{
	if (bFPS)
	{
		// 官方 FirstPerson 模板：控制器朝向驱动、无躯体、第一人称手臂可见
		bUseControllerRotationYaw = true;
		bUseControllerRotationPitch = false;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		SpringArm->TargetArmLength = 0.f;
		SpringArm->SocketOffset = FVector(0.f, 0.f, 65.f);
		SpringArm->bEnableCameraLag = false;
		if (GetMesh()) GetMesh()->SetVisibility(false, true);
		if (FPArms)
		{
			FPArms->SetVisibility(true, true);
			if (USkeletalMesh* Arms = LoadObject<USkeletalMesh>(nullptr, FPArmsMeshPath))
			{
				FPArms->SetSkeletalMesh(Arms);
			}
			if (!FPArms->GetAnimInstance() && FPArms->GetSkeletalMeshAsset())
			{
				if (UClass* ArmsABP = LoadClass<UAnimInstance>(nullptr, FPArmsABPPath))
				{
					FPArms->SetAnimInstanceClass(ArmsABP);
				}
			}
		}
	}
	else
	{
		// 官方 ThirdPerson 模板：朝移动方向转身、跟随臂相机、Manny/Quinn 可见
		bUseControllerRotationYaw = false;
		bUseControllerRotationPitch = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		SpringArm->TargetArmLength = 400.f;
		SpringArm->SocketOffset = FVector(0.f, 0.f, 50.f);
		SpringArm->bEnableCameraLag = true;
		if (GetMesh()) GetMesh()->SetVisibility(true, true);
		if (FPArms) FPArms->SetVisibility(false, true);
	}
}

void ABlockoutTestPawn::ToggleViewMode()
{
	bFPS = !bFPS;
	ApplyViewMode();
}

void ABlockoutTestPawn::ToggleCharacter()
{
	bQuinn = !bQuinn;
	if (GetMesh())
	{
		if (USkeletalMesh* NewMesh = LoadObject<USkeletalMesh>(nullptr, bQuinn ? QuinnMeshPath : MannyMeshPath))
		{
			GetMesh()->SetSkeletalMesh(NewMesh);
		}
	}
}

void ABlockoutTestPawn::SetRunning(bool bInRunning)
{
	bRun = bInRunning;
	if (!bIsCrouched)
	{
		GetCharacterMovement()->MaxWalkSpeed = bRun
			? (Preset == EBlockoutTestPreset::Uncharted ? 560.f : (Preset == EBlockoutTestPreset::CS ? 500.f : 380.f))
			: 200.f;
	}
}

void ABlockoutTestPawn::ToggleCrouch()
{
	if (bIsCrouched) UnCrouch(); else Crouch();
}

void ABlockoutTestPawn::DoJump()
{
	if (!GetCharacterMovement()->IsFalling())
	{
		bJumpTracking = true;
		JumpStart = GetActorLocation();
		JumpApexZ = JumpStart.Z;
	}
	Jump();
}

void ABlockoutTestPawn::DoRoll()
{
	// 翻滚：水平冲量（附加动作）
	const FVector Fwd = FRotator(0.f, GetActorRotation().Yaw, 0.f).Vector();
	LaunchCharacter(Fwd * 700.f + FVector(0.f, 0.f, 150.f), true, true);
}

void ABlockoutTestPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bJumpTracking)
	{
		JumpApexZ = FMath::Max(JumpApexZ, GetActorLocation().Z);
	}
}

void ABlockoutTestPawn::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (bJumpTracking)
	{
		bJumpTracking = false;
		const FVector Now = GetActorLocation();
		LastJumpHeight = FMath::Max(0.f, JumpApexZ - JumpStart.Z);
		LastJumpDistance = FVector::Dist2D(Now, JumpStart);
	}
}

FString ABlockoutTestPawn::GetStatusText() const
{
	const FString PresetStr = Preset == EBlockoutTestPreset::CS ? TEXT("类CS")
		: Preset == EBlockoutTestPreset::Souls ? TEXT("类魂") : TEXT("类神海");
	const float Speed = GetVelocity().Size2D();
	return FString::Printf(TEXT("测试[%s] %s(%s)  速度 %.0f  跳高 %.0f  跳距 %.0f"),
		*PresetStr, bFPS ? TEXT("FPS") : TEXT("TPS"), bQuinn ? TEXT("Quinn") : TEXT("Manny"),
		Speed, LastJumpHeight, LastJumpDistance);
}
