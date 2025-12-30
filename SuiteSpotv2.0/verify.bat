@echo off
set "MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (
    set "MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
)

echo [Verify] Starting Build...
"%MSBUILD%" SuiteSpot.vcxproj /p:Configuration=Release /p:Platform=x64 /p:TreatWarningsAsErrors=true /verbosity:minimal
if %errorlevel% neq 0 (
    echo [Verify] BUILD FAILED
    exit /b %errorlevel%
)

echo [Verify] Build Succeeded.
exit /b 0
