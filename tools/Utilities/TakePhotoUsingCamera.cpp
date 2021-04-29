//
// Created by liuzikai on 4/16/20.
// Reference: https://blog.csdn.net/Loser__Wang/article/details/51811347
// Usage:
// * Set camera_ parameters and outputPath below
// * Run the program
// * Press Q to take a photo
// * Press ESC to exit
//

#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

const string outputPath = "/Users/liuzikai/Documents/RoboMaster/Meta-Vision-II/data";
const int cameraIndex = 1;
const int cameraWidth = 640;
const int cameraHeight = 360;
const int cameraFPS = 330;

int main() {


    VideoCapture inputVideo(cameraIndex);

    if (!inputVideo.isOpened()) {
        cout << "Could not open the input video " << endl;
        return -1;
    }

    inputVideo.set(CAP_PROP_FRAME_WIDTH, cameraWidth);
    inputVideo.set(CAP_PROP_FRAME_HEIGHT, cameraHeight);
    inputVideo.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));
    inputVideo.set(CAP_PROP_FPS, cameraFPS);

    cout << inputVideo.get(CAP_PROP_FRAME_WIDTH) << "x"
         << inputVideo.get(CAP_PROP_FRAME_HEIGHT) << "@"
         << inputVideo.get(CAP_PROP_FPS);

    Mat frame;
    string imgname;
    int f = 1;
    while (true) {
        inputVideo >> frame;
        if (frame.empty()) break;
        imshow("Camera", frame);
        char key = waitKey(1);
        if (key == 27) break;
        if (key == 'q' || key == 'Q') {
            imgname = outputPath + "/" + to_string(f++) + ".jpg";
            imwrite(imgname, frame);
        }
    }
    cout << "Finished writing" << endl;
    return 0;
}