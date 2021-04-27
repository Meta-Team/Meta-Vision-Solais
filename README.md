Meta-Vision-Solais
===

## Dependencies
* CMake >= 3.10
* OpenCV 4
* Boost
* pugixml

## Setup on Jetson Nano

Ubuntu 18.04 for Jetson Nano has OpenCV 4.1.1 pre-installed.

### Install Boost
The Boost library from apt-get of Ubuntu 18.04 is too old. Building from source can be time-consuming as Jetson Nano 
doesn't have powerful CPU. Instead, install newer Boost from other source.
```shell
sudo add-apt-repository ppa:mhier/libboost-latest
sudo apt-get update
sudo apt install -y libboost1.74-dev
```

If the error `dpkg-deb: error: paste subprocess was killed by signal (Broken pipe)` occurs, run the following commands.

```shell
sudo dpkg -i --force-overwrite /var/cache/apt/archives/libboost1.74-dev_1.74-0~16~ubuntu18.04.1_arm64.deb
sudo apt install -f libboost1.74-dev
```

### Install pugixml
The pugixml from apt-get can't be found by cmake, so manual installation is required. You may use the latest package in 
the following commands.
```shell
wget https://github.com/zeux/pugixml/releases/download/v1.11.4/pugixml-1.11.4.tar.gz
tar -xzf pugixml-1.11.4.tar.gz
cd pugixml-1.11.4
mkdir build
cd build
cmake ..
make
sudo make install
```

## Packages
## Terminal -> Core
| Name   | Type   | Argument         |Note|
|--------|--------|------------------|----|
| fetch | Bytes | nullptr  | Fetch result |
| stop | Bytes | nullptr | Stop execution |
| runCamera | Bytes | nullptr | Start execution on camera |
| fps | Bytes | nullptr | Fetch frame processed |


## Core -> Terminal
| Name   | Type   | Argument         |
|--------|--------|------------------|
| msg | String | Message to be shown in the status bar |
| res | Bytes | Result protobuf message |
| fps | Int | Frame processed since last fetch |
