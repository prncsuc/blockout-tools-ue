// BlockoutStudio - 变换 Gizmo 实现
#include "BlockoutGizmo.h"

ABlockoutGizmo::ABlockoutGizmo()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	SetCanBeDamaged(false);
}

FVector ABlockoutGizmo::GetAxisVector(int32 AxisIndex) const
{
	FVector Local = (AxisIndex == 0) ? FVector::ForwardVector
		: (AxisIndex == 1) ? FVector::RightVector : FVector::UpVector;
	if (CoordSpace == EBlockoutCoordSpace::Local)
	{
		return Orientation.RotateVector(Local);
	}
	return Local;
}

int32 ABlockoutGizmo::HitTestAxis(const FVector& RayOrigin, const FVector& RayDir) const
{
	// 射线与每条轴线段的最小距离判定，阈值随轴长度缩放
	const float SegLen = AxisLength * DisplayScale;
	const float Threshold = SegLen * 0.18f;

	int32 BestAxis = -1;
	double BestDist = Threshold;

	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const FVector Dir = GetAxisVector(Axis).GetSafeNormal();
		const FVector A = Location;
		const FVector B = Location + Dir * SegLen;

		// 射线与线段最近点（参数化求解）
		const FVector W0 = A - RayOrigin;
		const double A_ = FVector::DotProduct(RayDir, RayDir);
		const double B_ = FVector::DotProduct(RayDir, Dir);
		const double C_ = FVector::DotProduct(Dir, Dir);
		const double D_ = FVector::DotProduct(RayDir, W0);
		const double E_ = FVector::DotProduct(Dir, W0);
		const double Denom = A_ * C_ - B_ * B_;

		double RayT = 0.0, SegT = 0.0;
		if (FMath::Abs(Denom) > KINDA_SMALL_NUMBER)
		{
			RayT = (B_ * E_ - C_ * D_) / Denom;
			SegT = (A_ * E_ - B_ * D_) / Denom;
		}
		SegT = FMath::Clamp(SegT, 0.0, 1.0);
		RayT = FMath::Max(RayT, 0.0);

		const FVector P1 = RayOrigin + RayDir * RayT;
		const FVector P2 = A + Dir * SegT;
		const double Dist = FVector::Dist(P1, P2);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			BestAxis = Axis;
		}
	}
	return BestAxis;
}
