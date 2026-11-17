using UnrealBuildTool;
public class NewKarterz : ModuleRules
{
	public NewKarterz(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ProceduralMeshComponent", "Json", "JsonUtilities", "UMG", "Slate", "SlateCore" });
	}
}
