/*
 * Created by liuzikai on 5/22/21.
 * Reference: https://blog.csdn.net/u010750137/article/details/98457477
 *
 * Require ../../../data/params/<width>x<height>.xml for camera parameters
 */

#include <iostream>
#include <array>
#include <opencv2/opencv.hpp>
#include <zbar.h>
#include "ArmorSolver.h"

using namespace cv;
using namespace std;
using namespace zbar;
using namespace meta;

const int cameraIndex = 1;
const int cameraWidth = 1280;
const int cameraHeight = 720;
const int cameraFPS = 120;
const float armorWidth = 100;
const float armorHeight = 100;

typedef struct {
    string type;
    string data;
    vector<Point> location;
} decodedObject;

// Find and decode barcodes and QR codes
void decode(Mat &im, vector<decodedObject> &decodedObjects) {

    // Create zbar scanner
    ImageScanner scanner;

    // Configure scanner
    scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);

    // Convert image to grayscale
    Mat imGray;
    cvtColor(im, imGray, COLOR_BGR2GRAY);

    // Wrap image data in a zbar image
    Image image(im.cols, im.rows, "Y800", (uchar *) imGray.data, im.cols * im.rows);

    // Scan the image for barcodes and QRCodes
    int n = scanner.scan(image);

    // Print results
    for (Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
        decodedObject obj;

        obj.type = symbol->get_type_name();
        obj.data = symbol->get_data();

        // Print type and data
//        cout << "Type : " << obj.type << endl;
//        cout << "Data : " << obj.data << endl << endl;

        // Obtain location
        for (int i = 0; i < symbol->get_location_size(); i++) {
            obj.location.emplace_back(symbol->get_location_x(i), symbol->get_location_y(i));
        }

        decodedObjects.push_back(obj);
    }
}

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

    Mat cameraMatrix;
    Mat distCoeffs;
    float zScale;

    FileStorage fs("../../data/params/" + std::to_string(cameraWidth) + "x" + std::to_string(cameraHeight) + ".xml",
                   FileStorage::READ);
    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;
    fs["zScale"] >> zScale;


    ArmorSolver solver(armorWidth, armorHeight, cameraMatrix, distCoeffs, zScale);

    Mat img;
    vector<vector<Point> > squares;

    while (waitKey(1) != 'q') {
        inputVideo >> img;
        Mat demo;
        Mat gray;
        Mat edge;
        // Variable for decoded objects
        vector<decodedObject> decodedObjects;

        // Find and decode barcodes and QR codes
        decode(img, decodedObjects);

        // Loop over all decoded objects
        for (int i = 0; i < decodedObjects.size(); i++) {
            vector<Point> points = decodedObjects[i].location;
            vector<Point> hull;

            // If the points do not form a quad, find convex hull
            if (points.size() > 4) {
                convexHull(points, hull);
            } else {
                hull = points;
            }
            array<Point2f, 4> pnts;

            // Number of points in the convex hull
            int n = hull.size();

            if (n == 4) {
                for (int j = 0; j < n; j++) {
                    line(img, hull[j], hull[(j + 1) % n], Scalar(255, 0, 0), 3);
                }

                pnts[0] = Point2f(hull[1].x, hull[1].y);
                pnts[1] = Point2f(hull[0].x, hull[0].y);
                pnts[2] = Point2f(hull[3].x, hull[3].y);
                pnts[3] = Point2f(hull[2].x, hull[2].y);

                auto offset = solver.solve(pnts);
                cout << "X " << offset[0] << "    Y " << offset[1] << "    Z " << offset[2] << endl;
            }
        }

        resize(img, demo, Size(960, 540));
        imshow("demo", demo);
    }
    return 0;
}
