#ifndef FILL_LOCAL_MINIMA_IN_IMAGE_H
#define FILL_LOCAL_MINIMA_IN_IMAGE_H

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
