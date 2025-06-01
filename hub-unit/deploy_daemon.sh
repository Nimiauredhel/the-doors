sudo mkdir /usr/bin/doors_hub
sudo cp build/* /usr/bin/doors_hub/
sudo cp doors_hub.service /etc/systemd/system/doors_hub.service
sudo systemctl daemon-reload
sudo systemctl start doors_hub
sudo systemctl enable doors_hub
