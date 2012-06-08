@echo off

SET vcvars="c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
SET version_h=".\version.h"
SET resource_rc="..\src\HexEdit\HexEdit.rc"

REM update the version build count, and 
REM the resource-file prior to building the solution
incbuild.rb %version_h% %resource_rc%

REM path to the visual-studio commandline environment variables

REM spawn another shell and run 'build0' with the VCVARS
%comspec% /c "%vcvars% x86 && build0.bat"

REM package everything together
buildzip.rb
