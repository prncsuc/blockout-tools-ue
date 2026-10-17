// BlockoutStudio - 游戏模式实现
#include "BlockoutGameMode.h"
#include "BlockoutEditorPlayerController.h"
#include "BlockoutHUD.h"

ABlockoutCameraPawn::ABlockoutCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

ABlockoutGameMode::ABlockoutGameMode()
{
	DefaultPawnClass = ABlockoutCameraPawn::StaticClass();
	PlayerControllerClass = ABlockoutEditorPlayerController::StaticClass();
	HUDClass = ABlockoutHUD::StaticClass();
}
