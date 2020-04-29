#include <opencv2/opencv.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <stdio.h>
using namespace cv;
using namespace std;

RNG rng(12345);

int getMaxAreaContourId(vector <vector<cv::Point>> contours) {
    double maxArea = 0;
    int maxAreaContourId = -1;
    for (int j = 0; j < contours.size(); j++) {
        double newArea = cv::contourArea(contours.at(j));
        if (newArea > maxArea) {
            maxArea = newArea;
            maxAreaContourId = j;
        } // End if
    } // End for
    return maxAreaContourId;
} // End function

int main(int, char**)
{
    Mat frame, blurFrame;
    //--- INITIALIZE VIDEOCAPTURE
    VideoCapture cap;
    // open the default camera using default API
    cap.open(0);
    // OR advance usage: select any API backend
    int deviceID = 0;             // 0 = open default camera
    int apiID = cv::CAP_ANY;      // 0 = autodetect default API
    // open selected camera using selected API
    cap.open(deviceID + apiID);
    // check if we succeeded
    if (!cap.isOpened()) {
        cerr << "ERROR! Unable to open camera\n";
        return -1;
    }

    cap.set(CV_CAP_PROP_FRAME_WIDTH,640);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,480);
    
    //--- GRAB AND WRITE LOOP
    cout << "Start grabbing" << endl
        << "Press any key to terminate" << endl;
    for (;;)
    {
        // wait for a new frame from camera and store it into 'frame'
        cap.read(frame);
        // check if we succeeded
        if (frame.empty()) {
            cerr << "ERROR! blank frame grabbed\n";
            break;
        }
        // show live and wait for a key with timeout long enough to show images
        if (waitKey(5) >= 0)
            break;


        GaussianBlur( frame, blurFrame, Size( 11, 11 ), 0, 0 );

        Mat fullImageHSV;
        cvtColor(blurFrame, fullImageHSV, CV_BGR2HSV);

        Mat frame_threshold;
        inRange(fullImageHSV, Scalar(36, 25, 25), Scalar(70, 255, 255), frame_threshold);

        // erode   (   frame_threshold,
        //             frame_threshold,
        //             NULL, //try frame_threshold
        //             Point(-1,-1),
        //             2,
        //             BORDER_CONSTANT,
        //             morphologyDefaultBorderValue() 
        //         );
        erode(frame_threshold, frame_threshold, 0);
        erode(frame_threshold, frame_threshold, 0);
        dilate(frame_threshold, frame_threshold, 0);
        dilate(frame_threshold, frame_threshold, 0);


        // dilate   (  frame_threshold,
        //             frame_threshold,
        //             NULL,
        //             Point(-1,-1),
        //             2,
        //             BORDER_CONSTANT,
        //             morphologyDefaultBorderValue() 
        //         );

        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;

        //was  CV_RETR_TREE
        findContours( frame_threshold, contours, hierarchy, RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

        int largest_area=0;
        int largest_contour_index=0;
        
		Rect bounding_rect, boundRect;
		float radius;
		Point2f center;
		vector<Point> contours_poly;
		

        for( size_t i = 0; i< contours.size(); i++ ) // iterate through each contour.
    	{
        	double area = contourArea( contours[i] );  //  Find the area of contour

        	if( area > largest_area )
        	{
            	largest_area = area;
            	largest_contour_index = i;               //Store the index of largest contour
            	bounding_rect = boundingRect( contours[i] ); // Find the bounding rectangle for biggest contour
				approxPolyDP( contours[largest_contour_index], contours_poly, 3, true );
				boundRect = boundingRect( contours_poly );
				minEnclosingCircle( contours_poly, center, radius );
        	}
    	}

        /// Draw contours
        Mat drawing = Mat::zeros( frame_threshold.size(), CV_8UC3 );
        // for( int i = 0; i< contours.size(); i++ )
        // {
            // Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
            // drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
        // }
// 
        // contours.at(getMaxAreaContourId(contours))

		Scalar color = Scalar( rng.uniform(0, 256), rng.uniform(0,256), rng.uniform(0,256) );
		rectangle( frame, boundRect.tl(), boundRect.br(), color, 2 );
		circle( drawing, center, (int)radius, color, 2 );
		drawContours( drawing, contours,largest_contour_index, Scalar( 0, 255, 0 ), 2 ); // Draw the largest contour using previously stored index.


        /// Show in a window
        namedWindow( "Contours", CV_WINDOW_AUTOSIZE );
        imshow( "Contours", drawing );
        imshow("Live", frame);

    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}
