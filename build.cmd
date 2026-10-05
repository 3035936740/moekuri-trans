@echo off
setlocal
cd /d "%~dp0"
if defined VSCMD_ARG_TGT_ARCH goto compile
set "CN_VS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "CN_VS=%%i"
call "%CN_VS%\VC\Auxiliary\Build\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
:compile
if not "%VSCMD_ARG_TGT_ARCH%"=="x86" exit /b 1
if not exist build mkdir build
pushd build
cl /nologo /c /std:c++17 /O1 /Oi- /GS- /GR- /Zl /utf-8 /DNOMINMAX /DUNICODE /D_UNICODE /Fo:runtime.obj ..\src\runtime.cpp
if errorlevel 1 exit /b 1
link /nologo /dll /noentry /nodefaultlib /machine:x86 /dynamicbase /nxcompat /opt:ref /opt:icf /out:runtime.dll runtime.obj libcmt.lib kernel32.lib user32.lib gdi32.lib cabinet.lib
if errorlevel 1 exit /b 1
popd
set "CN_RC=rc.exe"
where rc.exe >nul 2>&1
if errorlevel 1 for /d %%s in ("%ProgramFiles(x86)%\Windows Kits\10\bin\*") do if exist "%%s\x86\rc.exe" set "CN_RC=%%s\x86\rc.exe"
pushd src
"%CN_RC%" /nologo /fo ..\build\tool.res tool.rc
if errorlevel 1 exit /b 1
popd
pushd build
set "CN_STAGE=MoeKuriTools.pending.%RANDOM%%RANDOM%.exe"
cl /nologo /std:c++17 /EHsc /O2 /MT /utf-8 /DNOMINMAX /DUNICODE /D_UNICODE /W4 /Fo:tool.obj ..\src\tool.cpp /Fe:%CN_STAGE% tool.res /link /subsystem:windows /dynamicbase /nxcompat user32.lib gdi32.lib comctl32.lib comdlg32.lib shell32.lib bcrypt.lib ole32.lib oleaut32.lib uuid.lib cabinet.lib fontsub.lib
if errorlevel 1 exit /b 1
popd
powershell -NoProfile -ExecutionPolicy Bypass -File install-build.ps1 -StagingName "%CN_STAGE%"
if errorlevel 1 exit /b 1
exit /b 0
