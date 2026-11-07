# Builds and Releases

## Build Configurations

```powershell
msbuild weave.sln /p:Configuration=Debug /p:Platform=x64
msbuild weave.sln /p:Configuration=Release /p:Platform=x64
```

Use Visual Studio 2022 with the C++ desktop workload and Windows SDK. Intermediate files go to `build/x64/<configuration>` and outputs to `dist/x64/<configuration>`. Clone or update submodules recursively to obtain MinHook.

## Versioning

The root `VERSION` file is the source of the three-component package version. The build generates a version header and Windows DLL version resource. `WeaveGetVersion()` returns that version plus the current Git revision; a repository with no commit uses `uncommitted`; uncommitted changes do not change the reported revision. `WEAVE_API_VERSION` is a separate binary API version and must match the header used by the caller.

## Artifacts

The workflow builds pushes and pull requests to `dev`, version tags and manual runs. Build artifacts contain:

```text
weave-sdk-x64.zip
├── bin/weave_proxy.dll
├── include/weave.h
├── lib/weave_proxy.lib
├── docs/
├── licenses/MinHook.txt
├── LICENSE
├── README.md
└── VERSION

weave_proxy.pdb
```

The SDK DLL, import library and separate PDB come from the same build. Keep the PDB matching the deployed DLL. GitHub Releases are created for `v*` tags only when the tag matches `VERSION`, for example `v0.0.1` for `0.0.1`.
