Meta-Vision-Solais
===

TODO...

## Dependencies
* OpenCV 4
* pugixml
* Boost

## Setup on Jetson Nano
```shell
sudo apt-get install -y libboost-all-dev
```

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
| camera | String | "toggle" | A frame will be sent if the camera opens |
| camera | String | "fetch"  | |
| loadDataSet | String | Data set name  | |

## Core -> Terminal
| Name   | Type   | Argument         |
|--------|--------|------------------|
| message | String | Message to be shown in the status bar |
| cameraInfo | String | Camera info |
| cameraImage | Bytes | JPEG bytes  |
| imageList | ListOfStrings | Image filenames |