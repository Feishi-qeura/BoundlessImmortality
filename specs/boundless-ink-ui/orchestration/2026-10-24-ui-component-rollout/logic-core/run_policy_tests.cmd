@echo off
setlocal
call "D:\VS\IDE\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
pushd "%~dp0"
cl /nologo /EHsc /std:c++17 /W4 /WX policy_tests.cpp /Fe:policy_tests.exe /Fo:policy_tests.obj > policy-build.log 2>&1
if errorlevel 1 (type policy-build.log & popd & exit /b 2)
policy_tests.exe > policy-result.txt
set "test_result=%errorlevel%"
type policy-result.txt
popd
exit /b %test_result%
