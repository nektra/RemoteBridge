@ECHO OFF
SETLOCAL
IF NOT "%VCINSTALLDIR%" == "" GOTO do_process
IF "%VS90COMNTOOLS%" == "" GOTO show_err

ECHO Setting up a Visual Studio x86 Command Prompt environment... 
CALL "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
IF "%VS90COMNTOOLS%" == "" GOTO err_cantsetupvs

:do_process
ECHO Compiling assembler code...
ML.EXE /c /nologo /Fo"DllGetClassObject_x86.obj" "DllGetClassObject_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML.EXE /c /nologo /Fo"IClassFactory_CreateInstance_x86.obj" "IClassFactory_CreateInstance_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML.EXE /c /nologo /Fo"IClassFactory2_CreateInstanceLic_x86.obj" "IClassFactory2_CreateInstanceLic_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML.EXE /c /nologo /Fo"IUnknown_QueryInterface_x86.obj" "IUnknown_QueryInterface_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML.EXE /c /nologo /Fo"IUnknown_AddRef_x86.obj" "IUnknown_AddRef_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML.EXE /c /nologo /Fo"IUnknown_Release_x86.obj" "IUnknown_Release_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML.EXE /c /nologo /Fo"IUnknown_Generic_x86.obj" "IUnknown_Generic_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML.EXE /c /nologo /Fo"SHWeakReleaseInterface_x86.obj" "SHWeakReleaseInterface_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML.EXE /c /nologo /Fo"CoUninitialize_x86.obj" "CoUninitialize_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML.EXE /c /nologo /Fo"CoInitializeSecurity_x86.obj" "CoInitializeSecurity_x86.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile

ECHO Coverting to INL...
..\..\..\MiscUtils\File2Inc\File2Inc.exe DllGetClassObject_x86.obj                DllGetClassObject_x86.inl                /var:aDllGetClassObjectX86               /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IClassFactory_CreateInstance_x86.obj     IClassFactory_CreateInstance_x86.inl     /var:aIClassFactoryCreateInstanceX86     /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IClassFactory2_CreateInstanceLic_x86.obj IClassFactory2_CreateInstanceLic_x86.inl /var:aIClassFactory2CreateInstanceLicX86 /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IUnknown_QueryInterface_x86.obj          IUnknown_QueryInterface_x86.inl          /var:aIUnknownQueryInterfaceX86          /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IUnknown_AddRef_x86.obj                  IUnknown_AddRef_x86.inl                  /var:aIUnknownAddRefX86                  /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IUnknown_Release_x86.obj                 IUnknown_Release_x86.inl                 /var:aIUnknownReleaseX86                 /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IUnknown_Generic_x86.obj                 IUnknown_Generic_x86.inl                 /var:aIUnknownGenericX86                 /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe SHWeakReleaseInterface_x86.obj           SHWeakReleaseInterface_x86.inl           /var:aSHWeakReleaseInterfaceX86          /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe CoUninitialize_x86.obj                   CoUninitialize_x86.inl                   /var:aCoUninitializeX86                  /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe CoInitializeSecurity_x86.obj             CoInitializeSecurity_x86.inl             /var:aCoInitializeSecurityX86            /type:obj
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
