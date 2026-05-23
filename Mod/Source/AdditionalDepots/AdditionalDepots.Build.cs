using UnrealBuildTool;

public class AdditionalDepots : ModuleRules
{
	public AdditionalDepots(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bUseUnity = false;
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "ReliableMessaging" });
		PublicDependencyModuleNames.AddRange(new string[] { "DummyHeaders" });
		PublicDependencyModuleNames.AddRange(new string[] { "FactoryGame", "SML" });
	}
}
