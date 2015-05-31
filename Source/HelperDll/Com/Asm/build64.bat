@ECHO OFF
SETLOCAL
IF NOT "%VCINSTALLDIR%" == "" GOTO do_process
IF "%VS90COMNTOOLS%" == "" GOTO show_err

ECHO Setting up a Visual Studio x64 Command Prompt environment... 
CALL "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
IF "%VS90COMNTOOLS%" == "" GOTO err_cantsetupvs

:do_process
ECHO Compiling assembler code...
ML64.EXE /c /nologo /Fo"DllGetClassObject_x64.obj" "DllGetClassObject_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML64.EXE /c /nologo /Fo"IClassFactory_CreateInstance_x64.obj" "IClassFactory_CreateInstance_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML64.EXE /c /nologo /Fo"IClassFactory2_CreateInstanceLic_x64.obj" "IClassFactory2_CreateInstanceLic_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML64.EXE /c /nologo /Fo"IUnknown_QueryInterface_x64.obj" "IUnknown_QueryInterface_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML64.EXE /c /nologo /Fo"IUnknown_AddRef_x64.obj" "IUnknown_AddRef_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML64.EXE /c /nologo /Fo"IUnknown_Release_x64.obj" "IUnknown_Release_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML64.EXE /c /nologo /Fo"IUnknown_Generic_x64.obj" "IUnknown_Generic_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML64.EXE /c /nologo /Fo"SHWeakReleaseInterface_x64.obj" "SHWeakReleaseInterface_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML64.EXE /c /nologo /Fo"CoUninitialize_x64.obj" "CoUninitialize_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile
ML64.EXE /c /nologo /Fo"CoInitializeSecurity_x64.obj" "CoInitializeSecurity_x64.asm"
IF NOT %ERRORLEVEL% == 0 goto bad_compile

ECHO Coverting to INL...
..\..\..\MiscUtils\File2Inc\File2Inc.exe DllGetClassObject_x64.obj                DllGetClassObject_x64.inl                 /var:aDllGetClassObjectX64              /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IClassFactory_CreateInstance_x64.obj     IClassFactory_CreateInstance_x64.inl     /var:aIClassFactoryCreateInstanceX64     /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IClassFactory2_CreateInstanceLic_x64.obj IClassFactory2_CreateInstanceLic_x64.inl /var:aIClassFactory2CreateInstanceLicX64 /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IUnknown_QueryInterface_x64.obj          IUnknown_QueryInterface_x64.inl          /var:aIUnknownQueryInterfaceX64          /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IUnknown_AddRef_x64.obj                  IUnknown_AddRef_x64.inl                  /var:aIUnknownAddRefX64                  /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IUnknown_Release_x64.obj                 IUnknown_Release_x64.inl                 /var:aIUnknownReleaseX64                 /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe IUnknown_Generic_x64.obj                 IUnknown_Generic_x64.inl                 /var:aIUnknownGenericX64                 /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe SHWeakReleaseInterface_x64.obj           SHWeakReleaseInterface_x64.inl           /var:aSHWeakReleaseInterfaceX64          /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe CoUninitialize_x64.obj                   CoUninitialize_x64.inl                   /var:aCoUninitializeX64                  /type:obj
..\..\..\MiscUtils\File2Inc\File2Inc.exe CoInitializeSecurity_x64.obj             CoInitializeSecurity_x64.inl             /var:aCoInitializeSecurityX64            /type:obj
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
