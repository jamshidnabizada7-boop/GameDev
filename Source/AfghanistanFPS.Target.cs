using UnrealBuildTool;
using System.Collections.Generic;
public class AfghanistanFPSTarget : TargetRules
{
    public AfghanistanFPSTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("AfghanistanFPS");
    }
}
