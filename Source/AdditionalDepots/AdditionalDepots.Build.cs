using UnrealBuildTool;
using System.IO;
using System;

public class AdditionalDepots : ModuleRules
{
	public AdditionalDepots(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		// FactoryGame transitive dependencies
		// Not all of these are required, but including the extra ones saves you from having to add them later.
		// Some entries are commented out to avoid compile-time warnings about depending on a module that you don't explicitly depend on.
		// You can uncomment these as necessary when your code actually needs to use them.
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject",
			"Engine",
			//"DeveloperSettings",
			//"PhysicsCore",
			"InputCore",
			//"GeometryCollectionEngine",
			//"AnimGraphRuntime",
			//"AssetRegistry",
			//"NavigationSystem",
			//"AIModule",
			"GameplayTasks",
			"SlateCore", "Slate", "UMG",
			//"RenderCore",
			//"CinematicCamera",
			//"Foliage",
			"NetCore",
			//"GameplayTags",
			//"Json", "JsonUtilities"
			"ReliableMessaging"
        });

		// Header stubs
		PublicDependencyModuleNames.AddRange(new string[] {
			"DummyHeaders",
		});

		if (Target.Type == TargetRules.TargetType.Editor) {
			//PublicDependencyModuleNames.AddRange(new string[] {"AnimGraph"});
		}
		PublicDependencyModuleNames.AddRange(new string[] {"FactoryGame", "SML"});
	}
}
