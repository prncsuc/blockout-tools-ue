// BlockoutTestPawn.cpp - P4 测试模式跑图角色实现
#include "BlockoutTestPawn.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

ABlockoutTestPawn::ABlockoutTestPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// 胶囊碰撞：1.75m 高角色
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	// 移动组件基础参数
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->bUseControllerDesiredRotation = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->SetWalkableFloorAngle(52.f);
	Move->MaxStepHeight = 45.f;

	// 相机：弹簧臂 + 相机，控制旋转
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 320.f;
	SpringArm->SocketOffset = FVector(0.f, 0.f, 55.f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 12.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;

	// 默认 CS 尺度
	ApplyPreset(EBlockoutTestPreset::CS);
}

void ABlockoutTestPawn::ApplyPreset(EBlockoutTestPreset InPreset)
{
	Preset = InPreset;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	switch (Preset)
	{
	case EBlockoutTestPreset::CS:
		// 类 CS:GO：快节奏、高跳、低重力
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

void ABlockoutTestPawn::ToggleViewMode()
{
	bFPS = !bFPS;
	if (SpringArm)
	{
		SpringArm->TargetArmLength = bFPS ? 0.f : 320.f;
		SpringArm->SocketOffset = bFPS ? FVector(0.f, 0.f, 64.f) : FVector(0.f, 0.f, 55.f);
	}
}

void ABlockoutTestPawn::SetRunning(bool bInRunning)
{
	bRun = bInRunning;
	GetCharacterMovement()->MaxWalkSpeed = bRun
		? (Preset == EBlockoutTestPreset::Uncharted ? 560.f : (Preset == EBlockoutTestPreset::CS ? 500.f : 380.f))
		: 200.f;
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
	// 翻滚：水平冲量
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
	return FString::Printf(TEXT("测试[%s] %s  速度 %.0f  跳高 %.0f  跳距 %.0f"),
		*PresetStr, bFPS ? TEXT("FPS") : TEXT("TPS"), Speed, LastJumpHeight, LastJumpDistance);
}
