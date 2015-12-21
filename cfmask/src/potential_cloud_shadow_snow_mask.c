
#ifdef _OPENMP
    #include <omp.h>
#endif


#include <stdio.h>
#include <stdbool.h>


#include "espa_geoloc.h"


#include "const.h"
#include "error.h"
#include "cfmask.h"
#include "input.h"
#include "misc.h"
#include "fill_local_minima_in_image.h"
#include "potential_cloud_shadow_snow_mask.h"


bool is_fill_data
(
    Input_t * input, /* I: input structure */
    int column,      /* I: column in the input data array */
    bool use_thermal /* I: use the thermal data or not */
)
{
    if (input->buf[BI_BLUE][column] == FILL_PIXEL
        || input->buf[BI_GREEN][column] == FILL_PIXEL
        || input->buf[BI_RED][column] == FILL_PIXEL
        || input->buf[BI_NIR][column] == FILL_PIXEL
        || input->buf[BI_SWIR_1][column] == FILL_PIXEL
        || input->buf[BI_SWIR_2][column] == FILL_PIXEL)
    {
        return true;
    }

    if (use_thermal)
    {
        if (input->buf[BI_THERMAL][column] <= FILL_PIXEL)
            return true;
    }

    return false;
}


bool basic_cloud_test
(
    Input_t * input, /* I: input structure */
    int column,      /* I: column in the input data array */
    float ndvi,      /* I: NDVI value */
    float ndsi,      /* I: NDSI value */
    bool use_thermal /* I: use the thermal data or not */
)
{
    bool result = false;

    if (((ndsi - 0.8) < MINSIGMA)
         && ((ndvi - 0.8) < MINSIGMA)
         && (input->buf[BI_SWIR_2][column] > 300))
    {
        result = true;
    }

    /* If we are using thermal, then the original test was and'ing the thermal
       test */
    if (result && use_thermal)
    {
        if (input->buf[BI_THERMAL][column] < 2700)
        {
            result = true;
        }
        else
        {
            result = false;
        }
    }

    return result;
}


bool basic_snow_test
(
    Input_t * input, /* I: input structure */
    int column,      /* I: column in the input data array */
    float ndsi,      /* I: NDSI value */
    bool use_thermal /* I: use the thermal data or not */
)
{
    bool result = false;

    if (((ndsi - 0.15) > MINSIGMA)
        && (input->buf[BI_NIR][column] > 1100)
        && (input->buf[BI_GREEN][column] > 1000))
    {
        result = true;
    }

    /* If we are using thermal, then the original test was and'ing the thermal
       test */
    if (result && use_thermal)
    {
        if (input->buf[BI_THERMAL][column] < 1000)
        {
            result = true;
        }
        else
        {
            result = false;
        }
    }

    return result;
}


bool zhe_water_test
(
    Input_t * input, /* I: input structure */
    int column,      /* I: column in the input data array */
    float ndvi       /* I: NDVI value */
)
{
    if ((((ndvi - 0.01) < MINSIGMA) && (input->buf[BI_NIR][column] < 1100))
        || (((ndvi - 0.1) < MINSIGMA)
            && (ndvi > MINSIGMA)
            && (input->buf[BI_NIR][column] < 500)))
    {
        return true;
    }

    return false;
}


