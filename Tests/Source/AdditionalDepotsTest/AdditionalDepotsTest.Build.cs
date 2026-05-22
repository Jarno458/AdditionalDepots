using UnrealBuildTool;

public class AdditionalDepotsTest : ModuleRules
{
	public AdditionalDepotsTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bUseUnity = false;
		OptimizeCode = CodeOptimization.Never;

        PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject",
			"Engine",
			"InputCore",
			"GameplayTasks",
			"SlateCore", "Slate", "UMG",
			"NetCore",
        });

		// Header stubs
		PublicDependencyModuleNames.AddRange(new string[] {
			"DummyHeaders",
		});

		// Mod code depdencies
        PrivateDependencyModuleNames.AddRange(new string[] { 
            "AdditionalDepots"
        });

		PublicDependencyModuleNames.AddRange(new string[] {"FactoryGame", "SML"});
	}
}
