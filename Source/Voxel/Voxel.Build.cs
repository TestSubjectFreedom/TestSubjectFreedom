using UnrealBuildTool;

public class Voxel : ModuleRules
{
	public Voxel(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
			{ "Core", "CoreUObject", "Engine", "ProceduralMeshComponent" });
		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] { "Voxel/Public" });
		PrivateIncludePaths.AddRange(new string[] { "Voxel/Private" });
	}
}