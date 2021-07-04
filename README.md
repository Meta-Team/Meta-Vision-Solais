Meta-Vision-Solais
===

=> Please make sure to read [Project Wiki](https://github.com/Meta-Team/Meta-Vision-Solais/wiki) :smiley:

# Dependencies
* CMake >= 3.10
* OpenCV 4
* Boost
* pugixml
* ZBar (for ArmorSolverUnitTest)

# Setup on Jetson Nano (Ubuntu)

Ubuntu 18.04 for Jetson Nano has OpenCV 4.1.1 pre-installed.

## Install or Upgrade CMake
```shell
# Remove the existing version
sudo apt remove --purge cmake

# If snap fails to download, consider using proxy
sudo snap set system proxy.http="http://<ip>:<port>"
sudo snap set system proxy.https="http://<ip>:<port>"

# Install
sudo snap install cmake --classic
cmake --version
```

cmake installed by snap is at `/snap/bin`, which is not in PATH by default. To add it to PATH:
```shell
echo 'export PATH="/snap/bin:$PATH"' >> ~/.bashrc
echo 'export PATH="/snap/bin:$PATH"' >> ~/.zshrc
```

## Install Boost
The Boost library from apt-get of Ubuntu 18.04 is too old. Building from source can be time-consuming as Jetson Nano 
doesn't have powerful CPU. Instead, install newer Boost from third-party source.
```shell
sudo add-apt-repository ppa:mhier/libboost-latest
```

```shell
sudo apt-get update
sudo apt install libboost1.74-dev
```

_Note: this package only provides dynamic libraries. Do not use `set(Boost_USE_STATIC_LIBS ON)`._

_Note: tried install Boost 1.76 from source but encountered `boost::filesystem::status: Function not implemented`..._

## Install Protocol Buffer
```shell
sudo snap install protobuf --classic
```

## Install pugixml
The pugixml from apt-get can't be found by cmake, so manual installation is required. You may use the latest package in 
the following commands.
```shell
wget https://github.com/zeux/pugixml/releases/download/v1.11.4/pugixml-1.11.4.tar.gz
tar -xzf pugixml-1.11.4.tar.gz
cd pugixml-1.11.4
mkdir build
cd build
cmake ..
make -j4
sudo make install
```

## Install ZBar
```shell
sudo apt-get install libzbar-dev libzbar0
```

# Setup on macOS

```shell
brew install cmake opencv boost pugixml zbar protobuf
```

# CMake Options for Jetson Nano (Ubuntu)
```shell
-DCMAKE_PREFIX_PATH=/snap/protobuf/current
```

# CMake Options
TODO...

# Design Ideas
* Near-zero overhead for terminal-related code in Solais Core
  * Solais Core never sends actively
  * Result images, frame rates, parameters and other data are fetched by Solais Terminal
* AimingSolver should always use time stamp from Source and should never call chrono::now().


# Packages

`NameOnly` is `Bytes` with empty content.

## Terminal -> Core
| Name   | Type   | Argument         |Description| Note |
|--------|--------|------------------|----|----|
| fetch | NameOnly |  | Fetch result | |
| stop | NameOnly | | Stop execution | |
| fps | NameOnly | | Fetch frame processed in each components | See reply fps package below |
| switchParamSet | String | ParamSet name | | |
| setParams | Bytes | ParamSet | | |
| getParams | NameOnly | | Fetch current params | |
| getCurrentParamSetName | NameOnly | | | |
| reloadLists | NameOnly | | Reload at core  | Need to fetch manually |
| fetchLists | NameOnly | | Fetch data set list and parameter set list | |
| switchImageSet | String | Path of the data set | | |
| runCamera | Bytes | nullptr | Start execution on camera | |
| runImage | String | Image Name | | Result sent back automatically |
| runImageSet | NameOnly |  | | Use current ImageSet set by switchImageSet |
| previewImage | String | Video file name | Fetch the first frame of the specific video | Result package sent by Core |
| runVideo | String | Video file name | | |
| captureImage | NameOnly | | Capture camera image and save to file | Require manual reload of the image list |
| startRecord  | NameOnly | | Start recording video from camera | |
| stopRecord  | NameOnly | | Stop recording video from camera | |


## Core -> Terminal
| Name   | Type   | Argument         | Note |
|--------|--------|------------------| ---- |
| msg | String | Message to be shown in the status bar | |
| res | Bytes | Result protobuf message | |
| executionStarted | String | "camera"/"image <filename>"/"image set"/"recording <filename>" | Allow Terminal to start fetching |
| fps | ListOfStrings | Frame processed in Input and Executor since last fetch, each number as a string | |
| params | Bytes | Current params | |
| imageList | ListOfStrings | Image names | |
| imageSetList | ListOfStrings | Data set names | |
| videoList | ListOfStrings | Video names | |
| currentParamSetName | String | | |
 
NameOnly res package (size of 0) is sent if the Executor is not running. Terminal then holds the fetch command.
