#include <stdio.h>
#include <stdlib.h>
#include "cv.h"
#include "highgui.h"
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include "string.h"
#include <mpi.h>
#include <thread>

using namespace cv;
using namespace std;

#define WIDTH 1280
#define HEIGHT 800

#define PLAY_MUSIC 0 // 1 to play music, 0 to just print out when it _would_ play music (much less annoying this way)



int grid_x, grid_y, grid_spacing, grid_rows, grid_cols;

int webcams_x, webcams_y;

int *grid, *master_grid;
int music_column = 0;

string sounds[5] = {
	"sounds/Piano.ff.C5.wav",
	"sounds/Piano.ff.C4.wav",
	"sounds/Piano.ff.D4.wav",
	"sounds/Piano.ff.E4.wav",
	"sounds/Piano.ff.G4.wav"
};

int array_coordinates(int width, int x, int y)
{
    return width * y + x;
}

/*
 * Draws the grid onto a OpenCV image
 */
void draw_grid(IplImage *image, int *grid)
{
	for(int x = grid_x; x < grid_x + grid_spacing * grid_cols; x += grid_spacing)
	{
		cvLine(image, Point(x, grid_y), Point(x, grid_y + grid_spacing * grid_rows), Scalar(0, 255, 0), 1);
	}

	for(int y = grid_y; y < grid_y + grid_spacing * grid_rows; y += grid_spacing)
	{
		cvLine(image, Point(grid_x, y), Point(grid_x + grid_spacing * grid_cols, y), Scalar(0, 255, 0), 1);
	}


	for(int row = 0; row < grid_rows; row++)
	{
		for(int col = 0; col < grid_cols; col++)
		{
			if(grid[grid_cols * row + col] == 1)
			{
				cvRectangle(image, Point(grid_x + col * grid_spacing, grid_y + row * grid_spacing), Point(grid_x + (col + 1) * grid_spacing, grid_y + (row + 1) * grid_spacing), Scalar(0, 0, 255), 2);
			}
		}
	}
}

/*
 * Prints out the grid for inspection
 */
void print_grid()
{
	for(int i = 0; i < grid_rows; i++) {
		for(int j = 0; j < grid_cols; j++)
		{
			cout << grid[grid_cols * i + j] << "\t";
		}

		cout << endl;
	}
}

/*
 * Prints out the master grid for inspection
 */
void print_master_grid()
{
    for(int y = 0; y < grid_rows * webcams_y; y++)
    {
        for(int x = 0; x < grid_cols * webcams_x; x++)
        {
            cout << master_grid[array_coordinates(grid_cols * webcams_x, x, y)] << "\t";
        }

        cout << endl;
    }
}

double time_diff(struct timeval x , struct timeval y)
{
	double x_ms , y_ms , diff;

	x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
	y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;

	diff = (double)y_ms - (double)x_ms;

	return diff;
}

// Scan the grid to look for significant differences between the reference and current
void update_grid(IplImage *difference, int difference_threshold)
{
    for(int row = 0; row < grid_rows; row++)
    {
        for(int col = 0; col < grid_cols; col++ )
        {
            Mat roi(difference, Rect(grid_x + col * grid_spacing, grid_y + row * grid_spacing, grid_spacing, grid_spacing));

            if(sum(roi)[0] > difference_threshold)
            {
                grid[grid_cols * row + col] = 1;
            }
            else
            {
                grid[grid_cols * row + col] = 0;
            }
        }
    }
}

//
// Returns how many grid cells are activated
//
int count_grid_cells_on()
{
	int count = 0;

	for(int row = 0; row < grid_rows; row++)
	{
		for(int col = 0; col < grid_cols; col++)
		{
			count += grid[grid_cols * row + col];
		}
	}

	return count;
}

//
// Capture a frame from the webcam
//
int capture_frame(CvCapture *capture, IplImage *color_frame, IplImage *gray_frame)
{
    // Capture a frame from the webcam
    color_frame = cvQueryFrame( capture );


    // Check to make sure the frame was captured correctly
    if( !color_frame )
    {
      cout << "ERROR: frame is null..." << endl;

      return 0;
    }

    // Convert the frame from color to grayscale
    cvCvtColor(color_frame, gray_frame, CV_RGB2GRAY);

    return 1;
}

// Take the absolute difference between the original reference photo and the current frame
void compute_difference(IplImage *reference, IplImage *gray_frame, IplImage *difference)
{
    cvAbsDiff(reference, gray_frame, difference);
}

/*
 * music_task is a thread task that handles playing the sounds
 *
 * it watches the grid global variable and plays sounds at consistent intervals
 */
void music_task()
{
	struct timeval before, after;
	gettimeofday(&before, NULL);
	while(true)
	{
		gettimeofday(&after, NULL);

		if(time_diff(before, after) > 400000)
		{
			cout << "Firing at " <<  time_diff(before, after) << endl;

			for(int row = 0; row < grid_rows * webcams_y; row++)
			{
				if(grid[grid_cols * row + music_column] == 1)
				{
					printf("Playing %i %i\n", row, music_column);

					string command = "play " + sounds[row] + " &";
					cout << command << endl;

					if(PLAY_MUSIC == 1)
					{
						system(command.c_str());
					}
					else
					{
						cout << command << endl;
					}	
				}
			}

			gettimeofday(&before, NULL);

			music_column = (music_column + 1) % (grid_cols * webcams_x);

			//print_grid();
		}

	}
}

