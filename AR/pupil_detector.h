#ifndef PUPIL_DETECTOR
#define PUPIL_DETECTOR

#include <limits>
#include "opencv2/opencv.hpp"

struct Center
{
  cv::Point2d location;
  double radius;
  double confidence;
};

class pupil_detector{
	public:
		int thresholdStep = 2;
		int minThreshold = 50;
		int maxThreshold = 220;
		int minRepeatability = 3;
		int minDistBetweenBlobs = 10;

		bool filterByColor = true;
		int blobColor = 0;

		bool filterByArea = true;
		int minArea = 25;
		int maxArea = 5000;

		bool filterByCircularity = false;
		float minCircularity = 0.8f;
		float maxCircularity = std::numeric_limits<float>::max();

		bool filterByInertia = true;
		//minInertiaRatio = 0.6;
		float minInertiaRatio = 0.1f;
		float maxInertiaRatio = std::numeric_limits<float>::max();

		bool filterByConvexity = true;
		//minConvexity = 0.8;
		float minConvexity = 0.95f;
		float maxConvexity = std::numeric_limits<float>::max();
		
		void findBlobs(const cv::Mat& image, const cv::Mat& binaryImage, std::vector<Center> &centers) const;
	
		void detect(const cv::Mat& image, std::vector<cv::KeyPoint>& keypoints) const;
};

#endif
