This repository is archived, please visit [Meta-Vision-SolaisNG](https://github.com/Meta-Team/Meta-Vision-SolaisNG).

Meta-Vision-Solais
===

=> Please make sure to visit [Project Wiki](https://github.com/Meta-Team/Meta-Vision-Solais/wiki) :smiley:

_Solais_ means "light" in Irish. [Claíomh Solais](https://en.wikipedia.org/wiki/Cla%C3%ADomh_Solais), 
"Sword of Light" or "Shining Sword,"  is a weapon that 
appears in Irish and Scottish Gaelic folktales, reputedly as an Undefeatable Sword such that once unsheathed, 
no one could escape its blows.

Also, we Vision group mainly deal with lights in the images :)

# Dependencies
* CMake >= 3.10
* OpenCV 4
* Boost
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
```

CMake installed by Snap is at `/snap/bin`, which is not in PATH by default. To add it to PATH:
```shell
echo 'export PATH="/snap/bin:$PATH"' >> ~/.bashrc
echo 'export PATH="/snap/bin:$PATH"' >> ~/.zshrc
```

To check CMake's version:

```shell
cmake --version
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

## Install ZBar
```shell
sudo apt-get install libzbar-dev libzbar0
```

# Setup on macOS

```shell
brew install cmake opencv boost zbar protobuf
```

# CMake Build System
This project uses CMake build system. The main design idea is to make dependencies flexible: if dependencies of a
target cannot be fully satisfied, the target is not added but no error is raised, so that other targets may still
be able to build.

Therefore, we don't need to install Qt (required by SolaisTerminal) on Jetson Nano, neither
OpenCV (required by Solais Core) on the PC that only needs to run SolaisTerminal.

The two main targets are Solais Core (Solais) and Terminal (SolaisTerminal). The others are shared components, 
tools and unit tests.

## CMake Options for Jetson Nano (Ubuntu)
To let CMake find ProtoBuf installed by Snap, the installation path needs to be supplied manually:
```
-DCMAKE_PREFIX_PATH=/snap/protobuf/current
```

# Other CMake Options
The following option specify Qt install path (required to let CMake find Qt):
```
-DQt5_DIR="/usr/local/Cellar/qt@5/5.15.2/lib/cmake/Qt5"
```

`PARAM_SET_ROOT` and `DATA_SET_ROOT` define the directory paths of parameter sets and data sets. By default, they
are both `<Project Directory>/data`, and therefore:

```
<Project Directory>/data/params: parameter files
<Project Directory>/data/params_backup: backup of parameter files on every save
<Project Directory>/data/videos: video files
<Project Directory>/data/images/<Image Set Folder>: image files
```

To disable Serial (for example, to test Solais locally):

```
-DSERIAL_DEVICE=""
```

# Design Idea: Core-Terminal Co-Design from the Start

One of the difficulties of Vision is tuning and testing. Hard-coded parameters are unacceptable, as every change
requires a re-compile. 

On the other hand, results of each processing step need to be visualized. With the typical 
approach used by most teams: OpenCV's imshow, images can only be shown on Jetson's desktop and therefore a screen is still
required.

Therefore, from the start of this project, a Terminal for tuning and testing is co-designed with the Core:
* Protocol Buffer is used for parameters and results.
* Core communicates with Terminal completely through TCP with the customized protocol ([TerminalSocket.h](include/TerminalSocket.h))
* Terminal UI are automatically generated from .proto file at compile time (with [GeneratePhaseUI.py](tools/SolaisTerminal/GeneratePhaseUI.py)).
Every time to add a parameter, all we need is:
  * Change [Parameters.proto](src/Parameters.proto)
  * Add a default value in [ParamSetManager.cpp](src/ParamSetManager.cpp)
  * Add a default value in each existing json file of parameters (otherwise the error of missing fields will arise)
  * Use the parameter in the code
  * There is nothing to do with Terminal UI file.
* Terminal-related code should have zero overhead in Core when the Terminal is not attached.
  * Core never sends results actively. Result images, frame rates, parameters and other data are fetched by Terminal.

[doc/message-table.md](doc/message-table.md) describes the list of messages exchanged between the Core and the Terminal.
Make sure to update it whenever a new message is added.
