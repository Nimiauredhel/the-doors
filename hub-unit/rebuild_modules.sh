sudo mkdir -p /usr/bin/doors_hub

cd hub_control
make strict

cd ../door_manager
make strict

cd ../intercom_server
make strict

cd ..
sudo cp -r build/* /usr/bin/doors_hub/
sudo cp doors_hub.service /etc/systemd/system/doors_hub.service

