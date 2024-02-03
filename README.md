# TO KNOW

Dont move the files into a other location, there is a softlink to these files, into '/usr/local/bin/'. <br>
So i can change the files if I want and dont have to change the links.

### Command to make a link

>[!TIP]
change the *pdf_search* in */usr/local/bin/**pdf_search*** to a command name of your choice, for later use.

```bash
sudo ln -s "$(pwd)/pdf_search" /usr/local/bin/pdf_search
```

### Command to delete the link again

```bash
sudo rm /usr/local/bin/pdf_search
```

## Usage

```bash
pdf_search <word_to_search> [-s] [-r] [-h/--help]
```

### Options

- **<word_to_search>**: The word to search for within PDF files.
- **-s**: Enable case-sensitive search.
- **-r**: Recursively search subdirectories.
- **-h/--help**: Display this usage information.

## Example

Search for "example" in all PDF files in the current directory:

```bash
pdfs example
```

Search for "example" in all PDF files in the current directory and subdirectories:

```bash
pdfs example -r
```

Search for "example" in all PDF files in the current directory with case sensitivity:

```bash
pdfs example -s
```

Search for "example" in all PDF files in the current directory and subdirectories, with case sensitivity:

```bash
pdfs example -s -r
```
