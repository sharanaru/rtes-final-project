/*
 *
 *  Example by Sam Siewert 
 *
 *  Updated 10/29/16 for OpenCV 3.1
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define NUMFRAMES 100
#define MATHFRAMES 100

using namespace cv;
using namespace std;

#define HRES 640
#define VRES 480

#define BILLION  1000000000.0

bool threadEnd = false;


// Transform display window
char timg_window_name[] = "Edge Detector Transform";

int lowThreshold=0;
int const max_lowThreshold = 100;
int kernel_size = 3;
int edgeThresh = 1;
int ratio = 3;
Mat canny_frame, cdst, timg_gray, timg_grad;
Mat gray;
vector<Vec4i> lines;

IplImage* frame;
time_t startTime, endTime;
CvCapture* capture;

struct execTime{
    struct timespec t1;
    struct timespec t2;
};

vector<double> fps;

static struct execTime exec;
pthread_mutex_t lock; 
pthread_mutex_t lock1; 
bool newVal = false;


void CannyThreshold(int, void*)
{
    //Mat mat_frame(frame);
    Mat mat_frame(cvarrToMat(frame));

    cvtColor(mat_frame, timg_gray, CV_RGB2GRAY);

    /// Reduce noise with a kernel 3x3
    blur( timg_gray, canny_frame, Size(3,3) );

    /// Canny detector
    Canny( canny_frame, canny_frame, lowThreshold, lowThreshold*ratio, kernel_size );

    /// Using Canny's output as a mask, we display our result
    timg_grad = Scalar::all(0);

    mat_frame.copyTo( timg_grad, canny_frame);

    imshow( timg_window_name, timg_grad );
}

void *cannyTransform(void *in)
{
    while(1)
    {
        struct timespec start, end;

        clock_gettime(CLOCK_REALTIME, &start);
        frame=cvQueryFrame(capture);
        if(!frame) break;

        CannyThreshold(0, 0);
        char q = cvWaitKey(33);
        if( q == 'q' )
        {
            printf("got quit\n");
            threadEnd = true;
            break;
        }
        clock_gettime(CLOCK_REALTIME, &end);

        pthread_mutex_lock(&lock); 
        exec.t1 = end;
        exec.t2 = start;
        newVal = true;
        pthread_mutex_unlock(&lock); 
    }
    threadEnd = true;
}

void *houghThread(void *in)
{
    while(1)
    {
        struct timespec start, end;

        clock_gettime(CLOCK_REALTIME, &start);
        frame=cvQueryFrame(capture);

        Mat mat_frame(cvarrToMat(frame));
        Canny(mat_frame, canny_frame, 50, 200, 3);

        cvtColor(canny_frame, cdst, CV_GRAY2BGR);
        cvtColor(mat_frame, gray, CV_BGR2GRAY);

        HoughLinesP(canny_frame, lines, 1, CV_PI/180, 50, 50, 10);

        for( size_t i = 0; i < lines.size(); i++ )
        {
          Vec4i l = lines[i];
          line(mat_frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
        }

     
        if(!frame) break;

        // cvShowImage seems to be a problem in 3.1
        //cvShowImage("Capture Example", frame);

        imshow("Capture Example", mat_frame);

        char c = cvWaitKey(10);
        if( c == 'q' ) break;
        clock_gettime(CLOCK_REALTIME, &end);

        pthread_mutex_lock(&lock); 
        exec.t1 = end;
        exec.t2 = start;
        newVal = true;
        pthread_mutex_unlock(&lock); 
    }

    threadEnd = true;
    cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");

}

void *logThread(void *in)
{
    while(!threadEnd)
    {
        struct timespec tempEnd;
        struct timespec tempStart;
        pthread_mutex_lock(&lock);
        if(newVal)
        {
            newVal = false;
            tempEnd = exec.t1;
            tempStart = exec.t2;
        }
        else
        {
            pthread_mutex_unlock(&lock);
            continue;
        }
        pthread_mutex_unlock(&lock);

        double time_spent = (tempEnd.tv_sec - tempStart.tv_sec) +
						    (tempEnd.tv_nsec - tempStart.tv_nsec) / BILLION;

        double inverse = (double)1.00 / time_spent;

        if(fps.size() < MATHFRAMES)
            fps.push_back(inverse);
        else
        {
            fps.erase(fps.begin());
            fps.push_back(inverse);
        }
    }
}

void *calcThread(void *in)
{
    while(!threadEnd)
    {
        vector<double> jitter;
        printf("\rAverage fps for the last %d frames: ", MATHFRAMES);
        pthread_mutex_lock(&lock1); 
        if(fps.size() < MATHFRAMES)
        {
            printf("Waiting to reach %d frames", MATHFRAMES);
            fflush(stdout);
        }
        else
        {
            double avg = 0;
            for(int i = 0; i < MATHFRAMES; i++)
            {
                avg += fps[i];

                if(i != MATHFRAMES - 1)
                {
                    // jitter[i] = abs(fps[i] - fps[i + 1]);
                    jitter.push_back(abs(fps[i] - fps[i + 1]));
                }
            }
            double jitterSum = 0;
            for(int i = 0; i < MATHFRAMES - 1; i++)
            {
                jitterSum += jitter[i];
                // printf("Jitter: %lf\n", jitter[i]);
            }
            avg /= fps.size();
            // printf("Jitter: %lf\n", jitterSum);
            jitterSum /= jitter.size();
            printf("\rAverage FPS for %d frames: %lf         Jitter: %lf                  ", MATHFRAMES, avg, jitterSum);
            fflush(stdout);
        }
        pthread_mutex_unlock(&lock1); 
    }
    printf("\n");
}


int main( int argc, char** argv )
{
    int dev=0;
    pthread_t thread, thread1, thread2;
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&lock1, NULL);

    if(argc > 1)
    {
        sscanf(argv[1], "%d", &dev);
        printf("using %s\n", argv[1]);
    }
    else if(argc == 1)
        printf("using default\n");

    else
    {
        printf("usage: capture [dev]\n");
        exit(-1);
    }

    namedWindow( timg_window_name, CV_WINDOW_AUTOSIZE );
    // Create a Trackbar for user to enter threshold
    createTrackbar( "Min Threshold:", timg_window_name, &lowThreshold, max_lowThreshold, CannyThreshold );

    capture = (CvCapture *)cvCreateCameraCapture(dev);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, HRES);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, VRES);

    printf("\n");
    pthread_create(&thread, NULL, houghThread, NULL);
    pthread_create(&thread1, NULL, calcThread, NULL);
    pthread_create(&thread2, NULL, logThread, NULL);
    pthread_join(thread, NULL);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    cvReleaseCapture(&capture);
    
}
