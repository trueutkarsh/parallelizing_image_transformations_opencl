
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
#define MAX_SOURCE_SIZE (0x100000)

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        printf("Usage: ./srgb2gray <image path> \n");
        exit(-1);
    }

    cv::Mat mat_src = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);
    // cv::cvtColor(mat_src, mat_src, CV_BGR2GRAY);
    int width = mat_src.rows, height = mat_src.cols;
    cv::Mat mat_dst(cv::Size(width, height), CV_8UC1);

    Vec3b p;
    float v;
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            p = (Vec3b)mat_src.at<Vec3b>(i, j);
            v = 0.11 * p.val[0] + 0.59 * p.val[1] + 0.30 * p.val[2];
            mat_dst.at<uchar>(i, j) = (uchar)v;
        }
    }

    string result = string(argv[0]) + "-result.jpeg";
    cv::imwrite(result, mat_dst);

    return 0;
}
