using UnrealBuildTool;

public class BlockoutStudio : ModuleRules
{
	public BlockoutStudio(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"RHI",
			"RenderCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"JsonUtilities",
			"GeometryCore",
			"GeometryFramework",
			"InteractiveToolsFramework",
			"ImageWrapper",
			"EnhancedInput",
			// DesktopPlatform 不可再分发（Shipping 打包报错），改用 Win32 通用对话框 FBlockoutFileDialog
		});
	}
}
