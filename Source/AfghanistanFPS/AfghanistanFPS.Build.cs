using UnrealBuildTool;
public class AfghanistanFPS : ModuleRules
{
    public AfghanistanFPS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AnimGraphRuntime", "AnimationCore", "UMG", "Slate", "SlateCore", "AIModule", "NavigationSystem", "GameplayTasks" });
        if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "Kismet", "BlueprintGraph", "UMG", "Slate", "SlateCore" });
    }
}
