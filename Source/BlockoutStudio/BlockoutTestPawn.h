// BlockoutTestPawn.h - P4 测试模式跑图角色（UE 官方 FPS/TPS 模板：Manny/Quinn + Enhanced Input）
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "BlockoutTestPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USkeletalMeshComponent;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputComponent;

// P4 三套角色尺度预设（官方模板操作方式下的移动参数配置）
UENUM(BlueprintType)
enum class EBlockoutTestPreset : uint8
{
	CS        UMETA(DisplayName = "类CS:GO"),
	Souls     UMETA(DisplayName = "类魂"),
	Uncharted UMETA(DisplayName = "类神海")
};

// 测试模式角色：官方小白人 Manny/Quinn + 官方 FPS/TPS 模板操作（Enhanced Input IA_Move/IA_Look/IA_Jump）
UCLASS()
class BLOCKOUTSTUDIO_API ABlockoutTestPawn : public ACharacter
{
	GENERATED_BODY()

public:
	ABlockoutTestPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	// P4 尺度预设
	void ApplyPreset(EBlockoutTestPreset InPreset);
	EBlockoutTestPreset GetPreset() const { return Preset; }

	// P2 官方第一/第三人称模板切换
	void ToggleViewMode();
	bool IsFPS() const { return bFPS; }

	// 官方小白人切换（Manny <-> Quinn）
	void ToggleCharacter();

	// P3 移动
	void SetRunning(bool bInRunning);
	void ToggleCrouch();
	void DoJump();
	void DoRoll();

	// P5 跳跃高度/距离测量
	float GetLastJumpHeight() const { return LastJumpHeight; }
	float GetLastJumpDistance() const { return LastJumpDistance; }
	FString GetStatusText() const;

protected:
	// ---- Enhanced Input（官方模板操作：IA_Move/IA_Look/IA_Jump + Shift 冲刺 / Ctrl 蹲） ----
	void InputMove(const FInputActionValue& Value);
	void InputLook(const FInputActionValue& Value);
	void InputJump(const FInputActionValue& Value);
	void InputJumpStop(const FInputActionValue& Value);
	void InputSprint(const FInputActionValue& Value);
	void InputSprintStop(const FInputActionValue& Value);
	void InputCrouch(const FInputActionValue& Value);

	// 官方模板相机/朝向逻辑
	void ApplyViewMode();
	// 运行时加载官方输入资产并挂载映射上下文
	void PushInputMapping();
	void PopInputMapping();

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	// 官方第一人称手臂（FPS 视角可见 / TPS 隐藏）
	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* FPArms;

	EBlockoutTestPreset Preset = EBlockoutTestPreset::CS;
	bool bFPS = false;
	bool bRun = false;
	bool bQuinn = false;

	// 官方输入资产（/Game/UEFirstPerson/FirstPerson/Input）
	UPROPERTY() UInputMappingContext* MoveIMC;
	UPROPERTY() UInputAction* MoveIA;
	UPROPERTY() UInputAction* LookIA;
	UPROPERTY() UInputAction* JumpIA;
	// 运行时构造（官方模板同类操作：Shift 冲刺 / Ctrl 蹲）
	UPROPERTY() UInputMappingContext* AuxIMC;
	UPROPERTY() UInputAction* SprintIA;
	UPROPERTY() UInputAction* CrouchIA;

	// 跳跃测量状态
	bool bJumpTracking = false;
	FVector JumpStart = FVector::ZeroVector;
	float JumpApexZ = 0.f;
	float LastJumpHeight = 0.f;
	float LastJumpDistance = 0.f;
};
