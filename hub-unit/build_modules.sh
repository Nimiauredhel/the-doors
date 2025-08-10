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
