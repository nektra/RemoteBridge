@ECHO OFF
SETLOCAL
SET JDKPATH=%~dp0\..\..\..\..\..\Dependencies\JDK
IF NOT EXIST "%JDKPATH%\bin\javac.exe" (
    ECHO Error: Java compiler not found
    GOTO end
)

ECHO Compiling java code...
DEL *.class /S /Q /F >NUL 2>NUL

"%JDKPATH%\bin\javac.exe" -d "%~dp0\.." InjectTestWithCallbacks.java
IF ERRORLEVEL 1 (
    ECHO Error: While compiling java code
    GOTO end
)

FOR %%a IN (Java*.java) DO (
    "%JDKPATH%\bin\javac.exe" %%a
    IF ERRORLEVEL 1 (
        ECHO Error: While compiling java code
        GOTO end
    )
)

ECHO Packaging...
"%JDKPATH%\bin\jar.exe" cvmf manifest.mf ..\JavaTest.jar Java*.class
IF ERRORLEVEL 1 (
    ECHO Error: While packaging java classes
    GOTO end
)

:end
ENDLOCAL
