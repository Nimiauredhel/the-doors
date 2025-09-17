# build the hub control (parent) module
cd hub_control
make -j4 strict

# build the child modules
cd ../door_manager
make -j4 strict

cd ../intercom_server
make -j4 strict

cd ../web_server
make -j4 strict

cd ../db_service
make -j4 strict

# return to top folder 
cd ..
