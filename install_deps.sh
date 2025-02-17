#!/bin/sh
if ! pkg-config --exists poppler-glib; then
    echo "poppler-glib is missing. Installing it..."
    if [ -x "$(command -v apt)" ]; then
        sudo apt update
        sudo apt install -y libpoppler-glib-dev
    elif [ -x "$(command -v pacman)" ]; then
        sudo pacman -Syu --noconfirm poppler-glib
    elif [ -x "$(command -v dnf)" ]; then
        sudo dnf install -y poppler-glib-devel
    elif [ -x "$(command -v brew)" ]; then
        brew install poppler
    else
        echo "Package manager not found. Please install poppler-glib manually."
        exit 1
    fi

    if ! pkg-config --exists poppler-glib; then
        echo "Failed to install poppler-glib. Please install it manually."
        exit 1
    fi
fi

echo "All dependencies are installed!"
