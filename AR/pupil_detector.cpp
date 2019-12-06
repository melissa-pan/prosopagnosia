#include "pupil_detector.h"

using namespace cv;

#define DEBUG 0

void pupil_detector::findBlobs(const Mat& image, const Mat& binaryImage, std::vector<Center> &centers) const
{
    centers.clear();

    std::vector < std::vector<Point> > contours;
    findContours(binaryImage, contours, RETR_LIST, CHAIN_APPROX_NONE);
    
    #ifdef DEBUG_OFF
		Mat keypointsImage;
		cvtColor(binaryImage, keypointsImage, COLOR_GRAY2RGB);

		Mat contoursImage;
		cvtColor(binaryImage, contoursImage, COLOR_GRAY2RGB);
		drawContours( contoursImage, contours, -1, Scalar(0,255,0) );
		imshow("contours", contoursImage );
		//std::cout<<contours.size()<<std::endl;ß
	#endif
    
    for (size_t contourIdx = 0; contourIdx < contours.size(); contourIdx++)
    {
        Center center;
        center.confidence = 1;
        Moments moms = moments(contours[contourIdx]);
        #ifdef DEBUG
            //std::cout<<"area: "<<moms.m00<<std::endl;
        #endif
        if (filterByArea)
        {
            double area = moms.m00;
            if (area < minArea || area >= maxArea)
                continue;
        }

        if (filterByCircularity)
        {
            double area = moms.m00;
            double perimeter = arcLength(contours[contourIdx], true);
            double ratio = 4 * CV_PI * area / (perimeter * perimeter);
            if (ratio < minCircularity || ratio >= maxCircularity)
                continue;
        }

        if (filterByInertia)
        {
            double denominator = std::sqrt(std::pow(2 * moms.mu11, 2) + std::pow(moms.mu20 - moms.mu02, 2));
            const double eps = 1e-2;
            double ratio;
            if (denominator > eps)
            {
                double cosmin = (moms.mu20 - moms.mu02) / denominator;
                double sinmin = 2 * moms.mu11 / denominator;
                double cosmax = -cosmin;
                double sinmax = -sinmin;

                double imin = 0.5 * (moms.mu20 + moms.mu02) - 0.5 * (moms.mu20 - moms.mu02) * cosmin - moms.mu11 * sinmin;
                double imax = 0.5 * (moms.mu20 + moms.mu02) - 0.5 * (moms.mu20 - moms.mu02) * cosmax - moms.mu11 * sinmax;
                ratio = imin / imax;
            }
            else
            {
                ratio = 1;
            }

            if (ratio < minInertiaRatio || ratio >= maxInertiaRatio)
                continue;

            center.confidence = ratio * ratio;
        }

        if (filterByConvexity)
        {
            std::vector < Point > hull;
            convexHull(contours[contourIdx], hull);
            double area = contourArea(contours[contourIdx]);
            double hullArea = contourArea(hull);
            if (fabs(hullArea) < DBL_EPSILON)
                continue;
            double ratio = area / hullArea;
            if (ratio < minConvexity || ratio >= maxConvexity)
                continue;
        }

        if(moms.m00 == 0.0)
            continue;
        center.location = Point2d(moms.m10 / moms.m00, moms.m01 / moms.m00);

        if (filterByColor)
        {
            if (binaryImage.at<uchar> (cvRound(center.location.y), cvRound(center.location.x)) != blobColor)
                continue;
        }

        //compute blob radius
        {
            std::vector<double> dists;
            for (size_t pointIdx = 0; pointIdx < contours[contourIdx].size(); pointIdx++)
            {
                Point2d pt = contours[contourIdx][pointIdx];
                dists.push_back(norm(center.location - pt));
            }
            std::sort(dists.begin(), dists.end());
            center.radius = (dists[(dists.size() - 1) / 2] + dists[dists.size() / 2]) / 2.;
        }

        centers.push_back(center);
    }
}

void pupil_detector::detect(const Mat& image, std::vector<cv::KeyPoint>& keypoints) const
{
    keypoints.clear();
    CV_Assert(minRepeatability != 0);
    Mat grayscaleImage;
    if (image.channels() == 3 || image.channels() == 4)
        cvtColor(image, grayscaleImage, COLOR_BGR2GRAY);
    else
        grayscaleImage = image;

    if (grayscaleImage.type() != CV_8UC1) {
        std::cout<<"Blob detector only supports 8-bit images!"<<std::endl;
        //CV_Error(Error::StsUnsupportedFormat, "Blob detector only supports 8-bit images!");
    }

    std::vector < std::vector<Center> > centers;
    
    
    for (double thresh = minThreshold; thresh <= maxThreshold; thresh += thresholdStep)
    {
        Mat binarizedImage;
        threshold(grayscaleImage, binarizedImage, thresh, 255, THRESH_BINARY);
        
        #ifdef DEBUG
        if(thresh == minThreshold) imshow("threshold_min", binarizedImage);
        else if(thresh == maxThreshold) imshow("threshold_max", binarizedImage);
        
        #endif

        std::vector < Center > curCenters;
        findBlobs(grayscaleImage, binarizedImage, curCenters);
        std::vector < std::vector<Center> > newCenters;
        Point2d mid_point(image.cols/2, image.rows/2);
        for (size_t i = 0; i < curCenters.size(); i++)
        {
            bool isNew = true;
            for (size_t j = 0; j < centers.size(); j++)
            {
                double dist = norm(centers[j][ centers[j].size() / 2 ].location - curCenters[i].location);
                isNew = dist >= minDistBetweenBlobs && dist >= centers[j][ centers[j].size() / 2 ].radius && dist >= curCenters[i].radius;
                if (!isNew)
                {
                    centers[j].push_back(curCenters[i]);

                    size_t k = centers[j].size() - 1;
                    while( k > 0 && curCenters[i].radius < centers[j][k-1].radius )
                    {
                        centers[j][k] = centers[j][k-1];
                        k--;
                    }
                    centers[j][k] = curCenters[i];

                    break;
                }
            }
            if (isNew) {
                newCenters.push_back(std::vector<Center> (1, curCenters[i]));
            }
        }
        std::copy(newCenters.begin(), newCenters.end(), std::back_inserter(centers));
    }

    int maxRepeatability = 0;

    for (size_t i = 0; i < centers.size(); i++)
    {
        
        if (centers[i].size() < minRepeatability){
            #ifdef DEBUG
            //std::cout<<"rejected due to minRepeatability"<<centers[i].size()<<std::endl;
            #endif
            continue;
        }else if(centers[i].size() > maxRepeatability){
            keypoints.clear();
            maxRepeatability = centers[i].size();
        }
        Point2d sumPoint(0, 0);
        double normalizer = 0;
        double max_radius_center;
        for (size_t j = 0; j < centers[i].size(); j++)
        {
            sumPoint += centers[i][j].confidence * centers[i][j].location;
            normalizer += centers[i][j].confidence;
            //if(centers[i][j].radius > max_radius_center){
            //    max_radius_center = centers[i][j].radius;
            //    sumPoint = centers[i][j].location;
            //}
        }
        sumPoint *= (1. / normalizer);
        KeyPoint kpt(sumPoint, (float)(centers[i][centers[i].size()/2].radius) * 2.0f);
        keypoints.push_back(kpt);
        #ifdef DEBUG
            //std::cout<<keypoints.size()<<std::endl;
        #endif
    }
}
