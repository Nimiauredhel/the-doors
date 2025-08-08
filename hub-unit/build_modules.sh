# build the hub control (parent) module
cd hub_control
make strict

# build the child modules
cd ../door_manager
make strict

cd ../intercom_server
make strict

# return to top folder 
cd ..

# create daemon folder if it does not exist
sudo mkdir -p /usr/bin/doors_hub

# copy all built modules to the daemon folder
sudo cp -r build/* /usr/bin/doors_hub/

