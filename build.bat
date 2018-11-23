set BUILDDIR=build

cd /d %~dp0
@echo off
for /l %%n in (1,1,10) do (
	if exist %BUILDDIR% (
		rmdir /s /q %BUILDDIR%
	)
)
@echo on

set ARCH=Win32 x64
for %%i in (%ARCH%) do (
	cd /d %~dp0
	mkdir %BUILDDIR%\%%i
	cd    %BUILDDIR%\%%i

	cmake -A %%i %~dp0
	cmake --build . --config Release
	cmake --build . --config Debug
	cd /d %~dp0
)
