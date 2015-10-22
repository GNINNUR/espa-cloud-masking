#ifndef OBJECT_CLOUD_SHADOW_MATCH_H
#define OBJECT_CLOUD_SHADOW_MATCH_H

int object_cloud_shadow_match
(
    Input_t *input,  /*I: input structure */
    float clear_ptm, /*I: percent of clear-sky pixels */
    float t_templ,   /*I: percentile of low background temperature */
    float t_temph,   /*I: percentile of high background temperature */
    int cldpix,      /*I: cloud buffer size */
    int sdpix,       /*I: shadow buffer size */
    unsigned char *pixel_mask, /*I/O:pixel mask */
    bool verbose     /*I: value to indicate if intermediate messages be
                          printed */
);

#endif
