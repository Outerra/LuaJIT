@rem Script to build LuaJIT with MSVC.
@rem Copyright (C) 2005-2015 Mike Pall. See Copyright Notice in luajit.h
@rem
@rem Either open a "Visual Studio .NET Command Prompt"
@rem (Note that the Express Edition does not contain an x64 compiler)
@rem -or-
@rem Open a "Windows SDK Command Shell" and set the compiler environment:
@rem     setenv /release /x86
@rem   -or-
@rem     setenv /release /x64
@rem
@rem Then cd to this directory and run this script.

@if not defined INCLUDE goto :FAIL

@setlocal
@cd ..\src
@set LJCOMPILE=cl /nologo /c /O2 /W3 /D_CRT_SECURE_NO_DEPRECATE
@set LJLINK=link /nologo
@set LJMT=mt /nologo
@set LJLIB=lib /nologo /nodefaultlib
@set DASMDIR=..\dynasm
@set DASM=%DASMDIR%\dynasm.lua
@set COMMDIR=..\..
@set LJDLLNAME=luajit.dll
@set LJLIBNAME=luajit.lib
@set LJPDBNAME=luajit.pdb
@set ALL_LIB=lib_base.c lib_math.c lib_bit.c lib_string.c lib_table.c lib_io.c lib_os.c lib_package.c lib_debug.c lib_jit.c lib_ffi.c
@set LJBINDIR=..\bin
@set LJLIBDIR=..\lib
@set LJINCLUDEDIR=..\include\luaJIT
@set LJINCLUDEFILES=(lauxlib.h,lua.h,lua.hpp,luaconf.h,luajit.h,lualib.h,luaext.h)
@set LJDEBUG=0
@set LJSTATIC=0
@set LJAMALG=0
@set ADDITIONAL_INCLUDE=/I"%COMMDIR%"

@if "%1"=="debug" @set LJDEBUG=1 
@if "%1"=="static" @set LJSTATIC=1
@if "%1"=="amalg" @set LJAMALG=1

@if "%2"=="debug" @set LJDEBUG=1 
@if "%2"=="static" @set LJSTATIC=1
@if "%2"=="amalg" @set LJAMALG=1

@if "%3"=="debug" @set LJDEBUG=1 
@if "%3"=="static" @set LJSTATIC=1
@if "%3"=="amalg" @set LJAMALG=1


%LJCOMPILE% host\minilua.c
@if errorlevel 1 goto :BAD
%LJLINK% /out:minilua.exe minilua.obj
@if errorlevel 1 goto :BAD
if exist minilua.exe.manifest^
  %LJMT% -manifest minilua.exe.manifest -outputresource:minilua.exe

@set DASMFLAGS=-D WIN -D JIT -D FFI -D P64
@set LJARCH=x64
@minilua
@if errorlevel 8 goto :X64
@set DASMFLAGS=-D WIN -D JIT -D FFI
@set LJARCH=x86
:X64
minilua %DASM% -LN %DASMFLAGS% -o host\buildvm_arch.h vm_x86.dasc
@if errorlevel 1 goto :BAD

@set LJBINDIR=%LJBINDIR%\%LJARCH%
@set LJLIBDIR=%LJLIBDIR%\%LJARCH%

%LJCOMPILE% /I "." /I %DASMDIR% host\buildvm*.c
@if errorlevel 1 goto :BAD
%LJLINK% /out:buildvm.exe buildvm*.obj
@if errorlevel 1 goto :BAD
if exist buildvm.exe.manifest^
  %LJMT% -manifest buildvm.exe.manifest -outputresource:buildvm.exe

buildvm -m peobj -o lj_vm.obj
@if errorlevel 1 goto :BAD
buildvm -m bcdef -o lj_bcdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m ffdef -o lj_ffdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m libdef -o lj_libdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m recdef -o lj_recdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m vmdef -o jit\vmdef.lua %ALL_LIB%
@if errorlevel 1 goto :BAD
buildvm -m folddef -o lj_folddef.h lj_opt_fold.c
@if errorlevel 1 goto :BAD

@if %LJDEBUG%==1 (
	@set LJCOMPILE=%LJCOMPILE% /Z7 /MTd
	@set LJLINK=%LJLINK% /debug
	@set LJBINDIR=%LJBINDIR%\Debug
	@set LJLIBDIR=%LJLIBDIR%\Debug
) else (
	
	@set LJBINDIR=%LJBINDIR%\Release
	@set LJLIBDIR=%LJLIBDIR%\Release
	@set LJCOMPILE=%LJCOMPILE% /Z7 /MT
)

mkdir %LJBINDIR%
mkdir %LJINCLUDEDIR%

@if %LJAMALG%==1 goto :AMALGDLL
@if %LJSTATIC%==1 goto :STATIC

:DLL
@set LJLIBDIR=%LJBINDIR%
%LJCOMPILE% %ADDITIONAL_INCLUDE% /Fd"%LJLIBDIR%\%LJPDBNAME%" /EHsc /DLUA_BUILD_AS_DLL lj_*.c lib_*.c lib_ext.cpp
@if errorlevel 1 goto :BAD
%LJLINK% /DLL /OUT:%LJBINDIR%\%LJDLLNAME% lj_*.obj lib_*.obj
@if errorlevel 1 goto :BAD
@goto :MTDLL

:STATIC
mkdir %LJLIBDIR%
%LJCOMPILE% %ADDITIONAL_INCLUDE% /Fd"%LJLIBDIR%\%LJPDBNAME%" /EHsc /DLUA_BUILD_AS_LIB lj_*.c lib_*.c lib_ext.cpp
@if errorlevel 1 goto :BAD
%LJLIB% /OUT:%LJLIBDIR%\%LJLIBNAME% lj_*.obj lib_*.obj
@if errorlevel 1 goto :BAD
@goto :MTDLL

:AMALGDLL
%LJCOMPILE% MD /DLUA_BUILD_AS_DLL ljamalg.c
@if errorlevel 1 goto :BAD
%LJLINK% /DLL /out:%LJBINDIR%\%LJDLLNAME% ljamalg.obj lj_vm.obj
@if errorlevel 1 goto :BAD

:MTDLL
if exist %LJDLLNAME%.manifest^
  %LJMT% -manifest %LJDLLNAME%.manifest -outputresource:%LJDLLNAME%;2

%LJCOMPILE% luajit.c
@if errorlevel 1 goto :BAD

%LJLINK% /out:%LJBINDIR%\luajit.exe luajit.obj %LJLIBDIR%\%LJLIBNAME%
@if errorlevel 1 goto :BAD
if exist luajit.exe.manifest^
  %LJMT% -manifest luajit.exe.manifest -outputresource:luajit.exe

@for %%f in %LJINCLUDEFILES% do @copy %%f %LJINCLUDEDIR%
  
@del *.obj *.manifest minilua.exe buildvm.exe
@echo.
@echo === Successfully built LuaJIT for Windows/%LJARCH% ===

@goto :END
:BAD
@echo.
@echo *******************************************************
@echo *** Build FAILED -- Please check the error messages ***
@echo *******************************************************
@goto :END
:FAIL
@echo You must open a "Visual Studio .NET Command Prompt" to run this script
:END
@endlocal