mkdir /home/debian/exe
cp build/* /home/debian/exe
cp doors_hub.service /etc/systemd/system/doors_hub.service
sudo systemctl start doors_hub
sudo systemctl enable doors_hub
