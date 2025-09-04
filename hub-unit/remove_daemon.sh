sh stop_daemon.sh
sudo rm /etc/systemd/system/doors_hub.service
sudo rm -r /usr/bin/doors_hub
sudo systemctl daemon-reload