/*****************************************************************************
MODULE:  potential_cloud_shadow_snow_mask

PURPOSE: Identify the cloud pixels, snow pixels, water pixels, clear land
         pixels, and potential shadow pixels

RETURN: SUCCESS
        FAILURE

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
3/15/2013   Song Guo         Original Development

NOTES:
1. Thermal buffer is expected to be in degrees Celsius with a factor applied
   of 100.  Many values which compare to the thermal buffer in this code are
   hardcoded and assume degrees celsius * 100.
*****************************************************************************/
int potential_cloud_shadow_snow_mask
(
    Input_t * input,            /*I: input structure */
    float cloud_prob_threshold, /*I: cloud probability threshold */
    float *clear_ptm,           /*O: percent of clear-sky pixels */
    float *t_templ,             /*O: percentile of low background temp */
    float *t_temph,             /*O: percentile of high background temp */
    unsigned char *pixel_mask,  /*I/O: pixel mask */
    unsigned char *conf_mask,   /*I/O: confidence mask */
    bool use_cirrus,            /*I: value to inidicate if Cirrus data should
                                     be used */
    bool use_thermal,           /*I: value to indicate if Thermal data should
                                     be used */
    bool verbose                /*I: value to indicate if intermediate
                                     messages should be printed */
)
{
    char *FUNC_NAME = "potential_cloud_shadow_snow_mask";
    char errstr[MAX_STR_LEN];   /* error string */
    int nrows = input->size.l;  /* number of rows */
    int ncols = input->size.s;  /* number of columns */
    int ib = 0;                 /* band index */
    int row = 0;                /* row index */
    int col = 0;                /* column index */
    int image_data_counter = 0;        /* mask counter */
    int clear_pixel_counter = 0;       /* clear sky pixel counter */
    int clear_land_pixel_counter = 0;  /* clear land pixel counter */
    int clear_water_pixel_counter = 0; /* clear water pixel counter */
    float ndvi, ndsi;           /* NDVI and NDSI values */
    int16 *f_temp = NULL;       /* clear land temperature */
    int16 *f_wtemp = NULL;      /* clear water temperature */
    float visi_mean;            /* mean of visible bands */
    float whiteness = 0.0;      /* whiteness value */
    float hot;                  /* hot value for hot test */
    float land_ptm;             /* clear land pixel percentage */
    float water_ptm;            /* clear water pixel percentage */
    unsigned char land_bit;     /* Which clear bit to test all or just land */
    unsigned char water_bit;    /* Which clear bit to test all or just water */
    float l_pt;                 /* low percentile threshold */
    float h_pt;                 /* high percentile threshold */
    float t_wtemp;              /* high percentile water temperature */
    float *wfinal_prob = NULL;  /* final water pixel probabilty value */
    float *final_prob = NULL;   /* final land pixel probability value */
    int t_bright;               /* brightness test value for water */
    float brightness_prob;      /* brightness probability value */
    int t_buffer;               /* temperature test buffer */
    float temp_diff = 0.0;      /* difference of low/high temperature
                                   percentiles */
    float vari_prob;            /* probability from NDVI, NDSI, and whiteness */
    float max_value;            /* maximum value */
    float *prob = NULL;         /* probability value */
    float *wprob = NULL;        /* probability value */
    float clr_mask = 0.0;       /* clear sky pixel threshold */
    float wclr_mask = 0.0;      /* water pixel threshold */
    int data_size;              /* Data size for memory allocation */
    int16 *nir = NULL;          /* near infrared band data */
    int16 *swir1 = NULL;        /* short wavelength infrared band data */
    int16 *nir_data = NULL;          /* Data to be filled */
    int16 *swir1_data = NULL;        /* Data to be filled */
    int16 *filled_nir_data = NULL;   /* Filled result */
    int16 *filled_swir1_data = NULL; /* Filled result */
    float nir_boundary;         /* NIR boundary value / background value */
    float swir1_boundary;       /* SWIR1 boundary value / background value */
    int16 shadow_prob;          /* shadow probability */
    int status;                 /* return value */
    int satu_bv;                /* sum of saturated bands 1, 2, 3 value */

    int pixel_index;
    int pixel_count;

    pixel_count = nrows * ncols;

    /* Dynamic memory allocation */
    unsigned char *clear_mask = NULL;

    clear_mask = calloc(pixel_count, sizeof(unsigned char));
    if (clear_mask == NULL)
    {
        RETURN_ERROR("Allocating mask memory", FUNC_NAME, FAILURE);
    }

    if (verbose)
    {
        printf("The first pass\n");
    }

    /* Loop through each line in the image */
    for (row = 0; row < nrows; row++)
    {
        if (verbose)
        {
            /* Print status on every 1000 lines */
            if (!(row % 1000))
            {
                printf("Processing line %d\r", row);
                fflush(stdout);
            }
        }

        /* For each of the image bands */
        for (ib = 0; ib < input->num_toa_bands; ib++)
        {
            /* Read each input reflective band -- data is read into
               input->buf[ib] */
            if (!GetInputLine(input, ib, row))
            {
                snprintf(errstr, sizeof(errstr),
                         "Reading input image data for line %d, band %d",
                         row, ib);
                RETURN_ERROR(errstr, FUNC_NAME, FAILURE);
            }
        }

        if (use_thermal)
        {
            /* For the thermal band */
            /* Read the input thermal band --
               data is read into input->therm_buf */
            if (!GetInputThermLine(input, row))
            {
                snprintf(errstr, sizeof(errstr),
                         "Reading input thermal data for line %d", row);
                RETURN_ERROR(errstr, FUNC_NAME, FAILURE);
            }
        }

        for (col = 0; col < ncols; col++)
        {
            pixel_index = row * ncols + col;

            int ib;
            for (ib = 0; ib < input->num_toa_bands; ib++)
            {
                if (input->buf[ib][col] == input->meta.satu_value_ref[ib])
                    input->buf[ib][col] = input->meta.satu_value_max[ib];
            }

            if (use_thermal)
            {
                if (input->buf[BI_THERMAL][col] ==
                    input->meta.satu_value_ref[BI_THERMAL])
                {
                    input->buf[BI_THERMAL][col] =
                        input->meta.satu_value_max[BI_THERMAL];
                }
            }

            /* process non-fill pixels only */
            if (is_fill_data(input, col, use_thermal))
            {
                pixel_mask[pixel_index] = CF_FILL_BIT;
                clear_mask[pixel_index] = CF_CLEAR_FILL_BIT;
                continue;
            }
            image_data_counter++;

            if ((input->buf[BI_RED][col] + input->buf[BI_NIR][col]) != 0)
            {
                ndvi = (float)(input->buf[BI_NIR][col]
                               - input->buf[BI_RED][col])
                       / (float)(input->buf[BI_NIR][col]
                                 + input->buf[BI_RED][col]);
            }
            else
                ndvi = 0.01;

            if ((input->buf[BI_GREEN][col] + input->buf[BI_SWIR_1][col]) != 0)
            {
                ndsi = (float)(input->buf[BI_GREEN][col]
                               - input->buf[BI_SWIR_1][col])
                       / (float)(input->buf[BI_GREEN][col]
                                 + input->buf[BI_SWIR_1][col]);
            }
            else
                ndsi = 0.01;

            /* Basic cloud test, equation 1 */
            if (basic_cloud_test(input, col, ndvi, ndsi, use_thermal))
            {
                pixel_mask[pixel_index] |= CF_CLOUD_BIT;
            }
            else
                pixel_mask[pixel_index] &= ~CF_CLOUD_BIT;

            /* It takes every snow pixel including snow pixels under thin
               or icy clouds, equation 20 */
            if (basic_snow_test(input, col, ndsi, use_thermal))
            {
                pixel_mask[pixel_index] |= CF_SNOW_BIT;
            }
            else
                pixel_mask[pixel_index] &= ~CF_SNOW_BIT;

            /* Zhe's water test (works over thin cloud), equation 5 */
            if (zhe_water_test(input, col, ndvi))
            {
                pixel_mask[pixel_index] |= CF_WATER_BIT;
            }
            else
                pixel_mask[pixel_index] &= ~CF_WATER_BIT;

            /* visible bands flatness (sum(abs)/mean < 0.6 => bright and dark
               cloud), equation 2 */
            if (pixel_mask[pixel_index] & CF_CLOUD_BIT)
            {
                visi_mean = (float)(input->buf[BI_BLUE][col]
                                    + input->buf[BI_GREEN][col]
                                    + input->buf[BI_RED][col]) / 3.0;
                if (visi_mean != 0)
                {
                    whiteness =
                        ((fabs ((float)input->buf[BI_BLUE][col] - visi_mean)
                          + fabs ((float)input->buf[BI_GREEN][col] - visi_mean)
                          + fabs ((float)input->buf[BI_RED][col]
                                  - visi_mean))) / visi_mean;
                }
                else
                {
                    /* Just put a large value to remove them from cloud pixel
                       identification */
                    whiteness = 100.0;
                }
            }

            /* Update cloud_mask,  if one visible band is saturated,
               whiteness = 0, due to data type conversion, pixel value
               difference of 1 is possible */
            if ((input->buf[BI_BLUE][col]
                 >= (input->meta.satu_value_max[BI_BLUE] - 1))
                ||
                (input->buf[BI_GREEN][col]
                 >= (input->meta.satu_value_max[BI_GREEN] - 1))
                ||
                (input->buf[BI_RED][col]
                 >= (input->meta.satu_value_max[BI_RED] - 1)))
            {
                whiteness = 0.0;
                satu_bv = 1;
            }
            else
            {
                satu_bv = 0;
            }

            if ((pixel_mask[pixel_index] & CF_CLOUD_BIT) &&
                (whiteness - 0.7) < MINSIGMA)
            {
                pixel_mask[pixel_index] |= CF_CLOUD_BIT;
            }
            else
                pixel_mask[pixel_index] &= ~CF_CLOUD_BIT;

            /* Haze test, equation 3 */
            hot = (float)input->buf[BI_BLUE][col]
                  - 0.5 * (float)input->buf[BI_RED][col]
                  - 800.0;
            if ((pixel_mask[pixel_index] & CF_CLOUD_BIT)
                && (hot > MINSIGMA || satu_bv == 1))
                pixel_mask[pixel_index] |= CF_CLOUD_BIT;
            else
                pixel_mask[pixel_index] &= ~CF_CLOUD_BIT;

            /* Ratio 4/5 > 0.75 test, equation 4 */
            if ((pixel_mask[pixel_index] & CF_CLOUD_BIT) &&
                input->buf[BI_SWIR_1][col] != 0)
            {
                if ((float)input->buf[BI_NIR][col] /
                    (float)input->buf[BI_SWIR_1][col] - 0.75 > MINSIGMA)
                    pixel_mask[pixel_index] |= CF_CLOUD_BIT;
                else
                    pixel_mask[pixel_index] &= ~CF_CLOUD_BIT;
            }
            else
                pixel_mask[pixel_index] &= ~CF_CLOUD_BIT;

            /* Cirrus cloud test */
            if (use_cirrus)
            {
                if ((pixel_mask[pixel_index] & CF_CLOUD_BIT)
                    ||
                    (float)(input->buf[BI_CIRRUS][col] / 400.0 - 0.25)
                    > MINSIGMA)
                {
                    pixel_mask[pixel_index] |= CF_CLOUD_BIT;
                }
                else
                    pixel_mask[pixel_index] &= ~CF_CLOUD_BIT;
            }

            /* Build counters for clear, clear land, and clear water */
            if (pixel_mask[pixel_index] & CF_CLOUD_BIT)
            {
                /* It is cloud so make sure none of the bits are set */
                clear_mask[pixel_index] = CF_CLEAR_NONE;
            }
            else
            {
                clear_mask[pixel_index] = CF_CLEAR_BIT;
                clear_pixel_counter++;

                if (pixel_mask[pixel_index] & CF_WATER_BIT)
                {
                    /* Add the clear water bit */
                    clear_mask[pixel_index] |= CF_CLEAR_WATER_BIT;
                    clear_water_pixel_counter++;
                }
                else
                {
                    /* Add the clear land bit */
                    clear_mask[pixel_index] |= CF_CLEAR_LAND_BIT;
                    clear_land_pixel_counter++;
                }
            }
        }
    }
    printf("\n");

    *clear_ptm = 100.0 * ((float)clear_pixel_counter
                          / (float)image_data_counter);
    land_ptm = 100.0 * ((float)clear_land_pixel_counter
                        / (float)image_data_counter);
    water_ptm = 100.0 * ((float)clear_water_pixel_counter
                         / (float)image_data_counter);

    if (verbose)
    {
        printf("(clear_pixels, clear_land_pixels, clear_water_pixels,"
               " image_data_counter) = (%d, %d, %d, %d)\n",
               clear_pixel_counter, clear_land_pixel_counter,
               clear_water_pixel_counter, image_data_counter);
        printf("(clear_ptm, land_ptm, water_ptm) = (%f, %f, %f)\n",
               *clear_ptm, land_ptm, water_ptm);
    }

    if ((*clear_ptm - 0.1) <= MINSIGMA)
    {
        if (use_thermal)
        {
            /* No thermal test is needed, all clouds */
            *t_templ = -1.0;
            *t_temph = -1.0;
        }

        for (pixel_index = 0; pixel_index < pixel_count; pixel_index++)
        {
            /* All cloud */
            if (!(pixel_mask[pixel_index] & CF_CLOUD_BIT))
                pixel_mask[pixel_index] |= CF_SHADOW_BIT;
            else
                pixel_mask[pixel_index] &= ~CF_SHADOW_BIT;
        }
    }
    else
    {
        if (use_thermal)
        {
            f_temp = calloc(pixel_count, sizeof(int16));
            f_wtemp = calloc(pixel_count, sizeof(int16));
            if (f_temp == NULL || f_wtemp == NULL)
            {
                RETURN_ERROR("Allocating temp memory", FUNC_NAME, FAILURE);
            }
        }

        if (verbose)
        {
            printf("The second pass\n");
        }

        /* Determine which bit to test for land */
        if ((land_ptm - 0.1) >= MINSIGMA)
        {
            /* use clear land only */
            land_bit = CF_CLEAR_LAND_BIT;
        }
        else
        {
            /* not enough clear land so use all clear pixels */
            land_bit = CF_CLEAR_BIT;
        }

        /* Determine which bit to test for water */
        if ((water_ptm - 0.1) >= MINSIGMA)
        {
            /* use clear water only */
            water_bit = CF_CLEAR_WATER_BIT;
        }
        else
        {
            /* not enough clear water so use all clear pixels */
            water_bit = CF_CLEAR_BIT;
        }

        int16 f_temp_max = SHRT_MIN;
        int16 f_temp_min = SHRT_MAX;
        int16 f_wtemp_max = SHRT_MIN;
        int16 f_wtemp_min = SHRT_MAX;
        int land_count = 0;
        int water_count = 0;
        /* Loop through each line in the image */
        for (row = 0; row < nrows; row++)
        {
            if (verbose)
            {
                /* Print status on every 1000 lines */
                if (!(row % 1000))
                {
                    printf("Processing line %d\r", row);
                    fflush(stdout);
                }
            }

            if (use_thermal)
            {
                /* For the thermal band, read the input thermal band  */
                if (!GetInputThermLine(input, row))
                {
                    snprintf(errstr, sizeof(errstr),
                             "Reading input thermal data for line %d", row);
                    RETURN_ERROR(errstr, FUNC_NAME, FAILURE);
                }
            }

            for (col = 0; col < ncols; col++)
            {
                pixel_index = row * ncols + col;

                if (clear_mask[pixel_index] & CF_CLEAR_FILL_BIT)
                    continue;

                if (use_thermal)
                {
                    if (input->buf[BI_THERMAL][col] ==
                        input->meta.satu_value_ref[BI_THERMAL])
                    {
                        input->buf[BI_THERMAL][col] =
                            input->meta.satu_value_max[BI_THERMAL];
                    }
                }

                /* get clear land temperature */
                if (clear_mask[pixel_index] & land_bit)
                {
                    if (use_thermal)
                    {
                        f_temp[land_count] = input->buf[BI_THERMAL][col];
                        if (f_temp_max < f_temp[land_count])
                            f_temp_max = f_temp[land_count];
                        if (f_temp_min > f_temp[land_count])
                            f_temp_min = f_temp[land_count];
                    }
                    land_count++;
                }

                /* get clear water temperature */
                if (clear_mask[pixel_index] & water_bit)
                {
                    if (use_thermal)
                    {
                        f_wtemp[water_count] = input->buf[BI_THERMAL][col];
                        if (f_wtemp_max < f_wtemp[water_count])
                            f_wtemp_max = f_wtemp[water_count];
                        if (f_wtemp_min > f_wtemp[water_count])
                            f_wtemp_min = f_wtemp[water_count];
                    }
                    water_count++;
                }
            }
        }
        printf("\n");

        if (use_thermal)
        {
            /* Set maximum and minimum values to zero if no clear land/water
               pixels */
            if (f_temp_min == SHRT_MAX)
                f_temp_min = 0;
            if (f_temp_max == SHRT_MIN)
                f_temp_max = 0;
            if (f_wtemp_min == SHRT_MAX)
                f_wtemp_min = 0;
            if (f_wtemp_max == SHRT_MIN)
                f_wtemp_max = 0;
        }

        /* Tempearture for snow test */
        l_pt = 0.175;
        h_pt = 1.0 - l_pt;

        if (use_thermal)
        {
            /* 0.175 percentile background temperature (low) */
            status = prctile(f_temp, land_count, f_temp_min, f_temp_max,
                             100.0 * l_pt, t_templ);
            if (status != SUCCESS)
            {
                RETURN_ERROR("Error calling prctile routine",
                             FUNC_NAME, FAILURE);
            }

            /* 0.825 percentile background temperature (high) */
            status = prctile(f_temp, land_count, f_temp_min, f_temp_max,
                             100.0 * h_pt, t_temph);
            if (status != SUCCESS)
            {
                RETURN_ERROR("Error calling prctile routine",
                             FUNC_NAME, FAILURE);
            }

            status = prctile(f_wtemp, water_count, f_wtemp_min, f_wtemp_max,
                             100.0 * h_pt, &t_wtemp);
            if (status != SUCCESS)
            {
                RETURN_ERROR("Error calling prctile routine",
                             FUNC_NAME, FAILURE);
            }

            /* Temperature test */
            t_buffer = 4 * 100;
            *t_templ -= (float)t_buffer;
            *t_temph += (float)t_buffer;
            temp_diff = *t_temph - *t_templ;

            /* Release f_temp memory */
            free(f_wtemp);
            f_wtemp = NULL;
            free(f_temp);
            f_temp = NULL;
        }

        wfinal_prob = calloc(pixel_count, sizeof(float));
        final_prob = calloc(pixel_count, sizeof(float));
        if (wfinal_prob == NULL || final_prob == NULL)
        {
            RETURN_ERROR("Allocating prob memory", FUNC_NAME, FAILURE);
        }

        if (verbose)
        {
            printf("The third pass\n");
        }

        /* Loop through each line in the image */
        for (row = 0; row < nrows; row++)
        {
            if (verbose)
            {
                /* Print status on every 1000 lines */
                if (!(row % 1000))
                {
                    printf("Processing line %d\r", row);
                    fflush(stdout);
                }
            }

            /* For each of the image bands */
            for (ib = 0; ib < input->num_toa_bands; ib++)
            {
                /* Read each input reflective band -- data is read into
                   input->buf[ib] */
                if (!GetInputLine(input, ib, row))
                {
                    snprintf(errstr, sizeof(errstr),
                             "Reading input image data for line %d, band %d",
                             row, ib);
                    RETURN_ERROR(errstr, FUNC_NAME, FAILURE);
                }
            }

            if (use_thermal)
            {
                /* For the thermal band, data is read into input->therm_buf */
                if (!GetInputThermLine(input, row))
                {
                    snprintf(errstr, sizeof(errstr),
                             "Reading input thermal data for line %d", row);
                    RETURN_ERROR(errstr, FUNC_NAME, FAILURE);
                }
            }

            /* Loop through each sample in the image */
            for (col = 0; col < ncols; col++)
            {
                pixel_index = row * ncols + col;

                if (pixel_mask[pixel_index] & CF_FILL_BIT)
                    continue;

                for (ib = 0; ib < NON_CIRRUS_BAND_COUNT; ib++)
                {
                    if (input->buf[ib][col] == input->meta.satu_value_ref[ib])
                        input->buf[ib][col] = input->meta.satu_value_max[ib];
                }

                if (use_thermal)
                {
                    if (input->buf[BI_THERMAL][col] ==
                        input->meta.satu_value_ref[BI_THERMAL])
                    {
                        input->buf[BI_THERMAL][col] =
                            input->meta.satu_value_max[BI_THERMAL];
                    }
                }

                if (pixel_mask[pixel_index] & CF_WATER_BIT)
                {
                    /* Brightness test (over water) */
                    t_bright = 1100;
                    brightness_prob = (float)input->buf[BI_SWIR_1][col] /
                        (float)t_bright;
                    if ((brightness_prob - 1.0) > MINSIGMA)
                        brightness_prob = 1.0;
                    if (brightness_prob < MINSIGMA)
                        brightness_prob = 0.0;

                    if (use_thermal)
                    {
                        /* water temperature probability value */
                        float wtemp_prob;

                        /* Get cloud prob over water */
                        /* Temperature test over water */
                        wtemp_prob =
                            (t_wtemp - (float)input->buf[BI_THERMAL][col])
                            / 400.0;

                        if (wtemp_prob < MINSIGMA)
                            wtemp_prob = 0.0;

                        brightness_prob *= wtemp_prob;
                    }

                    /*Final prob mask (water), cloud over water probability */
                    if (use_cirrus)
                    {
                        wfinal_prob[pixel_index] = 100.0
                            * (brightness_prob
                               + (float)input->buf[BI_CIRRUS][col] / 400.0);
                    }
                    else
                    {
                        wfinal_prob[pixel_index] = 100.0 * brightness_prob;
                    }

                    final_prob[pixel_index] = 0.0;
                }
                else
                {
                    if ((input->buf[BI_RED][col]
                         + input->buf[BI_NIR][col]) != 0)
                    {
                        ndvi = (float)(input->buf[BI_NIR][col]
                                       - input->buf[BI_RED][col])
                               / (float)(input->buf[BI_NIR][col]
                                         + input->buf[BI_RED][col]);
                    }
                    else
                        ndvi = 0.01;

                    if ((input->buf[BI_GREEN][col]
                         + input->buf[BI_SWIR_1][col]) != 0)
                    {
                        ndsi = (float)(input->buf[BI_GREEN][col]
                                       - input->buf[BI_SWIR_1][col])
                               / (float)(input->buf[BI_GREEN][col]
                                         + input->buf[BI_SWIR_1][col]);
                    }
                    else
                        ndsi = 0.01;

                    /* NDVI and NDSI should not be negative */
                    if (ndsi < MINSIGMA)
                        ndsi = 0.0;
                    if (ndvi < MINSIGMA)
                        ndvi = 0.0;

                    visi_mean = (input->buf[BI_BLUE][col]
                                 + input->buf[BI_GREEN][col]
                                 + input->buf[BI_RED][col]) / 3.0;
                    if (visi_mean != 0)
                    {
                        whiteness =
                            ((fabs((float)input->buf[BI_BLUE][col]
                                   - visi_mean)
                              + fabs((float)input->buf[BI_GREEN][col]
                                     - visi_mean)
                              + fabs((float)input->buf[BI_RED][col]
                                     - visi_mean))) / visi_mean;
                    }
                    else
                        whiteness = 0.0;

                    /* If one visible band is saturated, whiteness = 0 */
                    if ((input->buf[BI_BLUE][col]
                         >= (input->meta.satu_value_max[BI_BLUE] - 1))
                        ||
                        (input->buf[BI_GREEN][col]
                         >= (input->meta.satu_value_max[BI_GREEN] - 1))
                        ||
                        (input->buf[BI_RED][col]
                         >= (input->meta.satu_value_max[BI_RED] - 1)))
                    {
                        whiteness = 0.0;
                    }

                    /* Vari_prob=1-max(max(abs(NDSI),abs(NDVI)),whiteness); */
                    if ((ndsi - ndvi) > MINSIGMA)
                        max_value = ndsi;
                    else
                        max_value = ndvi;
                    if ((whiteness - max_value) > MINSIGMA)
                        max_value = whiteness;
                    vari_prob = 1.0 - max_value;

                    if (use_thermal)
                    {
                        /* temperature probability */
                        float temp_prob;

                        temp_prob =
                            (*t_temph - (float)input->buf[BI_THERMAL][col])
                            / temp_diff;

                        /* Temperature can have prob > 1 */
                        if (temp_prob < MINSIGMA)
                            temp_prob = 0.0;

                        vari_prob *= temp_prob;
                    }

                    /*Final prob mask (land) */
                    if (use_cirrus)
                    {
                        final_prob[pixel_index] = 100.0 *
                            (vari_prob + ((float)input->buf[BI_CIRRUS][col]
                                          / 400.0));
                    }
                    else
                    {
                        final_prob[pixel_index] = 100.0 * vari_prob;
                    }

                    wfinal_prob[pixel_index] = 0.0;
                }
            }
        }
        printf("\n");

        /* Allocate memory for prob */
        prob = malloc(pixel_count * sizeof(float));
        if (prob == NULL)
        {
            RETURN_ERROR("Allocating prob memory", FUNC_NAME, FAILURE);
        }

        float prob_max = 0.0;
        float prob_min = 0.0;
        land_count = 0;
        for (pixel_index = 0; pixel_index < pixel_count; pixel_index++)
        {
            if (clear_mask[pixel_index] & CF_CLEAR_FILL_BIT)
                continue;

            if (clear_mask[pixel_index] & land_bit)
            {
                prob[land_count] = final_prob[pixel_index];

                if ((prob[land_count] - prob_max) > MINSIGMA)
                    prob_max = prob[land_count];

                if ((prob_min - prob[land_count]) > MINSIGMA)
                    prob_min = prob[land_count];

                land_count++;
            }
        }

        /* Dynamic threshold for land */
        status = prctile2(prob, land_count, prob_min, prob_max,
                          100.0 * h_pt, &clr_mask);
        if (status != SUCCESS)
        {
            RETURN_ERROR("Error calling prctile2 routine", FUNC_NAME, FAILURE);
        }
        clr_mask += cloud_prob_threshold;

        /* Release memory for prob */
        free(prob);
        prob = NULL;

        /* Allocate memory for wprob */
        wprob = malloc(pixel_count * sizeof(float));
        if (wprob == NULL)
        {
            RETURN_ERROR("Allocating wprob memory", FUNC_NAME, FAILURE);
        }

        float wprob_max = 0.0;
        float wprob_min = 0.0;
        water_count = 0;
        for (pixel_index = 0; pixel_index < pixel_count; pixel_index++)
        {
            if (clear_mask[pixel_index] & CF_CLEAR_FILL_BIT)
                continue;

            if (clear_mask[pixel_index] & water_bit)
            {
                wprob[water_count] = wfinal_prob[pixel_index];

                if ((wprob[water_count] - wprob_max) > MINSIGMA)
                    wprob_max = wprob[water_count];

                if ((wprob_min - wprob[water_count]) > MINSIGMA)
                    wprob_min = wprob[water_count];

                water_count++;
            }
        }

        /* Dynamic threshold for water */
        status = prctile2(wprob, water_count, wprob_min, wprob_max,
                          100.0 * h_pt, &wclr_mask);
        if (status != SUCCESS)
        {
            RETURN_ERROR("Error calling prctile2 routine", FUNC_NAME, FAILURE);
        }
        wclr_mask += cloud_prob_threshold;

        /* Release memory for wprob */
        free(wprob);
        wprob = NULL;

        if (verbose)
        {
            printf("probability threshold (land) = %.2f\n", clr_mask);
            printf("probability threshold (water) = %.2f\n", wclr_mask);

            printf("The fourth pass\n");
        }

        /* Loop through each line in the image */
        for (row = 0; row < nrows; row++)
        {
            if (verbose)
            {
                /* Print status on every 1000 lines */
                if (!(row % 1000))
                {
                    printf("Processing line %d\r", row);
                    fflush(stdout);
                }
            }

            if (use_thermal)
            {
                /* For the thermal band, data is read into input->therm_buf */
                if (!GetInputThermLine(input, row))
                {
                    snprintf(errstr, sizeof(errstr),
                             "Reading input thermal data for line %d", row);
                    RETURN_ERROR(errstr, FUNC_NAME, FAILURE);
                }
            }

            for (col = 0; col < ncols; col++)
            {
                pixel_index = row * ncols + col;

                if (pixel_mask[pixel_index] & CF_FILL_BIT)
                    continue;

                if (use_thermal)
                {
                    if (input->buf[BI_THERMAL][col] ==
                        input->meta.satu_value_ref[BI_THERMAL])
                    {
                        input->buf[BI_THERMAL][col] =
                            input->meta.satu_value_max[BI_THERMAL];
                    }

                    if (input->buf[BI_THERMAL][col]
                        < *t_templ + t_buffer - 3500)
                    {
                        /* This test indicates a high confidence */
                        conf_mask[pixel_index] = CLOUD_CONFIDENCE_HIGH;

                        /* Original code was only this if test and setting the
                           cloud bit or not */
                        pixel_mask[pixel_index] |= CF_CLOUD_BIT;
                    }
                }

                if (conf_mask[pixel_index] == CLOUD_CONFIDENCE_NONE)
                {
                    if (((pixel_mask[pixel_index] & CF_CLOUD_BIT)
                         &&
                         (final_prob[pixel_index] > clr_mask)
                         &&
                         (!(pixel_mask[pixel_index] & CF_WATER_BIT)))
                        ||
                        ((pixel_mask[pixel_index] & CF_CLOUD_BIT)
                         &&
                         (wfinal_prob[pixel_index] > wclr_mask)
                         &&
                         (pixel_mask[pixel_index] & CF_WATER_BIT)))
                    {
                        /* This test indicates a high confidence */
                        conf_mask[pixel_index] = CLOUD_CONFIDENCE_HIGH;

                        /* Original code was only this if test and setting the
                           cloud bit or not */
                        pixel_mask[pixel_index] |= CF_CLOUD_BIT;
                    }
                    else if (((pixel_mask[pixel_index] & CF_CLOUD_BIT)
                              &&
                              (final_prob[pixel_index] > clr_mask-10.0)
                              &&
                              (!(pixel_mask[pixel_index] & CF_WATER_BIT)))
                             ||
                             ((pixel_mask[pixel_index] & CF_CLOUD_BIT)
                              &&
                              (wfinal_prob[pixel_index] > wclr_mask-10.0)
                              &&
                              (pixel_mask[pixel_index] & CF_WATER_BIT)))
                    {
                        /* This test indicates a medium confidence */
                        conf_mask[pixel_index] = CLOUD_CONFIDENCE_MED;

                        /* Don't set the cloud bit per the original code */
                        pixel_mask[pixel_index] &= ~CF_CLOUD_BIT;
                    }
                    else
                    {
                        /* All remaining are a low confidence */
                        conf_mask[pixel_index] = CLOUD_CONFIDENCE_LOW;

                        /* Don't set the cloud bit per the original code */
                        pixel_mask[pixel_index] &= ~CF_CLOUD_BIT;
                    }
                }
            }
        }
        printf("\n");

        /* Free the memory */
        free(wfinal_prob);
        wfinal_prob = NULL;
        free(final_prob);
        final_prob = NULL;

        /* Band NIR & SWIR1 flood fill section */
        data_size = input->size.l * input->size.s;
        nir = calloc(data_size, sizeof(int16));
        swir1 = calloc(data_size, sizeof(int16));
        if (nir == NULL || swir1 == NULL)
        {
            RETURN_ERROR("Allocating nir and swir1 memory",
                         FUNC_NAME, FAILURE);
        }

        if (verbose)
        {
            printf("The fifth pass\n");
        }

        nir_data = calloc(data_size, sizeof(int16));
        swir1_data = calloc(data_size, sizeof(int16));
        filled_nir_data = calloc(data_size, sizeof(int16));
        filled_swir1_data = calloc(data_size, sizeof(int16));
        if (nir_data == NULL || swir1_data == NULL ||
            filled_nir_data == NULL || filled_swir1_data == NULL)
        {
            RETURN_ERROR("Allocating nir and swir1 memory",
                         FUNC_NAME, FAILURE);
        }

        int16 nir_max = 0;
        int16 nir_min = 0;
        int16 swir1_max = 0;
        int16 swir1_min = 0;
        int nir_count = 0;
        int swir1_count = 0;
        /* Loop through each line in the image */
        for (row = 0; row < nrows; row++)
        {
            if (verbose)
            {
                /* Print status on every 1000 lines */
                if (!(row % 1000))
                {
                    printf("Processing line %d\r", row);
                    fflush(stdout);
                }
            }

            /* For each of the image bands */
            for (ib = 0; ib < input->num_toa_bands; ib++)
            {
                /* Read each input reflective band -- data is read into
                   input->buf[ib] */
                if (!GetInputLine(input, ib, row))
                {
                    snprintf(errstr, sizeof(errstr),
                             "Reading input image data for line %d, band %d",
                             row, ib);
                    RETURN_ERROR(errstr, FUNC_NAME, FAILURE);
                }
            }

            for (col = 0; col < ncols; col++)
            {
                pixel_index = row * ncols + col;

                if (clear_mask[pixel_index] & CF_CLEAR_FILL_BIT)
                    continue;

                if (input->buf[BI_NIR][col]
                    == input->meta.satu_value_ref[BI_NIR])
                {
                    input->buf[BI_NIR][col] =
                        input->meta.satu_value_max[BI_NIR];
                }
                if (input->buf[BI_SWIR_1][col]
                    == input->meta.satu_value_ref[BI_SWIR_1])
                {
                    input->buf[BI_SWIR_1][col] =
                        input->meta.satu_value_max[BI_SWIR_1];
                }

                if (clear_mask[pixel_index] & land_bit)
                {
                    nir[nir_count] = input->buf[BI_NIR][col];
                    if (nir[nir_count] > nir_max)
                        nir_max = nir[nir_count];
                    if (nir[nir_count] < nir_min)
                        nir_min = nir[nir_count];
                    nir_count++;

                    swir1[swir1_count] = input->buf[BI_SWIR_1][col];
                    if (swir1[swir1_count] > swir1_max)
                        swir1_max = swir1[swir1_count];
                    if (swir1[swir1_count] < swir1_min)
                        swir1_min = swir1[swir1_count];
                    swir1_count++;
                }
            }

            /* NIR */
            memcpy(&nir_data[row * input->size.s], &input->buf[BI_NIR][0],
                   input->size.s * sizeof(int16));
            /* SWIR1 */
            memcpy(&swir1_data[row * input->size.s], &input->buf[BI_SWIR_1][0],
                   input->size.s * sizeof(int16));
        }
        printf("\n");

        /* Estimating background (land) Band NIR Ref */
        status = prctile(nir, nir_count, nir_min, nir_max,
                         100.0 * l_pt, &nir_boundary);
        if (status != SUCCESS)
        {
            RETURN_ERROR("Calling prctile function", FUNC_NAME, FAILURE);
        }
        status = prctile(swir1, swir1_count, swir1_min, swir1_max,
                         100.0 * l_pt, &swir1_boundary);
        if (status != SUCCESS)
        {
            RETURN_ERROR("Calling prctile function", FUNC_NAME, FAILURE);
        }

        /* Release the memory */
        free(nir);
        free(swir1);
        nir = NULL;
        swir1 = NULL;

        /* Call the fill minima routine to do image fill */
/* Perform them in parallel if threading is enabled */
#ifdef _OPENMP
#pragma omp parallel sections
#endif
{
    {
        if (fill_local_minima_in_image("NIR Band", nir_data,
                                       input->size.l, input->size.s,
                                       nir_boundary, filled_nir_data)
            != SUCCESS)
        {
            printf("Error Running fill_local_minima_in_image on NIR band");
            status = ERROR;
        }
    }

#ifdef _OPENMP
    #pragma omp section
#endif
    {
        if (fill_local_minima_in_image("SWIR1 Band", swir1_data,
                                       input->size.l, input->size.s,
                                       swir1_boundary, filled_swir1_data)
            != SUCCESS)
        {
            printf("Error Running fill_local_minima_in_image on SWIR1 band");
            status = ERROR;
        }
    }
}

        /* Release the memory */
        free(nir_data);
        free(swir1_data);
        nir_data = NULL;
        swir1_data = NULL;

        if (status == ERROR)
        {
            free(filled_nir_data);
            free(filled_swir1_data);
            RETURN_ERROR("Running fill_local_minima_in_image",
                         FUNC_NAME, FAILURE);
        }

        if (verbose)
        {
            printf("The sixth pass\n");
        }

        int16 new_nir;
        int16 new_swir1;
        for (row = 0; row < nrows; row++)
        {
            if (verbose)
            {
                /* Print status on every 1000 lines */
                if (!(row % 1000))
                {
                    printf("Processing line %d\r", row);
                    fflush(stdout);
                }
            }

            /* For each of the image bands */
            for (ib = 0; ib < input->num_toa_bands; ib++)
            {
                /* Read each input reflective band -- data is read into
                   input->buf[ib] */
                if (!GetInputLine(input, ib, row))
                {
                    snprintf(errstr, sizeof(errstr),
                             "Reading input image data for line %d, band %d",
                             row, ib);
                    RETURN_ERROR(errstr, FUNC_NAME, FAILURE);
                }
            }

            for (col = 0; col < ncols; col++)
            {
                pixel_index = row * ncols + col;

                if (input->buf[BI_NIR][col]
                    == input->meta.satu_value_ref[BI_NIR])
                {
                    input->buf[BI_NIR][col] =
                        input->meta.satu_value_max[BI_NIR];
                }
                if (input->buf[BI_SWIR_1][col]
                    == input->meta.satu_value_ref[BI_SWIR_1])
                {
                    input->buf[BI_SWIR_1][col] =
                        input->meta.satu_value_max[BI_SWIR_1];
                }

                if (pixel_mask[pixel_index] & CF_FILL_BIT)
                {
                    conf_mask[pixel_index] = CF_FILL_PIXEL;
                    continue;
                }

                new_nir = filled_nir_data[pixel_index] -
                          input->buf[BI_NIR][col];
                new_swir1 = filled_swir1_data[pixel_index] -
                            input->buf[BI_SWIR_1][col];

                if (new_nir < new_swir1)
                    shadow_prob = new_nir;
                else
                    shadow_prob = new_swir1;

                if (shadow_prob > 200)
                    pixel_mask[pixel_index] |= CF_SHADOW_BIT;
                else
                    pixel_mask[pixel_index] &= ~CF_SHADOW_BIT;

                /* refine Water mask (no confusion water/cloud) */
                if ((pixel_mask[pixel_index] & CF_WATER_BIT) &&
                    (pixel_mask[pixel_index] & CF_CLOUD_BIT))
                {
                    pixel_mask[pixel_index] &= ~CF_WATER_BIT;
                }
            }
        }
        printf("\n");

        /* Release the memory */
        free(nir_data);
        nir_data = NULL;
        free(swir1_data);
        swir1_data = NULL;
        free(filled_nir_data);
        filled_nir_data = NULL;
        free(filled_swir1_data);
        filled_swir1_data = NULL;
    }

    free(clear_mask);
    clear_mask = NULL;

    return SUCCESS;
}
