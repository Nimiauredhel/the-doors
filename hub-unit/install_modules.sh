# create installtion folder if it does not exist
sudo mkdir -p /usr/bin/doors_hub

# copy all built modules to the installation folder
sudo cp -r build/out_runtime/* /usr/bin/doors_hub/

sudo setcap 'CAP_NET_BIND_SERVICE=ep' /usr/bin/doors_hub/intercom_server
sudo setcap 'CAP_NET_BIND_SERVICE=ep' /usr/bin/doors_hub/web_server

# copy library to library folder and refresh
sudo cp -r build/out_library/* /usr/lib/
sudo ldconfig
