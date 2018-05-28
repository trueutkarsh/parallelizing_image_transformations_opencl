
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/ocl/ocl.hpp>


// cl_mem構造体を参照するためにインクルード
#if defined __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.hpp>
#endif

#include <stdio.h>

#include <iostream>
#include <fstream>
#include <ctime>

using namespace cl;


#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)


int main(int argc, char **argv)
{
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    cl_context context = NULL;
    cl_command_queue command_queue = NULL;
    cl_mem memobj = NULL, n_obj = NULL, h_obj = NULL, r_obj = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret;


    if (argc != 4)
    {
        std::cout<<"Usage ./a.out <kernel file name> <image name> <mask file name> \n";
    }


    FILE *fp;
    const char * fileName = argv[1];
    std::size_t  source_size;
    char *source_str;
    cl_int i;

    // Open Kernel file
    fp = fopen(fileName, "r");
    if (!fp)
    {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char *)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    // Initialize data

    // Retrieve platform and device information
    ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

    // Create OpenCL context
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

    // Create command queue
    command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

    cv::Mat mat_src = cv::imread(argv[2], CV_LOAD_IMAGE_COLOR);
    cv::cvtColor(mat_src, mat_src, CV_BGR2GRAY);
    int width = mat_src.rows, height = mat_src.cols;


    // destination mat
    cv::Mat mat_dst(cv::Size(width, height), CV_8UC4);

    // std::cout<<"width "<<width<<" height" <<height<<std::endl;



    // cl_mem outputImage = clCreateBuffer(context, 
    //                                     CL_MEM_WRITE_ONLY , 
    //                                     sizeof(uchar)*width*height,
    //                                     NULL,
    //                                     NULL);

    // //Allocate memory
    // memobj = clCreateBuffer(context, CL_MEM_READ_WRITE, MEM_SIZE * sizeof(float), NULL, &ret);
    // h_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float), NULL, &ret);
    // n_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int), NULL, &ret);
    // r_obj = clCreateBuffer(context, CL_MEM_READ_WRITE, n * sizeof(float), NULL, &ret);
    // Copy data t allocated memory
    // ret = clEnqueueWriteBuffer(command_queue, memobj, CL_TRUE, 0, MEM_SIZE * sizeof(float), mem, 0, NULL, NULL);
    // ret = clEnqueueWriteBuffer(command_queue, h_obj, CL_TRUE, 0, sizeof(float), &h, 0, NULL, NULL);
    // ret = clEnqueueWriteBuffer(command_queue, n_obj, CL_TRUE, 0, sizeof(int), &n, 0, NULL, NULL);
    // ret = clEnqueueWriteBuffer(command_queue, r_obj, CL_TRUE, 0, n * sizeof(float), r, 0, NULL, NULL);

    // Create program from the input kernel source
    program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const std::size_t *)&source_size, &ret);

    // Build kernel program
    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        std::cout << "build program error.\n";
        std::size_t logLen = 0;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &logLen);
        char *info = new char[logLen];
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, logLen, info, NULL);
        std::cout << "error:\n";
        std::cout << info << std::endl;
        delete[] info;
        return -1;
    }



    int imgsize = mat_src.rows * mat_src.cols * mat_src.channels();
    uchar *buffer = new uchar[imgsize];
    memcpy(buffer, mat_src.data, imgsize);

    static const cl_image_format format1 = {CL_INTENSITY, CL_UNORM_INT8};
    static const cl_image_format format2 = {CL_RGBA, CL_UNORM_INT8};

    cl_mem inputImage = clCreateImage2D(context,
                                        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                        &format1,
                                        width, height, 0,
                                        (buffer),
                                        &ret);

    if (ret != CL_SUCCESS)
    {
        std::cerr << "cannot create image buffer:" << ret << std::endl;
        return -1;
    }

    // delete[] buffer;

    cl_mem outputImage = clCreateImage2D(context,
                                         CL_MEM_WRITE_ONLY,
                                         &format2,
                                         width, height, 0,
                                         NULL,
                                         &ret);

    if (ret != CL_SUCCESS)
    {
        std::cerr << "cannot create image buffer:" << ret << std::endl;
        return -1;
    }

    // declare host and device memeory for mask
    // check for file in argv here (if not exit), read line by line 3 by 3

    // float mask[] = {-1.0f/8, -1.0f/8, -1.0f/8, -1.0f/8, 1, -1.0f/8, -1.0f/8, -1.0f/8, -1.0f/8};
    int m_height, m_width , N;
    float *mask;
    int *dx;
    int *dy;
    std::ifstream maskfile(argv[3]);
    if (maskfile)
    {
        // make changes here read mask size also
        maskfile >> m_height >> m_width;
        
        // check if both of them are even
        if( ! (m_height%2 && m_width%2))
        {
            std::cout << "Invalid mask size. Rows and columns must be odd. \n";
            exit(-1);
        }


        N = m_width*m_height;

        mask = new float[N];
        dx = new int[N];
        dy = new int[N];
        
        for(int i = 0; i < m_height ; i++)
        {
            for(int j = 0; j < m_width ; j++)
            {
                maskfile >> mask[i*m_width + j];
                dx[i * m_width + j] = j - m_width/2;
                dy[i * m_width + j] = i - m_height/2;

            }
        }

    }
    else
    {
        std::cout<<"Cannot open mask file "<<argv[3]<<" .\n";
        exit(-1);
    }

    
    
    cl_mem mask_d = clCreateBuffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(float) * N, mask, &ret);

    if (ret != CL_SUCCESS)
    {
        std::cerr << "cannot create mask buffer:" << ret << std::endl;
        return -1;
    }

    cl_mem dx_d = clCreateBuffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int) * N, dx, &ret);

    if (ret != CL_SUCCESS)
    {
        std::cerr << "cannot create mask buffer:" << ret << std::endl;
        return -1;
    }

    cl_mem dy_d = clCreateBuffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int) * N, dy, &ret);

    if (ret != CL_SUCCESS)
    {
        std::cerr << "cannot create mask buffer:" << ret << std::endl;
        return -1;
    }

    // int dx[] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};

    // cl_mem dx_d = clCreateBuffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(float) * 9, mask, &ret);

    // if (ret != CL_SUCCESS)
    // {
    //     std::cerr << "cannot create mask buffer:" << ret << std::endl;
    //     return -1;
    // }

    // Create Kernel
    kernel = clCreateKernel(program, "gaussian_blur", &ret);

    // Set kernel arguments
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputImage);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&outputImage);
    ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&mask_d);
    ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&dx_d);
    ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&dy_d);
    ret = clSetKernelArg(kernel, 5, sizeof(int), (void *)&width);
    ret = clSetKernelArg(kernel, 6, sizeof(int), (void *)&height);
    ret = clSetKernelArg(kernel, 7, sizeof(int), (void *)&N);
    // add mask size also 
    // ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&n_obj);
    // ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&r_obj);

    // printf("Done\n");
    // Create launch configuraion
    std::size_t global_work_size[2] = {height, width};
    // std::size_t local_work_size[2] = {1, 1};
    // std::size_t local_work_size[3] = {1, 1, 0};

    // Execute kernel
    std::clock_t bt = clock();
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
    float runtime = float(clock() - bt) / CLOCKS_PER_SEC;
    printf("%.7f\n", runtime);
    // SIPL::Image<float> *gradient = new SIPL::Image<float>(width, height);

    // cv::Mat result(cv::Size(width, height), CV_8UC1);




    uchar *buffer2 = mat_dst.data;
    cl::size_t<3> origin, region;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = width;
    region[1] = height;
    region[2] = 1;

    ret = clEnqueueReadImage(command_queue, outputImage, CL_TRUE, origin, region, 0, 0, buffer2, 0, NULL, NULL);
    // ret = clEnqueueReadBuffer(command_queue, outputImage, CL_TRUE, 0, sizeof(uchar)*width*height*4, buffer, 0, NULL, NULL);
    
    if (ret != CL_SUCCESS)
    {
        printf("Error: Failed to read output array! %d\n", ret);
        exit(1);
    }
    // Copy results back
    // cv::Mat result(cv::Size(width, height), CV_8UC4, mat_src.data, cv::Mat::AUTO_STEP);

    // mat_src.convertTo(mat_src, CV_BGRA2RGB);
    // mat_dst.convertTo(mat_dst, CV_BGRA2RGB);

    // cv::imshow("result", result);
    // result.convertTo(result, CV)
    mat_dst.convertTo(mat_dst, CV_BGRA2GRAY);
    // result.convertTo(result, CV_8UC4, CV_IMWRITE_JPEG_QUALITY);

    cv::imwrite("result-convolution.jpeg", mat_dst);
    
    // gradient->display();
    // gradient->save("random.jpeg");

    // Release all
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(memobj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);

    free(source_str);
    delete[] buffer;

    return 0;
}
