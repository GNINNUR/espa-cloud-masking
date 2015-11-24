
#ifdef _OPENMP
    #include <omp.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "espa_geoloc.h"

#include "cfmask.h"
#include "error.h"
#include "input.h"
#include "misc.h"
#include "identify_clouds.h"
#include "object_cloud_shadow_match.h"

#define MAX_CLOUD_TYPE 3000000
#define MIN_CLOUD_OBJ 9


/*****************************************************************************
MODULE:  viewgeo

PURPOSE: Calculate the geometric parameters needed for the cloud/shadow match

RETURN: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
3/15/2013   Song Guo         Original Development
*****************************************************************************/
void viewgeo
(
    int x_ul,         /* I: upper left column */
    int y_ul,         /* I: upper left row */
    int x_ur,         /* I: upper right column */
    int y_ur,         /* I: upper right row */
    int x_ll,         /* I: lower left column  */
    int y_ll,         /* I: lower left row */
    int x_lr,         /* I: lower right column */
    int y_lr,         /* I: lower right row */
    float *a,         /* O: coefficient */
    float *b,         /* O: coefficient */
    float *c,         /* O: coefficient */
    float *omiga_par, /* O: angle of parallel line (in pi) */
    float *omiga_per  /* O: angle of vertical line (in pi) */
)
{
    float x_u, x_l, y_u, y_l;   /* intermediate variables */
    float k_ulr = 1.0;          /* intermediate slope value */
    float k_llr = 1.0;          /* intermediate slope value */
    float k_aver;               /* mean slope value */

    x_u = (float) (x_ul + x_ur) / 2.0;
    x_l = (float) (x_ll + x_lr) / 2.0;
    y_u = (float) (y_ul + y_ur) / 2.0;
    y_l = (float) (y_ll + y_lr) / 2.0;

    if (x_ul != x_ur)
        k_ulr = (float) (y_ul - y_ur) / (float) (x_ul - x_ur);
    /* get k of upper left and right points */
    else
        k_ulr = 0.0;
    if (x_ll != x_lr)
        k_llr = (float) (y_ll - y_lr) / (float) (x_ll - x_lr);
    /* get k of lower left and right points */
    else
        k_llr = 0.0;

    k_aver = (k_ulr + k_llr) / 2.0;
    *omiga_par = atan (k_aver); /* get the angle of parallel lines k (in pi) */

    /* AX(j)+BY(i)+C=0 */
    *a = y_u - y_l;
    *b = x_l - x_u;
    *c = y_l * x_u - x_l * y_u;

    *omiga_per = atan ((*b) / (*a)); /* get the angle which is perpendicular
                                        to the trace line */
}


/*****************************************************************************
MODULE:  mat_truecloud

PURPOSE:  Calculate shadow pixel locations of a true cloud segment

RETURN: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
3/15/2013   Song Guo         Original Development
*****************************************************************************/
void mat_truecloud
(
    int *x,           /*I: input pixel cloumn */
    int *y,           /*I: input pixel row */
    int array_length, /*I: number of input array */
    float *h,         /*I: cloud pixel height */
    float a,          /*I: coefficient */
    float b,          /*I: coefficient */
    float c,          /*I: coefficient */
    float inv_a_b_distance, /*I: precalculated */
    float inv_cos_omiga_per_minus_par,  /*I: precalculated */
    float cos_omiga_par,  /*I: precalculated */
    float sin_omiga_par,  /*I: precalculated */
    float *x_new,     /*O: output pixel cloumn */
    float *y_new      /*O: output pixel row */
)
{
    float dist;                 /* distance */
    float dist_par;             /* distance in parallel direction */
    float dist_move;            /* distance moved */
    float delt_x;               /* change in column */
    float delt_y;               /* change in row */

    float height = 705000.0;    /* average Landsat 4,5,&7 height (m) */
    int i;

    for (i = 0; i < array_length; i++)
    {
        dist = (a * (float) x[i] + b * (float) y[i] + c) * inv_a_b_distance;

        /* from the cetral perpendicular (unit: pixel) */
        dist_par = dist * inv_cos_omiga_per_minus_par;

        /* cloud move distance (m) */
        dist_move = (dist_par * h[i]) / height;

        delt_x = dist_move * cos_omiga_par;
        delt_y = dist_move * sin_omiga_par;

        x_new[i] = x[i] + delt_x;       /* new x, j */
        y_new[i] = y[i] + delt_y;       /* new y, i */
    }
}


