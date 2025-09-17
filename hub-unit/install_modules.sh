# create installtion folder if it does not exist
sudo mkdir -p /usr/bin/doors_hub

# copy all built modules to the installation folder
sudo cp -r build/* /usr/bin/doors_hub/
