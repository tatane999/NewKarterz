using UnrealBuildTool;
public class NewKarterzTarget : TargetRules
{
	public NewKarterzTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NewKarterz");
	}
}
