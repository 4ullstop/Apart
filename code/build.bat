@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

set commonCompilerFlags=-nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DAPART_INTERNAL=1 -DAPART_SLOW=1 -DAPART_WIN32=1 -FC -Zi

set commonLinkerFlags= -incremental:no user32.lib gdi32.lib winmm.lib

REM 32-bit build
REM cl %commonCompilerFlags% ..\code\win32_handmade.cpp %commonLinkerFlags%

cl %commonCompilerFlags% ..\code\apart.cpp -Fmpractice.map /LD /link /EXPORT:GameGetSoundData /EXPORT:GameUpdateAndRender
cl %commonCompilerFlags% ..\code\win32_apart.cpp -Fmwin32_learn.map /link %commonLinkerFlags%
popd
