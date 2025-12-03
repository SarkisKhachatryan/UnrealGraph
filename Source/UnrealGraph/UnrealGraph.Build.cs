using UnrealBuildTool;

public class UnrealGraph : ModuleRules
{
	public UnrealGraph(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
		);
				
		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);
			
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Json",
				"JsonUtilities"
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"UnrealEd",
				"ToolMenus",
				"EditorStyle",
				"EditorWidgets",
				"BlueprintGraph",
				"GraphEditor",
				"Kismet",
				"KismetCompiler",
				"PropertyEditor",
				"EditorSubsystem",
				"InputCore",
				"Projects",
				"ApplicationCore"
			}
		);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}