int main(int argc, char *argv[]) {

	//
	// Initialize MPI
	//
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	//
	// Initialize the grid
	//
	grid_x = 0;
	grid_y = 0;
	grid_spacing = 140; 
	grid_rows = 5;
	grid_cols = 8;

	//
	// Set up the grid of webcams
	//
	webcams_x = 2; // How many webcams across
	webcams_y = 1; // How many webcams down

	grid = (int *)malloc(sizeof(int) * grid_rows * grid_cols);

	for(int i = 0; i < grid_rows; i++)
	{
		for(int j = 0; j < grid_cols; j++)
		{
			grid[grid_cols * i + j] = 0;
		}
	}

	//
	// Rank 0 plays the music
	//
	if(rank == 0)
	{

		// Set up master grid
		master_grid = (int *)malloc(sizeof(int) * grid_rows * webcams_y * grid_cols * webcams_x);

		for(int y = 0; y < grid_rows * webcams_y; y++)
		{
			for(int x = 0; x < grid_cols * webcams_x; x++)
			{
				master_grid[array_coordinates(grid_cols * webcams_x, x, y)] = 0;
			}
		}

		print_master_grid();

		// Recieve a message
		MPI_Status status;

		thread t1(music_task);

		cvNamedWindow("drawing",  CV_WINDOW_AUTOSIZE);

		IplImage *drawing = cvCreateImage(cvSize(WIDTH, HEIGHT), IPL_DEPTH_8U, 1);

		//draw_grid(drawing, grid);


		while(true)
		{
			MPI_Recv(grid, grid_rows * grid_cols, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

			// Copy the grid into the master grid depending on which slave sent it
			int sender_x = (status.MPI_SOURCE - 1) % webcams_x;
			int sender_y = (status.MPI_SOURCE - 1) / webcams_x;

			for(int y = 0; y < grid_rows; y++)
			{
				for(int x = 0; x < grid_cols; x++)
				{
					master_grid[array_coordinates(webcams_x * grid_cols, sender_x * grid_cols + x, sender_y * grid_rows + y)] = grid[array_coordinates(grid_cols, x, y)];
				}
			}

            		//cout << "recieved message from (" << sender_x << "," << sender_y << ")" << endl;
			system("clear");
			print_master_grid();

			cvShowImage("drawing", drawing);
		}
	}

	//
	// Other ranks watch their webcams
	//
	if(rank > 0)
	{
		CvCapture *capture = 0;
		IplImage *color_frame = 0, *gray_frame = 0,
			 *color_reference = 0, *gray_reference = 0,
			 *difference = 0;

		int key = 0;

		// Initialize the webcam
		capture = cvCaptureFromCAM(0);
		cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, WIDTH);
		cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);


		// Check to see if the camera initialized correctly
		if ( !capture ) {
		    fprintf( stderr, "Cannot open initialize webcam!\n" );
		    return 1;
		}
        
		// Create output windows
		cvNamedWindow("difference",  CV_WINDOW_AUTOSIZE);
		cvNamedWindow("reference",   CV_WINDOW_AUTOSIZE);
		cvNamedWindow("color_frame", CV_WINDOW_AUTOSIZE);


		// Take the original reference image
		// We need kick the webcam so it starts initializing, then wait for the exposure
		// settings to stabilize. If we don't wait, the reference photo will be really dark.
		color_reference = cvQueryFrame(capture);
		sleep(2);
		color_reference = cvQueryFrame(capture);

		gray_frame     = cvCreateImage(cvGetSize(color_reference), IPL_DEPTH_8U, 1);
		difference     = cvCreateImage(cvGetSize(color_reference), IPL_DEPTH_8U, 1);
		gray_reference = cvCreateImage(cvGetSize(color_reference), IPL_DEPTH_8U, 1);

		cvCvtColor(color_reference, gray_reference, CV_RGB2GRAY);

		cvShowImage("reference", color_reference);

		//
		// CALIBRATE THE DIFFERENCE THRESHOLD
		//

		int difference_threshold = 0;
		int max_cells_on = 0;
		do
		{
			max_cells_on = 0;
			difference_threshold += 50000;

			cout << "Setting threshold to " << difference_threshold << endl;

			// We want to make sure that this is a reliable threshold with no false positives
			// so sample the webcam 5 times and make sure it never gets a positive
			for(int i = 0; i < 5; i++)
			{
				capture_frame(capture, color_frame, gray_frame);
				compute_difference(gray_reference, gray_frame, difference);

				update_grid(difference, difference_threshold);

				if(count_grid_cells_on() > max_cells_on)
				{
					max_cells_on = count_grid_cells_on();
				}
			}
		}
		while(max_cells_on > 0);


		cout << "Final calibrated difference threshold: " << difference_threshold << endl;

		while( key != 'q' ) {
		    capture_frame(capture, color_frame, gray_frame);
		    compute_difference(gray_reference, gray_frame, difference);

		    // Update the grid based on the new difference image
		    update_grid(difference, difference_threshold);
		    
		    // when we're done updating the grid
		    // send a message to rank 0
		    MPI_Send(grid, grid_rows * grid_cols, MPI_INT, 0, 0, MPI_COMM_WORLD);

		    // Draw the grid on top of each output
		    draw_grid(gray_frame, grid);
		    draw_grid(difference, grid);

		    // Display the current frame
		    cvShowImage("color_frame", gray_frame);
		    cvShowImage("difference",  difference);
		
		    // Exit if the user presses 'q'
		    key = cvWaitKey( 1 );
		}

		// Free memory
		cvDestroyWindow( "reference" );
		cvDestroyWindow( "difference" );
		cvDestroyWindow( "color_frame" );
		cvReleaseCapture( &capture );
	}

	//
	// Quit the program
	//
	MPI_Finalize();

	return 0;
}
