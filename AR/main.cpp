#include <iostream>
#include <opencv2/opencv.hpp>
#include "pupil_detector.h"

#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>


using namespace cv;
using namespace std;

//globals
int low_threshold = 20;
int high_threshold = 60;
int canny_threshold = 0;
int min_area = 300;//300;
int max_area = 3000;//500;

int ref_min_threshold;
int ref_max_threshold;

static void on_low_thresh_trackbar(int, void *)
{
    setTrackbarPos("Low threshold", "threshold", low_threshold);
}

static void on_high_thresh_trackbar(int, void *)
{
    setTrackbarPos("High threshold", "threshold", high_threshold);
}

static void on_min_area_thresh_trackbar(int, void *)
{
    setTrackbarPos("Min Area", "threshold", min_area);
}

static void on_max_area_thresh_trackbar(int, void *)
{
    setTrackbarPos("Max Area", "threshold", max_area);
}


void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void send_point( int new_socket, int x, int y ) {
  int n;

  char buffer[32];
  sprintf( buffer, "%d %d\n", x, y);
  cout<<"sending point "<<x<<" "<<y<<" "<<strlen(buffer)<<endl;
  send(new_socket , buffer , strlen(buffer) , 0 ); 
  
}

int findNearestPointCartesianCV(vector<Point_<float>> &input,Point_<float> point){
    double distance = INT_MAX;
    int closest_point_idx = 0;
    int count = 0;
    for(auto &i : input){
        double dist_to_point = cv::norm(i - point);
        closest_point_idx = dist_to_point < distance ? count : closest_point_idx;
        distance = dist_to_point < distance ? dist_to_point : distance;
        count++;
    }
    return closest_point_idx;
}

void find_calibration_points_by_pupil_keypoints (vector<Point_<float>>&pupil_keypoints,vector<Point_<float>>& calibration_points){
  //go over all pupil keypoints and determine a singlur one
  float mean_x = 0;
  float mean_y = 0;
  float sum_x = 0;
  float sum_y = 0;
  for(auto &i : pupil_keypoints){
    sum_x += i.x;
    sum_y += i.y;
  }
  mean_x = sum_x/int(pupil_keypoints.size());
  mean_y = sum_y/int(pupil_keypoints.size());
  calibration_points.push_back(Point_<float>(mean_x, mean_y));
}

