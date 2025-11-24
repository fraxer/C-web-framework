---
outline: deep
description: Step-by-step C Web Framework installation on Linux. Setting up GCC, CMake, OpenSSL, PCRE, Zlib and connecting PostgreSQL, MySQL, Redis.
---

# Install dependencies

The framework requires the following components:

**Required:** GCC 9.5+, CMake 3.12+, OpenSSL 1.1.1+, PCRE 8.43, Zlib 1.2.11, LibXml2 2.9.13

**Optional:** PostgreSQL, MySQL, Redis — install if you need to work with the corresponding databases.

Start by updating the package list:

```bash
sudo apt update
```

## GCC

Install the build-essential package by typing:

```bash
sudo apt install build-essential
```

The command installs many new packages, including `gcc`, `g++` and `make`

To verify that the GCC compiler has been successfully installed, use the gcc --version command, which displays the GCC version.

```bash
gcc --version
```

GCC is now installed on your system and you can start using it.

## Cmake

### Package Manager

Installing cmake from the official repositories is done with the command:

```bash
sudo apt install cmake
```

### Build from source files

Download the archive from the official site:

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.27.0-rc3/cmake-3.27.0-rc3.tar.gz
```

Unpack:

```bash
tar -zxvf cmake-3.27.0-rc3.tar.gz
```

Change to the unpacked directory:

```bash
cd cmake-3.27.0-rc3
```

Start the build process

```bash
./bootstrap
```

Start the installation process

```bash
make
```

Copy the compiled files to the appropriate locations

```bash
make install
```


## PCRE

Installing pcre from the official repositories is done with the command:

```bash
sudo apt install libpcre3-dev
```

## Zlib

### Package Manager

Installing zlib from the official repositories is done with the command:

```bash
sudo apt install zlib1g-dev
```

### Building from source files

Download the archive from the official site:

```bash
wget https://zlib.net/zlib-1.2.13.tar.gz
```

Unpack:

```bash
tar -zxvf zlib-1.2.13.tar.gz
```

Change to the unpacked directory:

```bash
cd zlib-1.2.13
```

Start the build process

```bash
./configure
```

Start the installation process

```bash
make
```

Copy the compiled files to the appropriate locations

```bash
make install
```

## OpenSSL

Installing openssl from the official repositories is done with the command:

```bash
sudo apt install openssl libssl-dev
```

## LibXml2

### Package Manager

Installing libxml2 from the official repositories is done with the command:

```bash
sudo apt install libxml2-dev
```

### Building from source files

Download the archive from the official site:

```bash
wget https://github.com/GNOME/libxml2/releases/download/v2.9.13/libxml2-2.9.13.tar.gz
```

Unpack:

```bash
tar -zxvf libxml2-2.9.13.tar.gz
```

Change to the unpacked directory:

```bash
cd libxml2-2.9.13
```

Start the build process

```bash
./configure
```

Start the installation process

```bash
make
```

Copy the compiled files to the appropriate locations

```bash
make install
```

## MySQL

```bash
sudo apt install mysql-server
```

Detailed instructions are available on [DigitalOcean](https://www.digitalocean.com/community/tutorials/how-to-install-mysql-on-ubuntu-20-04)

## PostgreSQL

```bash
sudo apt install postgresql postgresql-contrib
```

Detailed instructions are available on [DigitalOcean](https://www.digitalocean.com/community/tutorials/how-to-install-postgresql-on-ubuntu-20-04-quickstart)

## Redis

Add the repository to the apt index:

```bash
curl -fsSL https://packages.redis.io/gpg | sudo gpg --dearmor -o /usr/share/keyrings/redis-archive-keyring.gpg

echo "deb [signed-by=/usr/share/keyrings/redis-archive-keyring.gpg] https://packages.redis.io/deb $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/redis.list
```

Then install:

```bash
sudo apt-get install redis
```

Detailed instructions are available on [Redis](https://redis.io/docs/getting-started/installation/install-redis-on-linux/)