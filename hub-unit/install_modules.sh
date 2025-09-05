# create installtion folder if it does not exist
sudo mkdir -p /usr/bin/doors_hub

# copy all built modules to the installation folder
sudo cp -r build/* /usr/bin/doors_hub/


# stop nginx server if running
sudo nginx -s quit

# copy web interface
sudo cp -r site /usr/bin/doors_hub/
sudo cp nginx_conf /etc/nginx/sites-enabled/doors-hub

sudo nginx
