sudo systemctl stop doors_hub
sudo mkdir /usr/bin/doors_hub

cd hub_control
make

cd ../door_manager
make

cd ../intercom_server
make

cd ..

sudo cp build/* /usr/bin/doors_hub/
sudo cp doors_hub.service /etc/systemd/system/doors_hub.service
sudo systemctl daemon-reload
sudo systemctl start doors_hub
sudo systemctl enable doors_hub