/*****************************************************************************
MODULE:  image_dilate

PURPOSE: Dilate the image with a n x n rectangular buffer

RETURN: None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
11/18/2013   Song Guo         Original Development
*****************************************************************************/
void image_dilate
(
    unsigned char *in_mask,    /* I: Mask to be dilated */
    int nrows,                 /* I: Number of rows in the mask */
    int ncols,                 /* I: Number of columns in the mask */
    int idx,                   /* I: Pixel buffer 2 * idx + 1 */
    unsigned char search_type, /* I: Type of data to dilate */
    unsigned char *out_mask    /* O: Mask after dilate */
)
{
    bool found;                /* flag to add the bit to the output mask */
    /* loop indices */
    int row, col;
    int w_row, w_col;      /* window */
    int s_row;             /* start */
    int s_col;             /* start */
    int e_row;             /* end */
    int e_col;             /* end */
    /* locations */
    int row_index;
    int w_row_index;       /* window */
    int out_index;
    int in_index;

#ifdef _OPENMP
    #pragma omp parallel for private(s_row, e_row, row_index, col, out_index, s_col, e_col, found, w_row, w_row_index, w_col, in_index)
#endif
    for (row = 0; row < nrows; row++)
    {
        s_row = row - idx;
        e_row = row + idx;

        row_index = row * ncols;

        for (col = 0; col < ncols; col++)
        {
            out_index = row_index + col;

            /* Skip processing output that is a fill pixel */
            if (out_mask[out_index] & CF_FILL_BIT)
            {
                continue;
            }

            /* Quick check the current pixel */
            if (in_mask[out_index] & search_type)
            {
                out_mask[out_index] |= search_type;
                continue;
            }

            s_col = col - idx;
            e_col = col + idx;

            found = false;
            /* For each row in the window */
            for (w_row = s_row; w_row < e_row + 1 && !found; w_row++)
            {
                /* Skip out of bounds locations */
                if (w_row < 0 || w_row > (nrows - 1))
                    continue;

                w_row_index = w_row * ncols;

                /* For each column in the window */
                for (w_col = s_col; w_col < e_col + 1; w_col++)
                {
                    /* Skip out of bounds locations */
                    if (w_col < 0 || w_col > (ncols - 1))
                        continue;

                    in_index = w_row_index + w_col;

                    if (in_mask[in_index] & search_type)
                    {
                        found = true;
                        break;
                    }
                }
            }

            if (found)
                out_mask[out_index] |= search_type;
            else
                out_mask[out_index] &= ~search_type;
        }
    }
}


