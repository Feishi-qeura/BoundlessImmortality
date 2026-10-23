using UnrealBuildTool;

public class BoundlessImmortalityEditor : ModuleRules
{
    public BoundlessImmortalityEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add(System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "../BoundlessImmortality")));
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Niagara" });
        PrivateDependencyModuleNames.Add("NiagaraUIRenderer");
        PrivateDependencyModuleNames.Add("AssetTools");
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "NiagaraEditor", "AssetRegistry", "Json", "JsonUtilities", "InputCore", "Kismet", "KismetCompiler", "Slate", "SlateCore", "MeshDescription", "StaticMeshDescription", "BoundlessImmortality", "LiveInteraction", "UMG", "UMGEditor" });
    }
}
