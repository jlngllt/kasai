@echo off

if "%1" == "clean" goto clean

SET DEBUG="yes"
SET GRAPHIC_API="SUKI_D3D11"
::SET GRAPHIC_API="SUKI_D3D9"

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

:: ---
SET BIN=z_fuseki.exe
cl %CFLAGS% %SYMBOLS% -I . -c z_fuseki.c
cl %CFLAGS% %SYMBOLS% -I . -c ..\suki\s_win32.c
cl %CFLAGS% %SYMBOLS% -I . -c ..\suki\s_main.c
cl -nologo -Zi s_win32.obj s_main.obj z_fuseki.obj -Fe%BIN% -link User32.lib %GRAPHIC_LIB%
:: ---

goto eof

:clean
del /f /s %BIN%
del /f /s *.pdb
del /f /s *.obj
del /f /s *.ilk
goto eof

:eof
:: reset variable
SET SYMBOLS=
SET DEBUG=
SET BIN=
SET CFLAGS=
