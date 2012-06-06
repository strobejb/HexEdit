@ECHO OFF
REM Example usage:
REM incbuild.bat VERSION_BUILD_COUNT /path/to/version.h
if [%1]==[] goto usage
if [%2]==[] goto usage

 REM you need this to set and read a variable inside
   REM a parethetical structure such as a FOR loop
   setlocal enabledelayedexpansion

   REM This is the file we are going to alter
   set filename=%2
   set basename=%~nx2
   
   REM Use temp file
   REM delete if already exists
   REM so we can use append operator for all output
   if exist "%filename%.temp" del "%filename%.temp"
   
   REM For each line in the file...
   REM ...using FOR alone to parse the file skips blank lines so we...
   REM ...parse the output (note single quotes) of...
   REM running TYPE on the file and piping the output through FINDSTR...
   REM ...with the /n switch (this adds a line number and a colon at the start of each line)
   REM the FINDSTR search string is ".*" (find any characters including cr/lf)
   REM Split into 2* tokens, the asterisk means %%R is the entire remainder of the line
   REM delimiter being the colon thus...
   REM The number is token 1, %%Q (discarded, along with the colon)
   REM The original source file line is token 2, %%R
   REM note we escape the pipe character with a caret ^ in the FOR dataset block
   for /f "tokens=1,2* delims=:" %%Q in ('type "%filename%" ^| findstr /n ".*"') do (
   
      REM if token 2 is null then the line is blank so we echo a blank line to the temp output file
      if "%%R"=="" echo. >> "%filename%.temp"
      
      REM This flag gets set to 1 if we have a line that needs changing
      set incflag=0
      
      REM Split the line into 3 tokens with white space the delimiter
      for /f "tokens=1-3 delims= " %%A in ("%%R") do (
      
         REM test if an increment needs to happen and set the flag if it does

         if "%%B"=="%1" set incflag=1
         REM if "%%B"=="VERSION_BUILD_HI" set incflag=1
         REM %%C is the number
         
         REM If the line contains a number to increment...
         if !incflag! equ 1 (
            REM do it...
            set /a num=%%C+1
            
            REM info msg to console
            echo %%B: !num!
            
            REM write the altered line to file
            echo %%A %%B !num! >>"%filename%.temp"
         
         REM the line is a nonblank one that simply needs copying   
         ) else (

            echo %%R >> "%filename%.temp"

         REM Match those parentheses!
         )
      )
   )

   REM delete original file
   del "%filename%"
   
   REM rename temp file to original file name
   ren "%filename%.temp" "%basename%"

goto :eof

:usage
echo Usage:
echo incbuild.bat VERSION_BUILD_COUNT /path/to/version.h
echo 


:eof
