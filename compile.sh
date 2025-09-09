# Uses cmake and ninja for the main project

cd build
cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
ninja
cd ..

# Then compiles the server with just g++

g++ server.cpp ./lib/txt_writer.c ./lib/cxx_utils.cpp -o server.x86_64
