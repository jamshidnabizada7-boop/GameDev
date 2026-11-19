using UnrealBuildTool;
public class AfghanistanFPSEditorTarget : TargetRules
{
    public AfghanistanFPSEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("AfghanistanFPS");
    }
}
