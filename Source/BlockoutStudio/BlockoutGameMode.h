// BlockoutStudio - 游戏模式与工具相机 Pawn
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "BlockoutGameMode.generated.h"

// 极简相机 Pawn：无碰撞、无内置输入，位置/朝向由 PlayerController 驱动
UCLASS()
class BLOCKOUTSTUDIO_API ABlockoutCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ABlockoutCameraPawn();
};

UCLASS()
class BLOCKOUTSTUDIO_API ABlockoutGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABlockoutGameMode();
};
