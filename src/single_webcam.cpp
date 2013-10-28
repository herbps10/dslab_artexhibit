#include <stdio.h>
#include <stdlib.h>
#include "cv.h"
#include "highgui.h"
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include "string.h"
#include <thread>

using namespace cv;
using namespace std;

#define WIDTH 1280
#define HEIGHT 800

int grid_x, grid_y, grid_spacing, grid_m, grid_n;
int *grid;
int music_column = 0;

string sounds[5] = {
	"sounds/Piano.ff.C5.wav",
	"sounds/Piano.ff.C4.wav",
	"sounds/Piano.ff.D4.wav",
	"sounds/Piano.ff.E4.wav",
	"sounds/Piano.ff.G4.wav"
};

double time_diff(struct timeval x , struct timeval y)
{
	double x_ms , y_ms , diff;

	x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
	y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;

	diff = (double)y_ms - (double)x_ms;

	return diff;
}

void music_task()
{
	struct timeval before, after;
	gettimeofday(&before, NULL);



	while(true)
	{
		gettimeofday(&after, NULL);

		if(time_diff(before, after) > 400000)
		{
			printf("Firing at %f\n", time_diff(before, after));

			for(int row = 0; row < grid_m; row++)
			{
				if(grid[grid_n * row + music_column] == 1)
				{
					printf("Playing %i %i\n", row, music_column);

					string command = "play " + sounds[row] + " &";

					//system(command.c_str());
				}
			}

			gettimeofday(&before, NULL);

			music_column = (music_column + 1) % grid_n;
		}
	}
}

void draw_grid(IplImage *image, int *grid)
{
	for(int x = grid_x; x < grid_x + grid_spacing * grid_n; x += grid_spacing)
	{
		cvLine(image, Point(x, grid_y), Point(x, grid_y + grid_spacing * grid_m), Scalar(0, 255, 0), 1);
	}

	for(int y = grid_y; y < grid_y + grid_spacing * grid_m; y += grid_spacing)
	{
		cvLine(image, Point(grid_x, y), Point(grid_x + grid_spacing * grid_n, y), Scalar(0, 255, 0), 1);
	}


	for(int row = 0; row < grid_m; row++)
	{
		for(int col = 0; col < grid_n; col++)
		{
			if(col == music_column)
			{
				cvRectangle(image, Point(grid_x + col * grid_spacing, grid_y + row * grid_spacing), Point(grid_x + (col + 1) * grid_spacing, grid_y + (row + 1) * grid_spacing), Scalar(255, 0, 0), 2);
			}

			if(grid[grid_n * row + col] == 1)
			{
				cvRectangle(image, Point(grid_x + col * grid_spacing, grid_y + row * grid_spacing), Point(grid_x + (col + 1) * grid_spacing, grid_y + (row + 1) * grid_spacing), Scalar(0, 0, 255), 2);
			}
		}

	}

}

int main( int argc, char **argv)
{
	grid_x = 0;
	grid_y = 0;
	grid_spacing = 160; 
	grid_m = 5;
	grid_n = 8;

	CvCapture *capture = 0;
	IplImage  *frame = 0;
	int       key = 0;



	grid = (int *)malloc(sizeof(int) * grid_m * grid_n);

	for(int i = 0; i < grid_m; i++)
	{
		for(int j = 0; j < grid_n; j++)
		{
			grid[grid_n * i + j] = 0;
		}
	}

	/* initialize camera */
	capture = cvCaptureFromCAM(0);
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, WIDTH);
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

	/* always check */
	if ( !capture ) {
	    fprintf( stderr, "Cannot open initialize webcam!\n" );
	    return 1;
	}

	/* create a window for the video */
	cvNamedWindow( "result", CV_WINDOW_AUTOSIZE );

	IplImage *original, *difference;


	IplImage *drawing = cvCreateImage(cvSize(WIDTH, HEIGHT), IPL_DEPTH_8U, 1);

	cvNamedWindow("drawing", CV_WINDOW_AUTOSIZE);
	
	draw_grid(drawing, grid);

	cvShowImage("drawing", drawing);

	original = cvQueryFrame(capture);

	IplImage *img1 = cvCreateImage(cvGetSize(original), IPL_DEPTH_8U, 1);
	IplImage *img2 = cvCreateImage(cvGetSize(original), IPL_DEPTH_8U, 1);

	difference = cvCreateImage(cvGetSize(original), IPL_DEPTH_8U, 1);

	cvCvtColor(original, img2, CV_RGB2GRAY);


	cvNamedWindow("original", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("feed", CV_WINDOW_AUTOSIZE);

	cvShowImage("original", original);



	while( key != 'q' ) {
	    /* get a frame */
	    frame = cvQueryFrame( capture );

	    cvCvtColor(frame, img1, CV_RGB2GRAY);

	    /* always check */
	    if( !frame )
	    {
	      break;
	      fprintf( stdout, "ERROR: frame is null...\n" );
	    }

	    cvAbsDiff(img1, img2, difference);

	    for(int row = 0; row < grid_m; row++)
	    {
	        for(int col = 0; col < grid_n; col++ )
	        {
	    		Mat roi(difference, Rect(grid_x + col * grid_spacing, grid_y + row * grid_spacing, grid_spacing, grid_spacing));

			if(sum(roi)[0] > 360000)
			{
				grid[grid_n * row + col] = 1;
			}
			else
			{
				grid[grid_n * row + col] = 0;
			}
	        }
	    }

	    

	    draw_grid(drawing, grid);
	    draw_grid(frame, grid);
	    draw_grid(difference, grid);

	    /* display current frame */
	    cvShowImage( "feed", frame );
	    cvShowImage( "result", difference );
	    cvShowImage( "drawing", drawing );

	    /* exit if user press 'q' */
	    key = cvWaitKey( 1 );
	}

	/* free memory */
	cvDestroyWindow( "result" );
	cvReleaseCapture( &capture );

	return 0;
}
