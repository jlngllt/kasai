@echo off

if "%1" == "clean" goto clean

SET DEBUG="yes"
SET GRAPHIC_API="SUKI_D3D11"
::SET GRAPHIC_API="SUKI_D3D9"

SET BIN=s_test.exe
SET CFLAGS=-nologo -W4 -Zi

if %GRAPHIC_API% == "SUKI_D3D9" (
    SET GRAPHIC_LIB=d3d9.lib
    SET SYMBOLS=%SYMBOLS% -DSUKI_D3D9
    echo Build for D3D9
    )

if %GRAPHIC_API% == "SUKI_D3D11" (
    SET GRAPHIC_LIB=d3d11.lib d3dcompiler.lib dxgi.lib dxguid.lib
    SET SYMBOLS=%SYMBOLS% -DSUKI_D3D11
    echo Build for D3D11
    )

if %DEBUG% == "yes" (
    SET CFLAGS=%CFLAGS% -MTd
    )

if %DEBUG% == "no" (
    SET CFLAGS=%CFLAGS% -MD
    )

del *.pdb > NUL 2> NUL

cl %CFLAGS% %SYMBOLS% -I . -c s_win32.c
cl %CFLAGS% %SYMBOLS% -I . -c s_main.c
cl %CFLAGS% %SYMBOLS% -I . -c s_test.c
cl -nologo -Zi s_win32.obj s_main.obj s_test.obj -Fe%BIN% -link User32.lib %GRAPHIC_LIB%
goto eof

:clean
del /f /s %BIN%
del /f /s *.pdb
del /f /s *.obj
goto eof

:eof
:: reset variable
SET SYMBOLS=
SET DEBUG=
SET BIN=
SET CFLAGS=
