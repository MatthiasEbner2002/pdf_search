# TO KNOW

Dont move the files into a other location, there is a softlink to these files, into '/usr/local/bin/'. <br>
So i can change the files if I want and dont have to change the links. 

### Command to make a link:
    sudo ln -s "$(pwd)/pdf_search_file" /usr/local/bin/pdf_search_file

    sudo ln -s "$(pwd)/pdf_search" /usr/local/bin/pdf_search

###  Command to delete the link again:
    sudo rm /usr/local/bin/pdf_search

    sudo rm /usr/local/bin/pdf_search_file


TODO: make it possible to have a strict search (case sensitive)