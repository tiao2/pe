@if "%DEBUG%" == "" @echo off
set DIRNAME=%~dp0
set APP_HOME=%DIRNAME%
set JAVA_EXE=java.exe
if not defined JAVA_HOME (
  echo JAVA_HOME is not set.
  goto fail
)
%JAVA_EXE% -cp "%APP_HOME%\gradle\wrapper\gradle-wrapper.jar" org.gradle.wrapper.GradleWrapperMain %*
:fail
endlocal
