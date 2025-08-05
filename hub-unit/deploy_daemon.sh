sudo systemctl stop doors_hub

sh rebuild_modules.sh

sudo systemctl daemon-reload
sudo systemctl start doors_hub
sudo systemctl enable doors_hub
