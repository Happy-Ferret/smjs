@echo off

if "%1"=="" goto arg-missing

:update
rd /s/q "%~dp0\include"
rd /s/q "%~dp0\src"

xcopy /i/e/q "%1\js\public"                      "%~dp0\include\js"
xcopy /i/e/q "%1\intl\icu\source\common\unicode" "%~dp0\include\unicode"
xcopy /i/e/q "%1\js\src"                         "%~dp0\src\js"
xcopy /i/e/q "%1\js\jsd"                         "%~dp0\src\jsd"
xcopy /i/e/q "%1\mfbt"                           "%~dp0\src\mozilla"

goto end

:arg-missing
echo.
echo Usage:
echo   update-from-mozilla-central ^<path-to-mozilla-central^>
echo.

:end
