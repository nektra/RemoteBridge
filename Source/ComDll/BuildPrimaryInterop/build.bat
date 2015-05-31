@ECHO OFF
ECHO Creating TLB...
COPY /Y "%~dp0\..\RemoteBridge.idl" "%~dp0\RemoteBridge.idl" >NUL
COPY /Y "%~dp0\..\disp_ids.h" "%~dp0\disp_ids.h" >NUL
MIDL "%~dp0\RemoteBridge.idl" /D "NDEBUG" /nologo /char signed /env win32 /tlb "%~dp0\RemoteBridge.tlb" /h "%~dp0\RemoteBridge_i.h" /dlldata "%~dp0\dlldata.c" /iid "%~dp0\RemoteBridge_i.c" /proxy "%~dp0\RemoteBridge_p.c" /error all /error stub_data /Os
IF NOT %ERRORLEVEL% == 0 goto bad_compile

"%~dp0\..\..\..\Dependencies\TlbImp2\TlbImp2.exe" "%~dp0\RemoteBridge.tlb" /primary "/keyfile:%~dp0\keypair.snk" /transform:dispret "/out:%~dp0\..\..\..\bin\Nektra.RemoteBridge.dll"
IF NOT %ERRORLEVEL% == 0 goto bad_interopbuild

GOTO end

:bad_compile
ECHO Errors detected while compiling IDL file
GOTO end

:bad_interopbuild
ECHO Errors detected while building Primary Interop Assembly
GOTO end

:end
DEL /Q "%~dp0\RemoteBridge*.c" >NUL 2>NUL
DEL /Q "%~dp0\RemoteBridge*.h" >NUL 2>NUL
DEL /Q "%~dp0\dlldata*.c" >NUL 2>NUL
