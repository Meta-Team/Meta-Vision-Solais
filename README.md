Meta-Vision-Solais
===

TODO...

# Dependencies
* OpenCV 4
* pugixml
* Boost

# Packages
## Terminal -> Core
| Name   | Type   | Argument         |Note|
|--------|--------|------------------|----|
| camera | String | "toggle" | A frame will be sent if camera is opened |
|        |        | "fetch"  | |

## Core -> Terminal
| Name   | Type   | Argument         |
|--------|--------|------------------|
| message | String | Message to be shown in the status bar |
| cameraInfo | String | Camera info |
| cameraImage | Bytes | JPEG bytes  |