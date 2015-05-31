@ECHO OFF
SETLOCAL
IF NOT "%VCINSTALLDIR%" == "" GOTO do_process
IF "%VS90COMNTOOLS%" == "" GOTO show_err

ECHO Setting up a Visual Studio x86 Command Prompt environment... 
CALL "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
IF "%VS90COMNTOOLS%" == "" GOTO err_cantsetupvs

:do_process
ECHO Compiling assembler code...
ML.EXE /c /nologo /Fo"CreateJavaVM_x86.obj"         "CreateJavaVM_x86.asm"
ML.EXE /c /nologo /Fo"Native_RaiseCallback_x86.obj" "Native_RaiseCallback_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile

ECHO Coverting to INL...
..\..\..\MiscUtils\File2Inc\File2Inc.exe CreateJavaVM_x86.obj         CreateJavaVM_x86.inl         /var:aCreateJavaVMX86        /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe Native_RaiseCallback_x86.obj Native_RaiseCallback_x86.inl /var:aNativeRaiseCallbackX86 /type:obj
GOTO end

:show_err
ECHO Please execute this batch file inside a Visual Studio x86 Command Prompt
GOTO end

:err_cantsetupvs
ECHO Cannot initialize Visual Studio x86 Command Prompt environment
GOTO end

:bad_compile
ECHO Errors detected while compiling headers file
GOTO end

:end
ENDLOCAL
