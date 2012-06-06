@echo off

cd ..

REM Win32
msbuild HexEdit.sln /p:Configuration="Unicode Release" /p:Platform=Win32

REM Win64
msbuild HexEdit.sln /p:Configuration="Unicode Release" /p:Platform=x64

cd build