// BlockoutStudio - 核心数据类型定义
#pragma once

#include "CoreMinimal.h"
#include "BlockoutTypes.generated.h"

UENUM(BlueprintType)
enum class EBlockoutShape : uint8
{
	Cube        UMETA(DisplayName = "立方体"),
	Wall        UMETA(DisplayName = "墙体"),
	Staircase   UMETA(DisplayName = "楼梯"),
	Ramp        UMETA(DisplayName = "斜坡"),
	Wedge       UMETA(DisplayName = "楔形"),
	Arch        UMETA(DisplayName = "拱门"),
	DoorWall    UMETA(DisplayName = "门洞墙"),
	WindowWall  UMETA(DisplayName = "窗洞墙"),
	Cylinder    UMETA(DisplayName = "圆柱"),
	Sphere      UMETA(DisplayName = "球体"),
	Capsule     UMETA(DisplayName = "胶囊体"),
	Pipe        UMETA(DisplayName = "管道"),
	Colonnade   UMETA(DisplayName = "柱廊"),
	Fence       UMETA(DisplayName = "围墙"),
	Stairwell   UMETA(DisplayName = "楼梯间"),
	Floor       UMETA(DisplayName = "地板"),
	DoorPiece   UMETA(DisplayName = "门构件"),
	WindowPiece UMETA(DisplayName = "窗构件"),
	Custom      UMETA(DisplayName = "自定义网格")
};

UENUM(BlueprintType)
enum class EBlockoutSurfaceType : uint8
{
	Solid       UMETA(DisplayName = "实体"),
	Glass       UMETA(DisplayName = "玻璃"),
	Metal       UMETA(DisplayName = "金属"),
	Wood        UMETA(DisplayName = "木材"),
	Concrete    UMETA(DisplayName = "混凝土"),
	Grass       UMETA(DisplayName = "草地"),
	Dirt        UMETA(DisplayName = "泥土"),
	Water       UMETA(DisplayName = "水面"),
	Wall        UMETA(DisplayName = "墙体")
};

UENUM(BlueprintType)
enum class EBlockoutTransformMode : uint8
{
	Translate,
	Rotate,
	Scale
};

// ⑤ 交互方案：blockout toolkit 式 / ProBuilder 式，可在设置/菜单切换
UENUM(BlueprintType)
enum class EBlockoutInteractionMode : uint8
{
	Toolkit    UMETA(DisplayName = "Blockout Toolkit 式"),
	ProBuilder UMETA(DisplayName = "ProBuilder 式")
};

UENUM(BlueprintType)
enum class EBlockoutCoordSpace : uint8
{
	World,
	Local
};

UENUM(BlueprintType)
enum class EBlockoutPivotMode : uint8
{
	Center        UMETA(DisplayName = "物体中心"),
	BottomCenter  UMETA(DisplayName = "底面中心")
};

UENUM(BlueprintType)
enum class EBlockoutTexturePattern : uint8
{
	None        UMETA(DisplayName = "纯色"),
	Checker     UMETA(DisplayName = "棋盘格"),
	Grid        UMETA(DisplayName = "ProBuilder网格"),
	Brick       UMETA(DisplayName = "砖墙"),
	Concrete    UMETA(DisplayName = "混凝土"),
	Tile        UMETA(DisplayName = "瓷砖"),
	MetalPlate  UMETA(DisplayName = "金属板")
};

UENUM(BlueprintType)
enum class EBlockoutHoleShape : uint8
{
	Rectangle,
	Polygon,
	Circle
};

USTRUCT(BlueprintType)
struct FBlockoutShapeParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Length = 200.f;   // X 尺寸 (uu)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Width  = 200.f;   // Y 尺寸
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Height = 200.f;  // Z 尺寸
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Thickness = 20.f; // 墙厚/管壁
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AngleDeg = 90.f;  // 拱弧角度/斜坡角度
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Steps = 5;        // 台阶数/分段
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Segments = 24;    // 圆弧分段
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HoleWidth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HoleHeight = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float HoleOffsetZ = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ArrayCount = 4;   // 阵列数量
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float ArraySpacing = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bThroughHole = true;
};

USTRUCT(BlueprintType)
struct FBlockoutObjectData
{
	GENERATED_BODY()

	UPROPERTY() FGuid Id;
	UPROPERTY() FString Name;
	UPROPERTY() EBlockoutShape Shape = EBlockoutShape::Cube;
	UPROPERTY() FBlockoutShapeParams Params;
	UPROPERTY() FTransform Transform = FTransform::Identity;
	UPROPERTY() EBlockoutSurfaceType SurfaceType = EBlockoutSurfaceType::Wall;
	UPROPERTY() FLinearColor Color = FLinearColor(0.6f, 0.6f, 0.6f, 1.f);
	UPROPERTY() EBlockoutTexturePattern TexturePattern = EBlockoutTexturePattern::Grid;
	UPROPERTY() float Tiling = 1.f;
	UPROPERTY() bool bHidden = false;
	UPROPERTY() bool bLocked = false;
	UPROPERTY() FGuid GroupId;
	UPROPERTY() bool bFreeform = false; // 是否已转为自由网格（点线面编辑）
	UPROPERTY() TArray<FVector> Vertices;
	UPROPERTY() TArray<int32> Triangles;
};

USTRUCT()
struct FBlockoutUndoOp
{
	GENERATED_BODY()

	UPROPERTY() TArray<FBlockoutObjectData> Before;
	UPROPERTY() TArray<FBlockoutObjectData> After;
	UPROPERTY() FString Description;
	UPROPERTY() FString OpType;
	FDateTime Timestamp;
};

// 测量类型
UENUM(BlueprintType)
enum class EBlockoutMeasureType : uint8
{
	Distance    UMETA(DisplayName = "距离"),
	Dimensions  UMETA(DisplayName = "尺寸"),
	Area        UMETA(DisplayName = "面积"),
	Volume      UMETA(DisplayName = "体积"),
	Angle       UMETA(DisplayName = "角度"),
	HeightDiff  UMETA(DisplayName = "高差")
};

// 测量标注
USTRUCT()
struct FBlockoutMeasureItem
{
	GENERATED_BODY()

	UPROPERTY() EBlockoutMeasureType Type = EBlockoutMeasureType::Distance;
	UPROPERTY() FVector Start = FVector::ZeroVector;
	UPROPERTY() FVector End = FVector::ZeroVector;
	UPROPERTY() FString Label;
	UPROPERTY() FVector PointA = FVector::ZeroVector;
	UPROPERTY() FVector PointB = FVector::ZeroVector;
	UPROPERTY() double Value = 0.0;
	UPROPERTY() FString Text;
};

// 轨迹录制帧
USTRUCT()
struct FBlockoutRecordFrame
{
	GENERATED_BODY()

	UPROPERTY() float Time = 0.f;
	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() FRotator Rotation = FRotator::ZeroRotator;
	UPROPERTY() bool bCrouch = false;
	UPROPERTY() bool bFPS = true;
	UPROPERTY() FVector HeadBoneOffset = FVector::ZeroVector;
};

namespace BlockoutDefs
{
	const float SnapStepOptions[] = { 1.f, 5.f, 10.f, 25.f, 50.f, 100.f };
}
