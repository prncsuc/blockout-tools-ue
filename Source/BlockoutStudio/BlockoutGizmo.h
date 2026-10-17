// BlockoutStudio - 变换 Gizmo（状态与拾取，可视化渲染由 HUD 负责）
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlockoutTypes.h"
#include "BlockoutGizmo.generated.h"

UCLASS()
class BLOCKOUTSTUDIO_API ABlockoutGizmo : public AActor
{
	GENERATED_BODY()

public:
	ABlockoutGizmo();

	// ---------- gizmo 状态（输入层写入，HUD 读取渲染） ----------
	UPROPERTY() EBlockoutTransformMode Mode = EBlockoutTransformMode::Translate;
	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() FRotator Orientation = FRotator::ZeroRotator;
	UPROPERTY() EBlockoutCoordSpace CoordSpace = EBlockoutCoordSpace::World;
	UPROPERTY() int32 HotAxis = -1;      // 高亮轴 0=X 1=Y 2=Z
	UPROPERTY() bool bDragging = false;
	UPROPERTY() float DisplayScale = 1.f;

	// 轴拾取：返回轴索引 0/1/2，-1 表示未命中
	int32 HitTestAxis(const FVector& RayOrigin, const FVector& RayDir) const;

	// 轴的世界向量
	FVector GetAxisVector(int32 AxisIndex) const;

	// 平移/缩放模式轴长度（世界单位）
	static constexpr float AxisLength = 120.f;
};
