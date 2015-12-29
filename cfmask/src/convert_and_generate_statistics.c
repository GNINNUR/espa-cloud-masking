
#ifdef _OPENMP
    #include <omp.h>
#endif


#include <stdio.h>
#include <stdbool.h>


#include "const.h"


/*****************************************************************************
MODULE:  convert_and_generate_statistics

PURPOSE: Convert the pixel_mask from a bit mask to a value mask, and gather
         statistics about the resulting CFmask band data.

RETURN: SUCCESS
        FAILURE
*****************************************************************************/
void convert_and_generate_statistics
(
    bool verbose, /* I: if intermediate messages are to be printed */
    unsigned char *pixel_mask,   /* I/O: pixel mask */
    int pixel_count,             /* I: */
    int data_count,              /* I: */
    float *clear_percent,        /* O: */
    float *cloud_percent,        /* O: */
    float *cloud_shadow_percent, /* O: */
    float *water_percent,        /* O: */
    float *snow_percent          /* O: */
)
{
    int clear_count = 0;
    int cloud_count = 0;
    int cloud_shadow_count = 0;
    int water_count = 0;
    int snow_count = 0;

    int pixel_index = 0;

#ifdef _OPENMP
    #pragma omp parallel for reduction(+:clear_count, cloud_count, cloud_shadow_count, snow_count, water_count)
#endif
    for (pixel_index = 0; pixel_index < pixel_count; pixel_index++)
    {
        if (pixel_mask[pixel_index] & CF_FILL_BIT)
        {
            pixel_mask[pixel_index] = CF_FILL_PIXEL;
        }
        else if (pixel_mask[pixel_index] & CF_CLOUD_BIT)
        {
            pixel_mask[pixel_index] = CF_CLOUD_PIXEL;
            cloud_count++;
        }
        else if (pixel_mask[pixel_index] & CF_SHADOW_BIT)
        {
            pixel_mask[pixel_index] = CF_CLOUD_SHADOW_PIXEL;
            cloud_shadow_count++;
        }
        else if (pixel_mask[pixel_index] & CF_SNOW_BIT)
        {
            pixel_mask[pixel_index] = CF_SNOW_PIXEL;
            snow_count++;
        }
        else if (pixel_mask[pixel_index] & CF_WATER_BIT)
        {
            pixel_mask[pixel_index] = CF_WATER_PIXEL;
            water_count++;
        }
        else
        {
            pixel_mask[pixel_index] = CF_CLEAR_PIXEL;
            clear_count++;
        }
    }

    *clear_percent = 100.0 * (float)clear_count / (float)data_count;
    *cloud_percent = 100.0 * (float)cloud_count / (float)data_count;
    *cloud_shadow_percent = 100.0 * (float)cloud_shadow_count
                                    / (float)data_count;
    *water_percent = 100.0 * (float)water_count / (float)data_count;
    *snow_percent = 100.0 * (float)snow_count / (float)data_count;

    if (verbose)
    {
        printf("CFmask Statistics\n");
        printf("      imagery pixel count = %10d\n", data_count);
        printf("           clear, percent = %10d  %5.2f\n",
               clear_count, *clear_percent);
        printf("           cloud, percent = %10d  %5.2f\n",
               cloud_count, *cloud_percent);
        printf("    cloud shadow, percent = %10d  %5.2f\n",
               cloud_shadow_count, *cloud_shadow_percent);
        printf("           water, percent = %10d  %5.2f\n",
               water_count, *water_percent);
        printf("            snow, percent = %10d  %5.2f\n",
               snow_count, *snow_percent);

        /* report cloud and cloud shadow percent; */
        printf("The cloud and shadow percentage is %5.2f\n",
               100.0 * (float)(cloud_count + cloud_shadow_count)
               / (float)data_count);
    }
}