/*****************************************************************************
MODULE:  object_cloud_shadow_match

PURPOSE: Identify the final shadow pixels by doing a geometric cloud and
         shadow matching which ends in with the maximum cloud and shadow
         similarity

RETURN: SUCCESS
        FAILURE

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
3/15/2013   Song Guo         Original Development
*****************************************************************************/
int object_cloud_shadow_match
(
    Input_t *input,  /*I: input structure */
    float clear_ptm, /*I: percent of clear-sky pixels */
    float t_templ,   /*I: percentile of low background temp */
    float t_temph,   /*I: percentile of high background temp */
    int cldpix,      /*I: cloud buffer size */
    int sdpix,       /*I: shadow buffer size */
    unsigned char *pixel_mask, /*I/O: pixel mask */
    bool use_thermal,           /*I: value to indicate if thermal data should
                                     be used */
    bool verbose     /*I: value to indicate if intermediate messages
                          be printed */
)
{
    char errstr[MAX_STR_LEN];   /* error string */
    int nrows = input->size.l;  /* number of rows */
    int ncols = input->size.s;  /* number of columns */

    float revised_ptm = 0.0;    /* revised percent of cloud */

    int cloud_count = 0;        /* cloud counter */
    int shadow_count = 0;       /* shadow counter */
    int imagery_pixel_count = 0; /* Count of imagery pixels */
    int cloud_counter = 0;      /* cloud pixel counter */

    int pixel_index;
    int pixel_count;

    pixel_count = nrows * ncols;

#ifdef _OPENMP
    #pragma omp parallel for reduction(+:imagery_pixel_count, cloud_counter)
#endif
    for (pixel_index = 0; pixel_index < pixel_count; pixel_index++)
    {
        /* Skip fill pixels */
        if (pixel_mask[pixel_index] & CF_FILL_BIT)
            continue;

        /* Not fill, so it is part of the imagery */
        imagery_pixel_count++;

        /* How many are cloud */
        if (pixel_mask[pixel_index] & CF_CLOUD_BIT)
            cloud_counter++;
    }

    /* Revised percent of cloud on the scene after plcloud */
    if (imagery_pixel_count != 0)
        revised_ptm = (float) cloud_counter / (float) imagery_pixel_count;
    else
        revised_ptm = 0.0;

    if (verbose)
    {
        printf ("cloud_counter, imagery_pixel_count = %d, %d\n", cloud_counter,
                imagery_pixel_count);
        printf ("Revised percent of cloud = %f\n", revised_ptm);
    }

    /* cloud covers more than 90% of the scene
       => no match => rest are definite shadows */
    if (clear_ptm <= 0.1 || revised_ptm >= 0.90)
    {
        for (pixel_index = 0; pixel_index < pixel_count; pixel_index++)
        {
            /* Skip fill pixels */
            if (pixel_mask[pixel_index] & CF_FILL_BIT)
                continue;

            /* No Shadow Match due to too much cloud (>90 percent)
               non-cloud pixels are just shadow pixels */
            if (!(pixel_mask[pixel_index] & CF_CLOUD_BIT))
            {
                pixel_mask[pixel_index] |= CF_SHADOW_BIT;
            }
        }
    }
    else
    {
        unsigned char *cal_mask = NULL; /* calibration pixel mask */
        int *cloud_map = NULL;      /* Image sized array with cloud numbers */
        int16 *temp_data = NULL;        /* brightness temperature */
        int16 *temp_obj = NULL;         /* temperature for each cloud */

        int index;             /* loop index */
        int row = 0;           /* row index */
        int col = 0;           /* column index */

        int base_h;            /* cloud base height */
        int out_all;           /* total number of pixels outdside boundary */
        int match_all;         /* total number of matched pixels */
        int total_all;         /* total number of pixels */
        int num_clouds;
        int max_cl_height;     /* Max cloud base height (m) */
        int min_cl_height;     /* Min cloud base height (m) */
        int max_height;        /* refined maximum height (m) */
        int min_height;        /* refined minimum height (m) */
        int i_step;            /* iteration step */
        int max_cloud_pixels = 1;
        int x_ul = 0;          /* upper left column */
        int y_ul = 0;          /* upper left row */
        int x_lr = 0;          /* lower right column */
        int y_lr = 0;          /* lower right row */
        int x_ll = 0;          /* lower left column */
        int y_ll = 0;          /* lower left row */
        int x_ur = 0;          /* upper right column */
        int y_ur = 0;          /* upper right row */
        int16 temp_obj_max = 0; /* maximum temperature for each cloud */
        int16 temp_obj_min = 0; /* minimum temperature for each cloud */
        int num_of_real_clouds; /* counter */
        int run_index;          /* Index into the cloud_runs */

        float t_similar;       /* similarity threshold */
        float t_buffer;        /* threshold for matching buffering */
        float max_similar = 0.95; /* max similarity threshold */
        float num_pix = 3.0;   /* number of inward pixes (240m) for cloud base
                                  temperature */
        float inv_rate_elapse = 1.0/6.5; /* inverse wet air lapse rate */
        float inv_rate_dlapse = 1.0/9.8; /* inverse dry air lapse rate */
        float thresh_match;    /* thresh match value */
        float record_thresh;   /* record thresh value */
        float inv_a_b_distance;            /* Inverse of... */
        float inv_cos_omiga_per_minus_par; /* Inverse of... */
        float cos_omiga_par;
        float sin_omiga_par;
        float a, b, c, omiga_par, omiga_per; /* variables used for viewgeo
                                                routine, see it for detail */

        int cloud_type;          /* cloud type iterator */
        int *cloud_orig_row_col; /* Array for original cloud locations */
        int *cloud_orig_row;
        int *cloud_orig_col;
        float *cloud_pos_row_col;
        float *cloud_pos_row;
        float *cloud_pos_col;

        float pixel_size = 30.0; /* pixel size */
        float sun_ele;           /* sun elevation angle */
        float tan_sun_elevation; /* tangent of sun elevation angle */
        float inv_shadow_step;
        float sun_tazi;          /* sun azimuth angle */
        float sun_tazi_rad;      /* sun azimuth angle in radiance */
        float shadow_unit_vec_x;
        float shadow_unit_vec_y;

        float pct_obj;           /* percent of edge pixels */
        float t_obj;             /* cloud percentile value */

        /* Tangent of sun elevation angle */
        sun_ele = 90.0 - input->meta.sun_zen;
        tan_sun_elevation = tan (sun_ele * RAD);

        /* Solar azimuth angle in radians */
        sun_tazi = input->meta.sun_az - 90.0;
        sun_tazi_rad = sun_tazi * RAD;

        if (verbose)
        {
            printf ("Shadow Match processing\n");
            printf ("Shadow match for cloud object >= %d pixels\n",
                    MIN_CLOUD_OBJ);
        }

        i_step = rint (2.0 * pixel_size * tan_sun_elevation);
        /* move 2 pixel at a time */
        if (i_step < (2 * pixel_size))
            i_step = 2 * pixel_size; /* Make i_step = 2 * pixel_size for polar
                                      large solar zenith angle case */

        inv_shadow_step = 1.0 / (pixel_size * tan_sun_elevation);
        shadow_unit_vec_x = cos(sun_tazi_rad);
        shadow_unit_vec_y = sin(sun_tazi_rad);

        /* Get moving direction, the idea is to get the corner rows/cols */
        bool not_found = true;
        for (row = 0; row < nrows && not_found; row++)
        {
            for (col = 0; col < ncols; col++)
            {
                pixel_index = row * ncols + col;

                if (!(pixel_mask[pixel_index] & CF_FILL_BIT))
                {
                    y_ul = row;
                    x_ul = col;
                    not_found = false;
                    break;
                }
            }
        }

        not_found = true;
        for (col = ncols - 1; col >= 0 && not_found; col--)
        {
            for (row = 0; row < nrows; row++)
            {
                pixel_index = row * ncols + col;

                if (!(pixel_mask[pixel_index] & CF_FILL_BIT))
                {
                    y_ur = row;
                    x_ur = col;
                    not_found = false;
                    break;
                }
            }
        }

        not_found = true;
        for (col = 0; col < ncols && not_found; col++)
        {
            for (row = nrows - 1; row >= 0; row--)
            {
                pixel_index = row * ncols + col;

                if (!(pixel_mask[pixel_index] & CF_FILL_BIT))
                {
                    y_ll = row;
                    x_ll = col;
                    not_found = false;
                    break;
                }
            }
        }

        not_found = true;
        for (row = nrows - 1; row >= 0 && not_found; row--)
        {
            for (col = ncols - 1; col >= 0; col--)
            {
                pixel_index = row * ncols + col;

                if (!(pixel_mask[pixel_index] & CF_FILL_BIT))
                {
                    y_lr = row;
                    x_lr = col;
                    not_found = false;
                    break;
                }
            }
        }

        /* get view angle geometry */
        viewgeo (x_ul, y_ul, x_ur, y_ur, x_ll, y_ll, x_lr, y_lr, &a, &b, &c,
                 &omiga_par, &omiga_per);

        /* These don't change so calculate them here */
        inv_a_b_distance = 1 / sqrt (a * a + b * b);
        inv_cos_omiga_per_minus_par = 1 / cos (omiga_per - omiga_par);
        cos_omiga_par = cos (omiga_par);
        sin_omiga_par = sin (omiga_par);

        /* Labeling the cloud pixels */
        printf("Labeling Clouds\n");
        int *cloud_pixel_count = NULL; /* Array for the count of pixels
                                          in each of the identified clouds */
        int *cloud_lookup = NULL; /* Array to point to the first run for each
                                     cloud */
        RLE_T *cloud_runs = NULL; /* Array of cloud run-length encoded
                                     segments */

        /* Allocate memory for array to map image pixels to cloud numbers */
        cloud_map = calloc (pixel_count, sizeof(*cloud_map));
        if (cloud_map == NULL)
        {
            sprintf (errstr, "Allocating memory for cloud map");
            RETURN_ERROR (errstr, "cloud/shadow match", FAILURE);
        }

        if (identify_clouds(pixel_mask, nrows, ncols, &cloud_runs,
                            &cloud_lookup, &cloud_pixel_count, &num_clouds,
                            cloud_map) != SUCCESS)
        {
            free (cloud_map);
            sprintf (errstr, "Failed labeling clouds");
            RETURN_ERROR (errstr, "cloud/shadow match", FAILURE);
        }

        /* If there are no clouds, set the maximum cloud pixels to 1 since
           leaving it at zero would cause problems later */
        if (num_clouds == 0)
            max_cloud_pixels = 1;

        printf("Filtering Clouds\n");
        num_of_real_clouds = 0;
        for (index = 1; index < num_clouds; index++)
        {
            if (cloud_pixel_count[index] <= MIN_CLOUD_OBJ)
            {
                cloud_pixel_count[index] = 0;
                cloud_lookup[index] = -1;
                continue;
            }

            num_of_real_clouds++;

            if (max_cloud_pixels < cloud_pixel_count[index])
                max_cloud_pixels = cloud_pixel_count[index];
        }

        if (verbose)
        {
            printf ("Num of clouds = %d\n", num_clouds);
            printf ("Num of real clouds = %d\n", num_of_real_clouds);
        }

        printf("Finding Shadows\n");
        /* Allocate space for the row/col locations of the height adjusted
           cloud */
        cloud_pos_row_col = malloc (2 * max_cloud_pixels
                                    * sizeof(*cloud_pos_row_col));

        /* Allocate space for the row/col locations of the original cloud */
        cloud_orig_row_col = malloc (2 * max_cloud_pixels
                                     * sizeof(*cloud_orig_row_col));

        if (cloud_pos_row_col == NULL || cloud_orig_row_col == NULL)
        {
            free (cloud_map);
            free (cloud_pos_row_col);
            free (cloud_orig_row_col);
            sprintf (errstr, "Allocating cloud memory");
            RETURN_ERROR (errstr, "cloud/shadow match", FAILURE);
        }

        /* Set up pointers to the row/column info for the cloud row/col info */
        cloud_pos_row = cloud_pos_row_col;
        cloud_pos_col = &cloud_pos_row_col[max_cloud_pixels];
        cloud_orig_row = cloud_orig_row_col;
        cloud_orig_col = &cloud_orig_row_col[max_cloud_pixels];

        if (use_thermal)
        {
            /* Thermal band data allocation */
            temp_data = calloc (pixel_count, sizeof (int16));
            if (temp_data == NULL)
            {
                free (cloud_map);
                free (cloud_pos_row_col);
                free (cloud_orig_row_col);
                sprintf (errstr, "Allocating temp memory");
                RETURN_ERROR (errstr, "cloud/shadow match", FAILURE);
            }

            /* Load the thermal band */
            for (row = 0; row < nrows; row++)
            {
                if (!GetInputThermLine (input, row))
                {
                    free (cloud_map);
                    free (cloud_pos_row_col);
                    free (cloud_orig_row_col);
                    free (temp_data);
                    sprintf (errstr, "Reading input thermal data for line %d",
                             row);
                    RETURN_ERROR (errstr, "cloud/shadow match", FAILURE);
                }
                memcpy (&temp_data[row * ncols], &input->therm_buf[0],
                        ncols * sizeof (int16));
            }

            /* Temperature of the cloud object */
            temp_obj = calloc (max_cloud_pixels, sizeof (int16));
            if (temp_obj == NULL)
            {
                free (cloud_map);
                free (cloud_pos_row_col);
                free (cloud_orig_row_col);
                free (temp_data);
                sprintf (errstr, "Allocating temp_obj memory");
                RETURN_ERROR (errstr, "cloud/shadow match", FAILURE);
            }
        }

        /* Cloud cal mask */
        cal_mask = calloc (pixel_count, sizeof (unsigned char));
        if (cal_mask == NULL)
        {
            free (cloud_map);
            sprintf (errstr, "Allocating cal_mask memory");
            RETURN_ERROR (errstr, "cloud/shadow match", FAILURE);
        }

        /* Cloud_cal pixels are cloud_mask pixels with < 9 pixels removed */
        unsigned char pixel_mask_value;
#ifdef _OPENMP
        #pragma omp parallel for private(pixel_mask_value)
#endif
        for (pixel_index = 0; pixel_index < pixel_count; pixel_index++)
        {
            pixel_mask_value = pixel_mask[pixel_index];

            /* Has not been used yet, so initialize it first */
            cal_mask[pixel_index] = CF_CLEAR_PIXEL;

            if ((pixel_mask_value & CF_CLOUD_BIT)
                && (!(pixel_mask_value & CF_FILL_BIT))
                && (cloud_pixel_count[cloud_map[pixel_index]] != 0))
            {
                cal_mask[pixel_index] |= CF_CLOUD_BIT;
            }
        }

        /* Use iteration to get the optimal move distance, Calulate the
           moving cloud shadow */
        for (cloud_type = 1; cloud_type < num_clouds; cloud_type++)
        {
            float *cloud_height;              /* Cloud height */
            float *matched_height;            /* Best match height values */
            float cloud_radius;               /* Cloud radius */
            short int t_obj_int;              /* Integer object temperature */
            int cloud_pixels = cloud_pixel_count[cloud_type];

            if (cloud_pixels == 0)
                continue;

            /* Update in Fmask v3.3, for larger (> 10% scene area), use
               another set of t_similar and t_buffer to address some
               missing cloud shadow at edge area */
            if (cloud_pixels <= (int) (0.1 * imagery_pixel_count))
            {
                t_similar = 0.3;
                t_buffer = 0.95;
            }
            else
            {
                t_similar = 0.1;
                t_buffer = 0.98;
            }

            /* Build the set of pixels for the current cloud and find the 
               min/max temperatures present in the cloud */
            temp_obj_max = SHRT_MIN;
            temp_obj_min = SHRT_MAX;
            index = 0;
            run_index = cloud_lookup[cloud_type];
            while (run_index != -1)
            {
                RLE_T *run = &cloud_runs[run_index];
                int end_col = run->start_col + run->col_count;

                for (col = run->start_col; col < end_col; col++)
                {
                    if (use_thermal)
                    {
                        temp_obj[index] = temp_data[run->row * ncols + col];

                        if (temp_obj[index] > temp_obj_max)
                            temp_obj_max = temp_obj[index];

                        if (temp_obj[index] < temp_obj_min)
                            temp_obj_min = temp_obj[index];
                    }

                    cloud_orig_col[index] = col;
                    cloud_orig_row[index] = run->row;
                    index++;
                }
                run_index = run->next_index;
            }

            /* Make sure the number of pixels counted is the same as the
               number expected */
            if (index != cloud_pixels)
            {
                free (cloud_map);
                free (cal_mask);
                free (cloud_pos_row_col);
                free (cloud_orig_row_col);
                free (temp_data);
                free (temp_obj);
                sprintf (errstr, "Inconsistent number of pixels found in a"
                         " cloud %d/%d - this is a bug", index, cloud_pixels);
                RETURN_ERROR (errstr, "cloud/shadow match", FAILURE);
            }

            if (use_thermal)
            {
                /* The base temperature for cloud.  Assumes object is round
                   with cloud_radius being the radius of cloud */
                cloud_radius = sqrt (cloud_pixels / (2.0 * PI));

                /* number of inward pixels for correct temperature */
                pct_obj = ((cloud_radius - num_pix) * (cloud_radius - num_pix))
                          / (cloud_radius * cloud_radius);
                if ((pct_obj - 1.0) >= MINSIGMA)
                {
                    /* Use the minimum temperature instead */
                    t_obj = temp_obj_min;
                }
                else if (prctile (temp_obj, cloud_pixels, temp_obj_min,
                                  temp_obj_max, 100.0 * pct_obj, &t_obj)
                         != SUCCESS)
                {
                    free (cloud_map);
                    free (cal_mask);
                    free (cloud_pos_row_col);
                    free (cloud_orig_row_col);
                    free (temp_data);
                    free (temp_obj);
                    RETURN_ERROR ("Error calling prctile",
                                  "cloud/shadow match", FAILURE);
                }
                t_obj_int = rint (t_obj);
            }

            /* refine cloud height range (m) */
            min_cl_height = 200;
            max_cl_height = 12000;

            if (use_thermal)
            {
                min_height =
                    (int) rint (10.0 * (t_templ - t_obj) * inv_rate_dlapse);
                max_height = (int) rint (10.0 * (t_temph - t_obj));

                /* Pick the smallest height range based */
                if (min_cl_height < min_height)
                    min_cl_height = min_height;
                if (max_cl_height > max_height)
                    max_cl_height = max_height;

                /* put the edge of the cloud the same value as t_obj */
#ifdef _OPENMP
                #pragma omp parallel for
#endif
                for (index = 0; index < cloud_pixels; index++)
                {
                    if (temp_obj[index] > t_obj_int)
                        temp_obj[index] = t_obj_int;
                }
            }

            /* Allocate memory for cloud height and matched height */
            cloud_height = malloc (cloud_pixels * sizeof(float));
            matched_height = calloc (cloud_pixels, sizeof(float));
            if (cloud_height == NULL || matched_height == NULL)
            {
                free (cloud_map);
                free (cal_mask);
                free (cloud_pos_row_col);
                free (cloud_orig_row_col);
                free (temp_data);
                free (temp_obj);
                free (cloud_height);
                free (matched_height);
                RETURN_ERROR ("Allocating cloud height memory",
                              "cloud/shadow match", FAILURE);
            }

            /* Initialize height and similarity info */
            record_thresh = 0.0;
            for (base_h = min_cl_height; base_h <= max_cl_height;
                 base_h += i_step)
            {
                if (use_thermal)
                {
#ifdef _OPENMP
                    #pragma omp parallel for
#endif
                    for (index = 0; index < cloud_pixels; index++)
                    {
                        cloud_height[index] =
                            (10.0 * (t_obj - (float) temp_obj[index]))
                            * inv_rate_elapse + (float) base_h;
                    }
                }
                else
                {
                    for (index = 0; index < cloud_pixels; index++)
                    {
                        cloud_height[index] = base_h;
                    }
                }

                /* Get the true postion of the cloud
                   calculate cloud DEM with initial base height */
                mat_truecloud (cloud_orig_col, cloud_orig_row,
                               cloud_pixels, cloud_height, a, b, c,
                               inv_a_b_distance,
                               inv_cos_omiga_per_minus_par,
                               cos_omiga_par, sin_omiga_par,
                               cloud_pos_col, cloud_pos_row);

                float i_xy;
                out_all = 0;
                match_all = 0;
                total_all = 0;
#ifdef _OPENMP
                #pragma omp parallel for private(i_xy, col, row) reduction(+:out_all, match_all, total_all)
#endif
                for (index = 0; index < cloud_pixels; index++)
                {
                    i_xy = cloud_height[index] * inv_shadow_step;

                    /* The check here can assume to handle the south up
                       north down scene case correctly as azimuth angle
                       needs to be added by 180.0 degree */
                    if ((input->meta.sun_az - 180.0) < MINSIGMA)
                    {
                        col = rint (cloud_pos_col[index]
                                    - i_xy * shadow_unit_vec_x);
                        row = rint (cloud_pos_row[index]
                                    - i_xy * shadow_unit_vec_y);
                    }
                    else
                    {
                        col = rint (cloud_pos_col[index]
                                    + i_xy * shadow_unit_vec_x);
                        row = rint (cloud_pos_row[index]
                                    + i_xy * shadow_unit_vec_y);
                    }

                    /* the id that is out of the image */
                    if (row < 0 || row >= nrows || col < 0 || col >= ncols)
                    {
                        out_all++;
                    }
                    else
                    {
                        int c_value = cloud_map[row * ncols + col];
                        unsigned char mask = pixel_mask[row * ncols + col];

                        if ((mask & CF_FILL_BIT)
                            || ((c_value != cloud_type)
                                && (mask & (CF_CLOUD_BIT | CF_SHADOW_BIT))))
                        {
                            match_all++;
                        }
                        if (c_value != cloud_type)
                        {
                            total_all++;
                        }
                    }
                }
                match_all += out_all;
                total_all += out_all;

                thresh_match = (float) match_all / (float) total_all;
                if (((thresh_match - t_buffer * record_thresh) >= MINSIGMA)
                    && (base_h < max_cl_height - i_step)
                    && ((record_thresh - max_similar) < MINSIGMA))
                {
                    if ((thresh_match - record_thresh) > MINSIGMA)
                    {
                        record_thresh = thresh_match;

                        /* Save the new heights */
                        memcpy(matched_height, cloud_height,
                               sizeof(*matched_height) * cloud_pixels);
                    }
                }
                else if ((record_thresh - t_similar) > MINSIGMA)
                {
                    /* Re-calculate the cloud position using the height
                       with the best match */
                    mat_truecloud (cloud_orig_col, cloud_orig_row,
                                   cloud_pixels, matched_height, a, b, c,
                                   inv_a_b_distance,
                                   inv_cos_omiga_per_minus_par,
                                   cos_omiga_par, sin_omiga_par,
                                   cloud_pos_col, cloud_pos_row);

                    float i_vir;
#ifdef _OPENMP
                    #pragma omp parallel for private(i_vir, col, row)
#endif
                    for (index = 0; index < cloud_pixels; index++)
                    {
                        i_vir = matched_height[index] * inv_shadow_step;

                        /* The check here can assume to handle the south
                           up north down scene case correctly as azimuth
                           angle needs to be added by 180.0 degree */
                        if ((input->meta.sun_az - 180.0) < MINSIGMA)
                        {
                            col = rint (cloud_pos_col[index]
                                        - i_vir * shadow_unit_vec_x);
                            row = rint (cloud_pos_row[index]
                                        - i_vir * shadow_unit_vec_y);
                        }
                        else
                        {
                            col = rint (cloud_pos_col[index]
                                        + i_vir * shadow_unit_vec_x);
                            row = rint (cloud_pos_row[index]
                                        + i_vir * shadow_unit_vec_y);
                        }

                        /* put data within range */
                        if (row < 0)
                            row = 0;
                        else if (row >= nrows)
                            row = nrows - 1;
                        if (col < 0)
                            col = 0;
                        else if (col >= ncols)
                            col = ncols - 1;

                        cal_mask[row * ncols + col] |= CF_SHADOW_BIT;
                    }

                    /* Done with this cloud */
                    break;
                }
                else
                {
                    record_thresh = 0.0;
                }
            }

            free (cloud_height);
            cloud_height = NULL;
            free (matched_height);
            matched_height = NULL;
        }

        /* Release memory */
        free (cloud_map);
        cloud_map = NULL;
        free (cloud_pos_row_col);
        cloud_pos_row_col = NULL;
        free (cloud_orig_row_col);
        cloud_orig_row_col = NULL;
        free (temp_data);
        temp_data = NULL;
        free (temp_obj);
        temp_obj = NULL;

        /* Do image dilate for cloud, shadow, snow */
        if (verbose)
           printf ("Performing cloud dilate\n");
        image_dilate (cal_mask, nrows, ncols, cldpix, CF_CLOUD_BIT,
                      pixel_mask);

        if (verbose)
           printf ("Performing cloud shadow dilate\n");
        image_dilate (cal_mask, nrows, ncols, sdpix, CF_SHADOW_BIT,
                      pixel_mask);

        /* Release memory */
        free (cal_mask);
        cal_mask = NULL;
    }

    /* Change pixel_mask to a value mask */
#ifdef _OPENMP
    #pragma omp parallel for reduction(+:cloud_count, shadow_count)
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
            shadow_count++;
        }
        else if (pixel_mask[pixel_index] & CF_SNOW_BIT)
        {
            pixel_mask[pixel_index] = CF_SNOW_PIXEL;
        }
        else if (pixel_mask[pixel_index] & CF_WATER_BIT)
        {
            pixel_mask[pixel_index] = CF_WATER_PIXEL;
        }
        else
        {
            pixel_mask[pixel_index] = CF_CLEAR_PIXEL;
        }
    }

    if (verbose)
    {
        float cloud_shadow_percent; /* cloud shadow percent */

        printf ("cloud_count, shadow_count, imagery_pixel_count = %d,%d,%d\n",
                cloud_count, shadow_count, imagery_pixel_count);

        /* record cloud and cloud shadow percent; */
        cloud_shadow_percent = (float) (cloud_count + shadow_count)
            / (float) imagery_pixel_count;
        printf ("The cloud and shadow percentage is %f\n",
                cloud_shadow_percent);
    }

    return SUCCESS;
}
