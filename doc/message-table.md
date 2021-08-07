Messages between Solais Core and Terminal
===

See [TerminalSocket.h](include/TerminalSocket.h) for the four types of packages.

`NameOnly` is `Bytes` with empty content.

## Terminal -> Core
| Name   | Type   | Argument         |Description| Note |
|--------|--------|------------------|----|----|
| fetch | String | Four characters of 'T' or 'F' for images of camera, brightness, color, and contours  | Fetch result | |
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
| res | Bytes | Result protobuf message | NameOnly res package (size of 0) is sent if the Executor is not running. Terminal then holds the fetch command |
| executionStarted | String | "camera"/"image <filename>"/"image set"/"recording <filename>" | Allow Terminal to start fetching |
| fps | ListOfStrings | Frame processed in Input and Executor since last fetch, each number as a string | |
| params | Bytes | Current params | |
| imageList | ListOfStrings | Image names | |
| imageSetList | ListOfStrings | Data set names | |
| videoList | ListOfStrings | Video names | |
| currentParamSetName | String | | |


