/**
*/
#include <ctime>
#include <iostream>
#include <unistd.h>
#include "opencv2/opencv.hpp"
#include "canny_util.h"

using namespace std;
using namespace cv;

/* Possible options: 320x240, 640x480, 1024x768, 1280x1040, and so on. */
/* Pi Camera MAX resolution: 2592x1944 */

#define WIDTH 640
#define HEIGHT 480
#define NFRAME 1.0

#define BILLION 1000000000L

int main(int argc, char **argv)
{
   char* dirfilename;        /* Name of the output gradient direction image */
   char outfilename[128];    /* Name of the output "edge" image */
   char composedfname[128];  /* Name of the output "direction" image */
   unsigned char *image;     /* The input image */
   unsigned char *edge;      /* The output edge image */
   int rows, cols;           /* The dimensions of the image. */
   float sigma,              /* Standard deviation of the gaussian kernel. */
	 tlow,               /* Fraction of the high threshold in hysteresis. */
	 thigh;              /* High hysteresis threshold control. The actual
			        threshold is the (100 * thigh) percentage point
			        in the histogram of the magnitude of the
			        gradient image that passes non-maximal
			        suppression. */

   /****************************************************************************
   * Get the command line arguments.
   ****************************************************************************/
   if(argc < 4){
   fprintf(stderr,"\n<USAGE> %s sigma tlow thigh [writedirim]\n",argv[0]);
      fprintf(stderr,"      sigma:      Standard deviation of the gaussian");
      fprintf(stderr," blur kernel.\n");
      fprintf(stderr,"      tlow:       Fraction (0.0-1.0) of the high ");
      fprintf(stderr,"edge strength threshold.\n");
      fprintf(stderr,"      thigh:      Fraction (0.0-1.0) of the distribution");
      fprintf(stderr," of non-zero edge\n                  strengths for ");
      fprintf(stderr,"hysteresis. The fraction is used to compute\n");
      fprintf(stderr,"                  the high edge strength threshold.\n");
      fprintf(stderr,"      writedirim: Optional argument to output ");
      fprintf(stderr,"a floating point");
      fprintf(stderr," direction image.\n\n");
      exit(1);
   }

   sigma = atof(argv[1]);
   tlow = atof(argv[2]);
   thigh = atof(argv[3]);
   rows = HEIGHT;
   cols = WIDTH;
   int counter=0;

   if(argc == 5) dirfilename = (char *) "dummy";
	 else dirfilename = NULL;

   VideoCapture cap;
   // open the default camera (/dev/video0)
   // Check VideoCapture documentation for more details
   if(!cap.open(0)){
				cout<<"Failed to open /dev/video0"<<endl;
        return 0;
	 }
	cap.set(CAP_PROP_FRAME_WIDTH, WIDTH);
   cap.set(CAP_PROP_FRAME_HEIGHT,HEIGHT);

	Mat frame, grayframe;
	double FPS_Current =0;
   double total_time_elapsed,total_time_capture,total_time_process;
   double realtotal_time_elapsed,realtotal_time_capture,realtotal_time_process;
   double time_elapsed_avg, time_capture_avg, time_process_avg;
   double realtime_elapsed_avg, realtime_capture_avg, realtime_process_avg;
   timespec start_total,end_total;
   clock_gettime(CLOCK_REALTIME,&start_total);
   for(;;)
   {
    timespec start_r,mid_r,end_r;
   clock_t begin, mid, end;
   double time_elapsed_current, time_capture_current, time_process_current;
   double realtime_elapsed_current, realtime_capture_current, realtime_process_current;
   clock_gettime(CLOCK_REALTIME,&start_r);
   begin = clock();
   //capture
	 cap >> frame;
	 if( frame.empty() ) break; // end of video stream
	 imshow("Live feed", frame);
	 if( waitKey(10) == 27 ) break; // stop capturing by pressing ESC
    clock_gettime(CLOCK_REALTIME,&mid_r);
	 mid = clock();
	 cvtColor(frame, grayframe, COLOR_BGR2GRAY);
	 image = grayframe.data;

   /****************************************************************************
   * Perform the edge detection. All of the work takes place here.
   ****************************************************************************/
   if(VERBOSE) printf("Starting Canny edge detection.\n");
   if(dirfilename != NULL){
      sprintf(composedfname, "camera_s_%3.2f_l_%3.2f_h_%3.2f.fim",
      sigma, tlow, thigh);
      dirfilename = composedfname;
   }
   canny(image, rows, cols, sigma, tlow, thigh, &edge, dirfilename);

   /****************************************************************************
   * Write out the edge image to a file.
   ****************************************************************************/
   sprintf(outfilename, "frame%03d.pgm",counter);
   counter++;
   if(VERBOSE) printf("Writing the edge iname in the file %s.\n", outfilename);
   if(write_pgm_image(outfilename, edge, rows, cols, NULL, 255) == 0){
      fprintf(stderr, "Error writing the edge image, %s.\n", outfilename);
      exit(1);
   }
   clock_gettime(CLOCK_REALTIME,&end_r);
   end = clock();
   
   time_elapsed_current = (double) (end - begin) / CLOCKS_PER_SEC;
   time_capture_current = (double) (mid - begin) / CLOCKS_PER_SEC;
   time_process_current = (double) (end - mid) / CLOCKS_PER_SEC;

   realtime_elapsed_current = (double) (end_r.tv_sec - start_r.tv_sec) + (double)(end_r.tv_nsec - start_r.tv_nsec) / (double)BILLION;
   realtime_capture_current = (double) (mid_r.tv_sec - start_r.tv_sec) + (double)(end_r.tv_nsec - start_r.tv_nsec) / (double)BILLION;
   realtime_process_current = (double) (end_r.tv_sec - mid_r.tv_sec) + (double)(end_r.tv_nsec - start_r.tv_nsec) / (double)BILLION;
   
   double realworldtime_elapsed = (end_r.tv_sec - start_r.tv_sec)+ (double)(end_r.tv_nsec - start_r.tv_nsec) / (double)BILLION; 
   
   imshow("Live feed (Hit esc to stop)", grayframe);
   
   total_time_elapsed += time_elapsed_current;
   total_time_capture += time_capture_current;
   total_time_process += time_process_current;

   realtotal_time_elapsed += realtime_elapsed_current;
   realtotal_time_capture += realtime_capture_current;
   realtotal_time_process += realtime_process_current;
   
   FPS_Current = NFRAME/realtime_elapsed_current;

   time_elapsed_avg = total_time_elapsed/counter;
   time_capture_avg = total_time_capture/counter;
   time_process_avg = total_time_process/counter;

   realtime_elapsed_avg = realtotal_time_elapsed/counter;
   realtime_capture_avg = realtotal_time_capture/counter;
   realtime_process_avg = realtotal_time_process/counter;
   
   printf("Wall time for frame: %lf \n",realworldtime_elapsed);
   printf("Average elapsed time: %lf \n",time_elapsed_avg);
   printf("Average capture time: %lf \n",time_capture_avg);
   printf("Average process time: %lf \n",time_process_avg);
   printf("Average real elapsed time: %lf \n",realtime_elapsed_avg);
   printf("Average real capture time: %lf \n",realtime_capture_avg);
   printf("Average real process time: %lf \n",realtime_process_avg);
   printf("FPS: %01lf\n", FPS_Current);

   grayframe.data = edge;
   
   }
   
   clock_gettime(CLOCK_REALTIME,&end_total);
   double realworldtime_total = (end_total.tv_sec - start_total.tv_sec)+ (end_total.tv_nsec - start_total.tv_nsec) / BILLION; 
   printf("Total wall time: %f\n ",realworldtime_total);
   
    return 0;
}
