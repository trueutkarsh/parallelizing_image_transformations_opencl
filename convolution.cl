__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
 
__kernel void gaussian_blur(
        __read_only image2d_t image,
        // __global uchar* output)
        __write_only image2d_t  outputImage, 
        constant float* mask,
        constant int *dx,
        constant int *dy,
        int width,
        int height,
        int N) 
{     
        int2 pos = (int2) (get_global_id(0), get_global_id(1));
 
        // // blurredImage[pos.x+pos.y*get_global_size(0)] = sum;
        // // float b = read_imagef(image, sampler, pos).x, g = read_imagef(image, sampler, pos).y, r = read_imagef(image, sampler, pos).z;
        // // outputImage[pos.x][pos.y] = 0.30*r + 0.59*g + 0.11*b;

        // int dx[] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
        // int dy[] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};

        float4 result = (float4)(0, 0, 0, 0);

        for(int i=0; i < N; i++)
        {
                result = result + (float4)read_imagef(image, sampler, pos + (int2)(dx[i], dy[i]))*mask[i];
        }

        // float4 v = read_imagef(image, sampler, pos);
        // float4 v2 = read_imagef(image, sampler, pos + 1);
        // float4 v3 = read_imagef(image, sampler, pos - 1);
        // // // float r = v.x*0.11 + v.y*0.59 + v.z*0.30  ;
        // // // float r2 = v2.x*0.11 + v2.y*0.59 + v2.z*0.30  ;
        // float r = v.x   ;

        // float4 b = result ;
        write_imagef(outputImage, pos, result);
        // barrier(CLK_GLOBAL_MEM_FENCE);

        
}

