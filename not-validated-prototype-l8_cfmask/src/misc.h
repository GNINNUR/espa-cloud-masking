#ifndef MISC_H
#define MISC_H


int prctile
(
    int16 *array, /*I: input data pointer */
    int nums,     /*I: number of input data array */
    int16 min,    /*I: minimum value in the input data array */
    int16 max,    /*I: maximum value in the input data array  */
    float prct,   /*I: percentage threshold */
    float *result /*O: percentile calculated */
);

int prctile2
(
    float *array, /*I: input data pointer */
    int nums,     /*I: number of input data array */
    float min,    /*I: minimum value in the input data array */
    float max,    /*I: maximum value in the input data array  */
    float prct,   /*I: percentage threshold */
    float *result /*O: percentile calculated */
);

int get_args
(
    int argc,            /* I: number of cmd-line args */
    char *argv[],        /* I: string of cmd-line args */
    char **xml_infile,   /* O: address of input XML filename */
    float *cloud_prob,   /* O: cloud_probability input */
    int *cldpix,         /* O: cloud_pixel buffer used for image dilate */
    int *sdpix,          /* O: shadow_pixel buffer used for image dilate  */
    bool *use_l8_cirrus, /* O: use L8 Cirrus cloud bit result flag */
    bool *use_thermal,   /* O: use thermal data flag */
    bool *verbose        /* O: verbose flag */
);

bool is_leap_year
(
    int year /*I: Year to test */
);

bool convert_year_month_day_to_doy
(
    int year,  /* I: Year */
    int month, /* I: Month */
    int day,   /* I: Day of month */
    int *doy   /* O: Day of year */
);

#endif
