sudo systemctl stop doors_hub
sudo mkdir /usr/bin/doors_hub

cd hub_control
make strict

cd ../door_manager
make strict

cd ../intercom_server
make strict

cd ..

sudo cp build/* /usr/bin/doors_hub/
sudo cp doors_hub.service /etc/systemd/system/doors_hub.service
sudo systemctl daemon-reload
sudo systemctl start doors_hub
sudo systemctl enable doors_hub
