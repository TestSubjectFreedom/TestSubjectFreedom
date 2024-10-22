// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TestSubjectFreedom : ModuleRules
{
    public TestSubjectFreedom(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(new string[] { "Voxel/Public" });
        PrivateIncludePaths.AddRange(new string[] { "Voxel/Private" });

        PublicDependencyModuleNames.AddRange(new string[]
            { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ProceduralMeshComponent", "Voxel" });
    }
}