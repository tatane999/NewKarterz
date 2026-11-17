using UnrealBuildTool;
public class NewKarterzEditorTarget : TargetRules
{
	public NewKarterzEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NewKarterz");
	}
}
