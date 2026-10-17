// BlockoutTestPawn.h - P4 测试模式跑图角色（FPS/TPS、走跑蹲跳、CS/魂/神海尺度、跳跃测量）
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlockoutTestPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

// P4 三套角色尺度预设
UENUM(BlueprintType)
enum class EBlockoutTestPreset : uint8
{
	CS        UMETA(DisplayName = "类CS:GO"),
	Souls     UMETA(DisplayName = "类魂"),
	Uncharted UMETA(DisplayName = "类神海")
};

// 测试模式角色：胶囊碰撞 + CharacterMovement，FPS/TPS 双视角
UCLASS()
class BLOCKOUTSTUDIO_API ABlockoutTestPawn : public ACharacter
{
	GENERATED_BODY()

public:
	ABlockoutTestPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;

	// P4 尺度预设
	void ApplyPreset(EBlockoutTestPreset InPreset);
	EBlockoutTestPreset GetPreset() const { return Preset; }

	// P2 FPS/TPS 切换
	void ToggleViewMode();
	bool IsFPS() const { return bFPS; }

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
	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera;

	EBlockoutTestPreset Preset = EBlockoutTestPreset::CS;
	bool bFPS = false;
	bool bRun = false;

	// 跳跃测量状态
	bool bJumpTracking = false;
	FVector JumpStart = FVector::ZeroVector;
	float JumpApexZ = 0.f;
	float LastJumpHeight = 0.f;
	float LastJumpDistance = 0.f;
};
