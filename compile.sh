# Uses cmake and ninja for the main project

cd build
cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
ninja
cd ..

# Then compiles the server with just g++

g++ src/server.cpp ./src/lib/txt_writer.c ./src/lib/cxx_utils.cpp -o server.x86_64
