set LIB="C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64/"
zig c++ ./src/main.cpp ./src/tpng.c ./assets/resource.res -L %LIB% -ld3d11 -ld3dcompiler -ldwmapi -lole32 -o .\build\chess.exe