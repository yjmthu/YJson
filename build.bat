call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -S . -B ./build
ninja -C build

