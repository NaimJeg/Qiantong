using System.IO;
using UnrealBuildTool;

public class QiantongCore : ModuleRules
{
	public QiantongCore(ReadOnlyTargetRules Target) : base(Target)
	{
		// UBT adapter only: CMake compiles portable sources without UE or PCH.
		Type = ModuleType.External;
		if (Target.Platform != UnrealTargetPlatform.Win64)
		{
			throw new BuildException("QiantongCore library wiring currently supports Win64 only.");
		}
		string Configuration = Target.bDebugBuildsActuallyUseDebugCRT ? "Debug" : "Release";
		string Library = Path.GetFullPath(Path.Combine(ModuleDirectory,
			"../../build/core", Configuration, "QiantongCore.lib"));
		if (!File.Exists(Library))
		{
			throw new BuildException("Build the portable library first: powershell -File Tools/Build.ps1");
		}
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PublicAdditionalLibraries.Add(Library);
		// Invalidate UBT's makefile and reject stale libraries after Core edits.
		foreach (string Folder in new string[] { "Public", "Private" })
		{
			foreach (string Input in Directory.GetFiles(Path.Combine(ModuleDirectory, Folder), "*", SearchOption.AllDirectories))
			{
				ExternalDependencies.Add(Input);
				if (File.GetLastWriteTimeUtc(Input) > File.GetLastWriteTimeUtc(Library))
				{
					throw new BuildException("Portable Core library is stale. Run powershell -File Tools/Build.ps1 first.");
				}
			}
		}
	}
}
