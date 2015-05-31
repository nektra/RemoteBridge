@ECHO OFF
SETLOCAL
IF NOT "%VCINSTALLDIR%" == "" GOTO do_process
IF "%VS90COMNTOOLS%" == "" GOTO show_err

ECHO Setting up a Visual Studio x64 Command Prompt environment... 
CALL "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
IF "%VS90COMNTOOLS%" == "" GOTO err_cantsetupvs

:do_process
ECHO Compiling assembler code...
ML64.EXE /c /nologo /Fo"Injector_x64.obj" "Injector_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile

ECHO Coverting to INL...
..\..\MiscUtils\File2Inc\File2Inc.exe Injector_x64.obj Injector_x64.inl /var:aInjectorX64 /type:obj
GOTO end

:show_err
ECHO Please execute this batch file inside a Visual Studio x64 Command Prompt
GOTO end

:err_cantsetupvs
ECHO Cannot initialize Visual Studio x64 Command Prompt environment
GOTO end

:bad_compile
ECHO Errors detected while compiling headers file
GOTO end

:end
ENDLOCAL
