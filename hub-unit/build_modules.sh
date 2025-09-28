mkdir -p build
cd build
cmake ..
cmake --build .
cd ..

cp -r web_server/site build/out_runtime/site
