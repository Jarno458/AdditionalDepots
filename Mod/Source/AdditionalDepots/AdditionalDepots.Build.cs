using UnrealBuildTool;

public class AdditionalDepots : ModuleRules
{
	public AdditionalDepots(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bUseUnity = false;
		OptimizeCode = CodeOptimization.Never;

        // FactoryGame transitive dependencies
        // Not all of these are required, but including the extra ones saves you from having to add them later.
        // Some entries are commented out to avoid compile-time warnings about depending on a module that you don't explicitly depend on.
        // You can uncomment these as necessary when your code actually needs to use them.
        PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject",
			"Engine",
			"InputCore",
			"GameplayTasks",
			"SlateCore", "Slate", "UMG",
			"NetCore",
			"ReliableMessaging"
        });

		// Header stubs
		PublicDependencyModuleNames.AddRange(new string[] {
			"DummyHeaders",
		});

		PublicDependencyModuleNames.AddRange(new string[] {"FactoryGame", "SML"});
	}
}
