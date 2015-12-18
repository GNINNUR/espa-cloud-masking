#ifndef POTENTIAL_CLOUD_SHADOW_SNOW_MASK_H
#define POTENTIAL_CLOUD_SHADOW_SNOW_MASK_H


int potential_cloud_shadow_snow_mask
(
    Input_t *input,             /* I: input structure */
    float cloud_prob_threshold, /* I: cloud probability threshold */
    float *clear_ptm,           /* O: percent of clear-sky pixels */
    float *t_templ,             /* O: percentile of low background temp */
    float *t_temph,             /* O: percentile of high background temp */
    unsigned char *pixel_mask,  /* I/O: pixel mask */
    unsigned char *conf_mask,   /* I/O: confidence mask */
    bool use_cirrus,            /* I: value to inidicate if Cirrus data should
                                      be used */
    bool use_thermal,           /* I: value to indicate if Thermal data should
                                      be used */
    bool verbose                /* I: value to indicate if intermediate
                                      messages should be printed */
);


#endif
