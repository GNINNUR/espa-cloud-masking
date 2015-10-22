#ifndef INPUT_H
#define INPUT_H

#include "const.h"
#include "cfmask.h"

/* Structure for the metadata */
typedef struct
{
    char sat[MAX_STR_LEN];    /* Satellite */
    int day_of_year;
    float sun_zen;            /* Solar zenith angle (degrees; scene center) */
    float sun_az;             /* Solar azimuth angle (degrees; scene center) */
    float k1;                 /* Thermal K1 constant for BT calculation */
    float k2;                 /* Thermal K2 constant for BT calculation */
    int fill;                 /* Fill value for image data */
    float pixel_size[2];      /* pixel size (x,y) */
    int satu_value_ref[BI_REFL_BAND_COUNT]; /* sat value of TOA products */
    int satu_value_max[BI_REFL_BAND_COUNT]; /* maximum TOA value */
    int therm_satu_value_ref;   /* saturation value of thermal product */
    int therm_satu_value_max;   /* maximum bt value (degrees Celsius) */
    float gain[BI_REFL_BAND_COUNT]; /* Band gain */
    float bias[BI_REFL_BAND_COUNT]; /* Band bias */
    float gain_th;              /* Thermal band gain */
    float bias_th;              /* Thermal band bias */
    float therm_scale_fact;     /* Scale factor for the thermal band */
    Geo_coord_t ul_corner;      /* UL lat/long corner coord */
    Geo_coord_t lr_corner;      /* LR lat/long corner coord */
} Input_meta_t;

/* Structure for the 'input' data type */
typedef struct
{
    Input_meta_t meta;          /* Input metadata */
    int nband;                  /* Number of input TOA reflectance bands */
    Img_coord_int_t size;       /* Input file size */
    char *file_name[BI_REFL_BAND_COUNT]; /* Name of the input TOA image files */
    char *file_name_therm;      /* Name of the input thermal file */
    FILE *fp_bin[BI_REFL_BAND_COUNT]; /* File ptr for TOA refl binary files */
    bool open[BI_REFL_BAND_COUNT];  /* Indicates whether the specific input
                                       TOA reflectance file is open for access;
                                       'true' = open, 'false' = not open */
    int16 *buf[BI_REFL_BAND_COUNT]; /* Input data buffer (one line of data) */
    FILE *fp_bin_therm;         /* File pointer for thermal binary file */
    bool open_therm;            /* Flag to indicate whether the input thermal
                                   file is open for access */
    int16 *therm_buf;           /* Input data buffer (one line of data) */
} Input_t;

/* Prototypes */
Input_t *OpenInput (Espa_internal_meta_t * metadata);
bool GetInputLine (Input_t * this, int iband, int iline);
bool GetInputThermLine (Input_t * this, int iline);
bool CloseInput (Input_t * this);
bool FreeInput (Input_t * this);
bool GetXMLInput (Input_t * this, Espa_internal_meta_t * metadata);

#endif
