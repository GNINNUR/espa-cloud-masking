#ifndef CONVERT_AND_GENERATE_STATISTICS_H
#define CONVERT_AND_GENERATE_STATISTICS_H


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
);


#endif
