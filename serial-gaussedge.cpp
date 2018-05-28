
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/ocl/ocl.hpp>

// cl_mem構造体を参照するためにインクルード
// #if defined __APPLE__
// #include <OpenCL/cl.h>
// #else
// #include <CL/cl.hpp>
// #endif

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace cv;

// #define MEM_SIZE (128)s

int main(int argc, char **argv)
{

    if(argc != 2)
    {
        printf("Usage: ./srgb2gray <image path> \n");
        exit(-1);
    }

    cv::Mat mat_src = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);
    cv::cvtColor(mat_src, mat_src, CV_BGR2GRAY);
    int width = mat_src.rows, height = mat_src.cols, nx, ny;
    cv::Mat mat_dst(cv::Size(width, height), CV_8UC1);

    int dx[] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};
    // float mask[] = {1.0/16, 1.0/8, 1.0/16, 1.0/8, 1.0/4, 1.0/8, 1.0/16, 1.0/8, 1.0/16};
    float mask[] = {0.125, 0.125, 0.125, 0.125 , -1, 0.125, 0.125, 0.125, 0.125};
    // float mask[] = {1, 1, 1, 1 , -8, 1, 1, 1, 1};
    
    uchar p, q;
    uchar v;
    for (int i = 0; i < width; i++)
    {
        for(int j = 0; j < height; j++)
        {
            // p = (uchar) mat_src.at<uchar>(i, j);
            // v = 0.11*p.val[0] + 0.59*p.val[1] + 0.30*p.val[2];
            v = 0;
            for(int k = 0; k < 9; k++)
            {
                nx = i + dx[k];
                ny = j + dy[k];
                
                if(nx >= 0 && nx < width && ny>=0 && ny < height)
                {
                    q = (uchar)mat_src.at<uchar>(nx, ny);
                    v = v + q*mask[k];
                }

            }

            mat_dst.at<uchar>(i, j) = (uchar)v;
        }
    }

    string result = string(argv[0]) + "-result.jpeg";
    cv::imwrite(result, mat_dst);

    return 0;
}
