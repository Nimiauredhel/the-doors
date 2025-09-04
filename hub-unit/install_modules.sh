# create installtion folder if it does not exist
sudo mkdir -p /usr/bin/doors_hub

# copy all built modules to the installation folder
sudo cp -r build/* /usr/bin/doors_hub/

# stop nginx server if running
sudo nginx -s stop

# copy web interface
sudo cp -r site /usr/bin/doors_hub/

confpath="$(pwd)/nginx.conf"
# set nginx profile
sudo nginx -c "$confpath"
