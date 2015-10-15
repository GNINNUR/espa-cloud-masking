#ifndef FILL_LOCAL_MINIMA_IN_IMAGE_H
#define FILL_LOCAL_MINIMA_IN_IMAGE_H

#if 0
Kept all of these for now, might want to use them.......... RDD

/* Scaling factor for cfmask */
#define SCALING_FACTOR_DN_TOA 10000
#define SCALING_FACTOR_DN_BT 100

/* Classification for cfmask algorithm */
#define CF_WATER_BIT 0x01
#define CF_SHADOW_BIT 0x02
#define CF_SNOW_BIT 0x04
#define CF_CLOUD_BIT 0x08
#define CF_FILL_BIT 0x10

/* Clear mask bit definitions */
#define CF_CLEAR_BIT 0x01
#define CF_CLEAR_WATER_BIT 0x02
#define CF_CLEAR_LAND_BIT 0x04

/* Cfmask integer classification values */
#define CF_CLEAR_PIXEL 0
#define CF_WATER_PIXEL 1
#define CF_CLOUD_SHADOW_PIXEL 2
#define CF_SNOW_PIXEL 3
#define CF_CLOUD_PIXEL 4
#define CF_FILL_PIXEL 255

/* Indices to the buffers in the input array */
#define CF_BLUE_BAND 0
#define CF_GREEN_BAND 1
#define CF_RED_BAND 2
#define CF_NIR_BAND 3
#define CF_SWIR1_BAND 4
#define CF_SWIR2_BAND 5
#define NBAND_REFL_MAX 6
#endif

int fill_local_minima_in_image
(
    const char *band_name,     /* I: Band name being filled */
    const short int *in_img,   /* I: Input image buffer */
    int num_rows,              /* I: Number of rows in the input image */
    int num_cols,              /* I: Number of columns in the input image */
    float boundary_val,        /* I: Background (land) percentage */ 
    short int *out_img         /* O: Output image buffer */
);

#endif /* FILL_LOCAL_MINIMA_IN_IMAGE_H */
