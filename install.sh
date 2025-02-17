#!/bin/bash


./install_deps.sh
if [ $? -ne 0 ]; then
    echo "Failed to install dependencies. Please check the install script."
    exit 1
fi

make

# Create symbolic link
sudo ln -s "$(pwd)/pdf_search" /usr/local/bin/pdf_search