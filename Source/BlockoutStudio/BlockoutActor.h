// BlockoutStudio - 白盒体块 Actor
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicMesh3.h"
#include "BlockoutTypes.h"
#include "BlockoutActor.generated.h"

class UDynamicMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class BLOCKOUTSTUDIO_API ABlockoutActor : public AActor
{
	GENERATED_BODY()

public:
	ABlockoutActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Blockout")
	UDynamicMeshComponent* MeshComp;

	// 体块数据（形状/参数/变换/外观）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blockout")
	FBlockoutObjectData Data;

	// 网格缓存（导出/测量用）
	UE::Geometry::FDynamicMesh3 CachedMesh;

	// 依据 Data 重建网格与外观
	void Rebuild();

	// 仅同步变换
	void SyncTransform();

	// 应用外观（颜色/图案/平铺）
	void ApplyAppearance();

	void SetHighlighted(bool bOn);

	bool IsHighlighted() const { return bHighlighted; }

	// 序列化辅助
	FBlockoutObjectData CaptureData() const { return Data; }
	void RestoreData(const FBlockoutObjectData& InData);

protected:
	virtual void BeginPlay() override;

private:
	UMaterialInstanceDynamic* EnsureMID();
	bool bHighlighted = false;
	UMaterialInstanceDynamic* MID = nullptr;
};
