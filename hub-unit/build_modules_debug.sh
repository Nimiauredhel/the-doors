mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
cd ..

cp -r web_server/site build/out_runtime/site
