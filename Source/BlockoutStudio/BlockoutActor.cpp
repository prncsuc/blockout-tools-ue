// BlockoutStudio - 白盒体块 Actor 实现
#include "BlockoutActor.h"
#include "Components/DynamicMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "BlockoutMeshGen.h"
#include "BlockoutTextureLib.h"

ABlockoutActor::ABlockoutActor()
{
	PrimaryActorTick.bCanEverTick = false;
	MeshComp = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("DynamicMesh"));
	RootComponent = MeshComp;
	MeshComp->SetMobility(EComponentMobility::Movable);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComp->EnableComplexAsSimpleCollision();
	MeshComp->bUseAsyncCooking = true;
	MeshComp->SetGenerateOverlapEvents(false);
}

void ABlockoutActor::BeginPlay()
{
	Super::BeginPlay();
	Rebuild();
}

void ABlockoutActor::Rebuild()
{
	FDynamicMesh3 Mesh;
	if (Data.bFreeform && Data.Triangles.Num() > 0 && Data.Vertices.Num() > 0)
	{
		for (const FVector& V : Data.Vertices)
		{
			Mesh.AppendVertex(FVector3d(V));
		}
		for (int32 i = 0; i + 2 < Data.Triangles.Num(); i += 3)
		{
			Mesh.AppendTriangle(Data.Triangles[i], Data.Triangles[i + 1], Data.Triangles[i + 2]);
		}
		FBlockoutMeshGen::FinalizeMesh(Mesh, false, 100.0);
	}
	else
	{
		FBlockoutMeshGen::BuildShape(Data.Shape, Data.Params, Mesh);
		bool bSmooth = (Data.Shape == EBlockoutShape::Sphere || Data.Shape == EBlockoutShape::Capsule ||
			Data.Shape == EBlockoutShape::Cylinder || Data.Shape == EBlockoutShape::Pipe);
		FBlockoutMeshGen::FinalizeMesh(Mesh, bSmooth, 100.0);
	}
	MeshComp->SetMesh(MoveTemp(Mesh));
	MeshComp->NotifyMeshUpdated();
	// 保留网格缓存供导出/测量使用
	if (const FDynamicMesh3* DynMesh = MeshComp->GetMesh())
	{
		CachedMesh = *DynMesh;
	}
	ApplyAppearance();
	SyncTransform();
}

void ABlockoutActor::SyncTransform()
{
	SetActorTransform(Data.Transform);
}

void ABlockoutActor::ApplyAppearance()
{
	UMaterialInstanceDynamic* Mat = EnsureMID();
	if (Mat)
	{
		Mat->SetVectorParameterValue(TEXT("BaseColor"), Data.Color);
		Mat->SetScalarParameterValue(TEXT("Tiling"), Data.Tiling);
		Mat->SetScalarParameterValue(TEXT("Highlight"), bHighlighted ? 1.f : 0.f);
		UTexture2D* Tex = FBlockoutTextureLib::GetPatternTexture(Data.TexturePattern);
		if (Tex && Data.TexturePattern != EBlockoutTexturePattern::None)
		{
			Mat->SetTextureParameterValue(TEXT("Pattern"), Tex);
			Mat->SetScalarParameterValue(TEXT("PatternStrength"), 1.f);
		}
		else
		{
			Mat->SetScalarParameterValue(TEXT("PatternStrength"), 0.f);
		}
	}
	MeshComp->SetCollisionEnabled(Data.bHidden ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	SetActorHiddenInGame(Data.bHidden);
	SetActorEnableCollision(!Data.bHidden && !Data.bLocked);
}

UMaterialInstanceDynamic* ABlockoutActor::EnsureMID()
{
	if (MID) return MID;
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/BlockoutStudio/Materials/M_BlockoutBase.M_BlockoutBase"));
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (Base)
	{
		MID = UMaterialInstanceDynamic::Create(Base, this);
		MeshComp->SetMaterial(0, MID);
	}
	return MID;
}

void ABlockoutActor::SetHighlighted(bool bOn)
{
	bHighlighted = bOn;
	if (MID)
	{
		MID->SetScalarParameterValue(TEXT("Highlight"), bOn ? 1.f : 0.f);
	}
}

void ABlockoutActor::RestoreData(const FBlockoutObjectData& InData)
{
	Data = InData;
	Rebuild();
}
