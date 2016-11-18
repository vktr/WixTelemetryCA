#addin nuget:?package=Cake.CMake&version=0.0.2

//////////////////////////////////////////////////////////////////////
// ARGUMENTS
//////////////////////////////////////////////////////////////////////

var target        = Argument("target", "Default");
var configuration = Argument("configuration", "Release");
var platform      = Argument("platform", "x64");

// Parameters
var OutputDirectory = Directory("./build-" + platform);
var BuildDirectory  = OutputDirectory + Directory(configuration);
var Version         = System.IO.File.ReadAllText("VERSION").Trim();

//////////////////////////////////////////////////////////////////////
// TASKS
//////////////////////////////////////////////////////////////////////

Task("Clean")
    .Does(() =>
{
    CleanDirectory(BuildDirectory);
});

Task("Generate-Project")
    .IsDependentOn("Clean")
    .Does(() =>
{
    var generator = (platform == "x86")
        ? "Visual Studio 14"
        : "Visual Studio 14 Win64";

    CMake("./", new CMakeSettings {
      OutputPath = OutputDirectory,
      Generator = generator,
      Toolset = "v140"
    });
});

Task("Build")
    .IsDependentOn("Generate-Project")
    .Does(() =>
{
    var settings = new MSBuildSettings()
                        .SetConfiguration(configuration);

    if(platform == "x86")
    {
        settings.WithProperty("Platform", "Win32")
                .SetPlatformTarget(PlatformTarget.x86);
    }
    else
    {
        settings.WithProperty("Platform", "x64")
                .SetPlatformTarget(PlatformTarget.x64);
    }

    MSBuild(OutputDirectory + File("WixTelemetryExtension.sln"), settings);
});

Task("Build-Test-Installer")
    .IsDependentOn("Build")
    .Does(() =>
{
    var arch = Architecture.X64;

    if(platform == "x86")
    {
        arch = Architecture.X86;
    }

    WiXCandle("./test/TestInstaller.wxs", new CandleSettings
    {
        Architecture = arch,
        Defines = new Dictionary<string, string>
        {
            { "BuildDirectory", BuildDirectory },
            { "Platform", platform }
        },
        OutputDirectory = BuildDirectory
    });

    WiXLight(BuildDirectory + File("TestInstaller.wixobj"), new LightSettings
    {
        OutputFile = BuildDirectory + File("TestInstaller.msi")
    });
});

//////////////////////////////////////////////////////////////////////
// TASK TARGETS
//////////////////////////////////////////////////////////////////////

Task("Default")
    .IsDependentOn("Build")
    ;

Task("Package")
    .IsDependentOn("Build-Test-Installer")
    ;

RunTarget(target);
