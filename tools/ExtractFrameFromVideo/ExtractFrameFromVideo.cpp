//
// Created by liuzikai on 4/7/20.
//
// A Tool to extract frames from video and save as jpg files.
// Usage:
// * Set video filename and output name below
// * Run the program
// * Press S to save current frame
// * Press ESC to exit
// * Press any other key to next frame
//

#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;
using namespace cv;

const string videoFilename = "/Users/liuzikai/Desktop/Screen Recording 2020-05-06 at 10.31.27.mov";
const string outputName = "/Users/liuzikai/Desktop/boxhead3";
// Actual output image: outputName + "-" + index + ".jpg"

int main() {

    VideoCapture cap(videoFilename);

    if (!cap.isOpened()) {
        cout << "Error opening video stream" << endl;
        return -1;
    }

    Mat frame;
    int cnt = 0;

    while (true) {

        cap >> frame;

        if (frame.empty()) break;

        imshow("Frame", frame);

        char c = (char) waitKey(0);
        if (c == 27) {  // ESC
            break;
        } else if (c == 115) {  // S
            imwrite(outputName + "-" + to_string(cnt) + ".jpg", frame);
            cout << "Frame " << cnt << " saved " << endl;
        }

        cnt++;
    }

    // When everything done, release the video capture and write object
    cap.release();

    // Closes all the windows
    destroyAllWindows();
    return 0;
}