#ifndef INPUT_H
#define INPUT_H


#include "const.h"
#include "cfmask.h"


/* Structure for the metadata */
typedef struct
{
    int day_of_year;
    float sun_zen;            /* Solar zenith angle (degrees; scene center) */
    float sun_az;             /* Solar azimuth angle (degrees; scene center) */
    float k1;                 /* Thermal K1 constant for BT calculation */
    float k2;                 /* Thermal K2 constant for BT calculation */
    int fill;                 /* Fill value for image data */
    float pixel_size[2];      /* pixel size (x,y) */
    int satu_value_ref[MAX_BAND_COUNT]; /* sat value of TOA products */
    int satu_value_max[MAX_BAND_COUNT]; /* maximum TOA value */
    float gain[MAX_BAND_COUNT]; /* Band gain */
    float bias[MAX_BAND_COUNT]; /* Band bias */
    float therm_scale_fact;     /* Scale factor for the thermal band */
    Geo_coord_t ul_corner;      /* UL lat/long corner coord */
    Geo_coord_t lr_corner;      /* LR lat/long corner coord */
} Input_meta_t;


/* Structure for the 'input' data type */
typedef struct
{
    Input_meta_t meta;          /* Input metadata */
    int satellite;              /* Specifies the satellite being processed */
    int sensor;                 /* Specifies the sensor being processed */
    int num_toa_bands;          /* Number of input TOA reflectance bands */
    Img_coord_int_t size;       /* Input file size */
    char *file_name[MAX_BAND_COUNT]; /* Name of the input TOA image files */
    FILE *fp_bin[MAX_BAND_COUNT]; /* File ptr for TOA refl binary files */
    bool open[MAX_BAND_COUNT];  /* Indicates whether the specific input
                                   TOA reflectance file is open for access;
                                   'true' = open, 'false' = not open */
    int16 *buf[MAX_BAND_COUNT]; /* Input data buffer (one line of data) */
    float dsun_doy[366];        /* Array of earth/sun distances for each DOY;
                                   read from the EarthSunDistance.txt file */
} Input_t;


/* Prototypes */
Input_t *
OpenInput(Espa_internal_meta_t *metadata, bool use_thermal);

bool
GetInputLine(Input_t *input, int iband, int iline);

bool
GetInputThermLine(Input_t *input, int iline);

bool
CloseInput(Input_t *input);

bool
FreeInput(Input_t *input);

bool
GetXMLInput(Input_t *input, Espa_internal_meta_t *metadata);


#endif