int main(int argc, char *argv[])
{
    //variables for network
    int server_fd, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char buffer[1024] = {0}; 

    bool calibrating = false;
    vector<Point_<int>> cali_map;
    int curr_display_idx = 0;
    int count_stored_keypoints = 0;
    bool working = false;

    vector<Point_<float>> stored_pupil_keypoints(10);


    for(int i = -2; i<=2;i++){
        for(int j = -2; j<=2;j++){
            cali_map.push_back(Point_<int>(300,500) + Point_<int>(i*80,j*80));
        }
    }

    Point_<float> center;
    vector<Point_<float>> calibration_points;

    bool online = argc > 1;
    if(online){
            if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
        { 
            perror("socket failed"); 
            exit(EXIT_FAILURE); 
        } 

        // Forcefully attaching socket to the port 8080 
        // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
        //                                             &opt, sizeof(opt))) 
        // { 
        //     perror("setsockopt"); 
        //     exit(EXIT_FAILURE); 
        // } 
        address.sin_family = AF_INET; 
        address.sin_addr.s_addr = inet_addr("192.168.7.2"); //server ip INADDR_ANY
        address.sin_port = htons( 50001 ); 
        
        cout<<"waiting for connection"<<endl;
        // Forcefully attaching socket to the port 8080 
        if (::bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0) 
        { 
            perror("bind failed"); 
            exit(EXIT_FAILURE); 
        } 
        if (listen(server_fd, 3) < 0) 
        { 
            perror("listen"); 
            exit(EXIT_FAILURE); 
        } 
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
                        (socklen_t*)&addrlen))<0) 
        { 
            perror("accept"); 
            exit(EXIT_FAILURE); 
        } 
        cout<<"continuing"<<endl;
    }
    

	

    //CV part

    VideoCapture cap("http://pi.local:5000/stream/video.mjpeg"); // open the streaming camera http://pi.local:5000/stream/video.mjpeg
    if(!cap.isOpened())  // check if we succeeded
        return -1;

    namedWindow("threshold",1);
    namedWindow("keypoints", 1);
    
    createTrackbar("Low threshold", "threshold", &low_threshold, 255, on_low_thresh_trackbar);
    createTrackbar("High threshold", "threshold", &high_threshold, 255, on_high_thresh_trackbar);
    createTrackbar("Min Area", "threshold", &min_area, 1000, on_min_area_thresh_trackbar);
    createTrackbar("Max Area", "threshold", &max_area, 10000, on_max_area_thresh_trackbar);
    
    pupil_detector left_pupil;
    
    // Filter by Area.
    left_pupil.filterByArea = true;
    
    // Filter by Circularity
    left_pupil.filterByCircularity = true;
    left_pupil.minCircularity = 0.6;
     
    // Filter by Convexity
    left_pupil.filterByConvexity = false;
    left_pupil.minConvexity = 0.87;
     
    // Filter by Inertia
    left_pupil.filterByInertia = false;
    left_pupil.minInertiaRatio = 0.01;
    
    //reflection detector
    pupil_detector reflection;
    // Filter by Area.
    reflection.filterByArea = true;
    
     
    // Filter by Circularity
    reflection.filterByCircularity = true;
    reflection.minCircularity = 0.6;
     
    // Filter by Convexity
    reflection.filterByConvexity = false;
    reflection.minConvexity = 0.87;
     
    // Filter by Inertia
    reflection.filterByInertia = false;
    reflection.minInertiaRatio = 0.01;
    
    reflection.filterByColor = true;
    reflection.blobColor = 1;
   
    for(;;)
    {

        Mat frame;
        cap >> frame; // get a new frame from camera
	
	//Rect myROI(125, 50, 200, 350);
	//Mat croppedImage = frame(myROI);
	Mat& croppedImage = frame;
	
	const int scale = 2;
	Mat small_frame( cvRound(croppedImage.rows/scale), cvRound(croppedImage.cols/scale), CV_8UC1);
	resize(croppedImage, small_frame, small_frame.size());
	
	
	
        cvtColor(small_frame, small_frame, COLOR_BGR2GRAY);
	
	GaussianBlur(small_frame, small_frame, Size(5,5), 1.5, 1.5);
	
	left_pupil.minThreshold = low_threshold;
	left_pupil.maxThreshold = high_threshold;
	left_pupil.minArea = min_area;
	left_pupil.maxArea = max_area;
	
	std::vector<KeyPoint> pupil_keypoints;
	std::vector<KeyPoint> ref_keypoints;
	left_pupil.detect( small_frame, pupil_keypoints);
	
	//detect reflection
	Mat binarizedImage;
	threshold(small_frame, binarizedImage, canny_threshold, 255, THRESH_BINARY);
	// imshow("reflection", binarizedImage);
	
	// Draw detected blobs as red circles.
	// DrawMatchesFlags::DRAW_RICH_KEYPOINTS flag ensures the size of the circle corresponds to the size of blob
	Mat im_with_keypoints;
	drawKeypoints( small_frame, pupil_keypoints, im_with_keypoints, Scalar(0,255,0), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
	//drawKeypoints( im_with_keypoints, ref_keypoints, im_with_keypoints, Scalar(0,255,0), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
	
	
	/*
	for(KeyPoint kpt: pupil_keypoints){
	    Point2f pt = kpt.pt;
	    stringstream temp;
	    temp << "(" << pt.x << "," << pt.y << ")";
	    const char* str = temp.str().c_str();
	    cout << "Inside pupil for loop";
	    write(newsockfd,str, 30);
	    break;
	}
    */
	// Show blobs
    if(pupil_keypoints.empty()){
        imshow("keypoints", small_frame );
    }else{
	    imshow("keypoints", im_with_keypoints );
    }
	char key = waitKey(30);
	if(!pupil_keypoints.empty() ){
	    KeyPoint & kpt = pupil_keypoints.front();

         
	    
	    cout<<"x:"<<kpt.pt.x<<"y:"<<kpt.pt.y<<"num"<<pupil_keypoints.size()<<endl;
        
        

        

        if (calibrating) {
          if (count_stored_keypoints == 10){
            calibrating = false;
            find_calibration_points_by_pupil_keypoints(stored_pupil_keypoints, calibration_points);
            count_stored_keypoints = 0;
            curr_display_idx++;
            
          }else{
            stored_pupil_keypoints[count_stored_keypoints] = kpt.pt;
            cout<<"calibrating point "<<curr_display_idx<<"count "<<count_stored_keypoints<<endl;
            count_stored_keypoints++;
          }

        }
        if( key == 'c') {
          calibrating = true;
          count_stored_keypoints = 0;
        }
        else if( key == 'w') {
          working = true;
        }



        if (working) {
            int idx  = findNearestPointCartesianCV(calibration_points,kpt.pt);
            curr_display_idx = idx;
        }
        
        if(online){

            
            if(curr_display_idx < cali_map.size()) send_point(new_socket, kpt.pt.x,kpt.pt.y );        
            // // send_point(new_socket,kpt.pt.x, kpt.pt.y);
        }


        
	}
    if( key == 'q'){
            cout<<"you pressed Q"<<endl;
            break;
        } 
        
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    cout<<"exiting"<<endl;
    return 0;
}


// cv::VideoCapture cap("http://pi.local:5000/stream/video.mjpeg"); 
// sudo service uv4l_raspicam restart
// /etc/uv4l/uv4l-raspicam
// sudo killall -HUP mDNSResponder && echo macOS DNS Cache Reset
// sudo shutdown -h now
// uv4l --driver raspicam --auto-video_nr --width 1280 --height 720 --encoding mjpeg --server-option '--port=5000' --server-option '--bind-host-address=pi.local'
// dd if=/dev/video2 of=snapshot.jpeg bs=11M count=1
// sudo apt-get --purge remove uv4l-raspicam-extras
