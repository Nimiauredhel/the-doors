# clean the hub control (parent) module
cd hub_control
make -j4 clean

# clean the child modules
cd ../door_manager
make -j4 clean

cd ../intercom_server
make -j4 clean

# clean the common header library
cd ../hub_common
sudo make -j4 clean

# return to top folder 
# and delete build folder
cd ..
sudo rm -rf build
