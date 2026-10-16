// BlockoutStudio - 独立白盒关卡工具
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBlockoutStudioModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
