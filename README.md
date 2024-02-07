# pds_search

## Table of content

- [1. How to start](#1-how-to-start)
  - [1.1. Short Version](#11-short-version)
  - [1.2. Long Version](#12-long-version)
    - [1.2.1. Download the repo](#121-download-the-repo)
    - [1.2.2. Switch to folder](#122-switch-to-folder)
    - [1.2.3. Compile](#123-compile)
  - [1.3. Command to make a link](#13-command-to-make-a-link)
  - [1.4. TO KNOW](#14-to-know)
  - [1.5. Command to delete the link again](#15-command-to-delete-the-link-again)
- [2. Usage](#2-usage)
  - [2.1. Options](#21-options)
- [3. Example](#3-example)

## 1. How to start

### 1.1. Short Version

```bash
git clone https://github.com/MatthiasEbner2002/pdf_search.git
cd pdf_search
make
chmod +x pdf_search
sudo ln -s "$(pwd)/pdf_search" /usr/local/bin/pdf_search # change pdf_search to prefered command name
```

### 1.2. Long Version

#### 1.2.1. Download the repo

```bash
git clone https://github.com/MatthiasEbner2002/pdf_search.git
```

#### 1.2.2. Switch to folder

```bash
cd pdf_search
```

#### 1.2.3. Compile

```bash
make
```

>[!NOTE]
>
> #### Add execution right if needed
>
>```bash
> chmod +x pdf_search
>```

### 1.3. Command to make a link

>[!TIP]
change the *pdf_search* in */usr/local/bin/**pdf_search*** to a command name of your choice, for later use.

```bash
sudo ln -s "$(pwd)/pdf_search" /usr/local/bin/pdf_search
```

### 1.4. TO KNOW

Dont move the files into a other location, there is a softlink to these files, into '/usr/local/bin/'.
So i can change the files if I want and dont have to change the links.

### 1.5. Command to delete the link again

```bash
sudo rm /usr/local/bin/pdf_search
```

## 2. Usage

```bash
pdf_search <word_to_search> [-s] [-r] [-h/--help]
```

### 2.1. Options

- **<word_to_search>**: The word to search for within PDF files.
- **-s**: Enable case-sensitive search.
- **-r**: Recursively search subdirectories.
- **-h/--help**: Display this usage information.

## 3. Example

Search for "example" in all PDF files in the current directory:

```bash
pdfs example
```

Search for "example of this" in all PDF files in the current directory:

```bash
pdfs "example of this"
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
