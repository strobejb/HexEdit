@echo off

cd ..

REM Win32 Release
msbuild HexEdit.sln /p:Configuration="Unicode Release" /p:Platform=Win32

REM Win64 Release
rem msbuild HexEdit.sln /p:Configuration="Unicode Release" /p:Platform=x64

cd build
