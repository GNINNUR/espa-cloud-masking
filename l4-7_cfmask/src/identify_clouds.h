#ifndef IDENTIFY_CLOUDS_H
#define IDENTIFY_CLOUDS_H

/* Define a run-length encoded structure for storing a run of cloud pixels */
typedef struct rle_t
{
    int row;            /* Image row this record is on */
    int start_col;      /* Image column the pixel run starts on */
    int col_count;      /* Number of pixels in the run */
    int next_index;     /* Index to the next run */
} RLE_T;

int identify_clouds
(
    unsigned char *pixel_mask, /* I: Cloud pixel mask */
    int nrows,                  /* I: Number of rows */
    int ncols,                  /* I: Number of columns */
    RLE_T **out_cloud_runs,     /* O: Array of cloud run-length encoded
                                      segments */
    int **out_cloud_lookup,     /* O: Array to map clouds to cloud runs */
    int **out_cloud_pixel_count,/* O: Cloud number */
    int *out_cloud_count,       /* O: Number of clouds in the cloud_lookup and
                                      cloud_pixel_count arrays */
    int *cloud_map              /* I/O: Image containing the cloud number for
                                        each pixel */
);

#endif
