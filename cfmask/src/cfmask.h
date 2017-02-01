#ifndef CFMASK_H
#define CFMASK_H


#define CFMASK_APP_NAME "cfmask"
#define CFMASK_VERSION "2.0.2"


typedef signed short int16;


/* Define the band indexes, used for band indexing into arrays and looping */
#define BI_BLUE                 0
#define BI_GREEN                1
#define BI_RED                  2
#define BI_NIR                  3
#define BI_SWIR_1               4
#define BI_SWIR_2               5
#define TM_REFL_BAND_COUNT      6
#define ETM_REFL_BAND_COUNT     6
#define NON_CIRRUS_BAND_COUNT   6
#define BI_CIRRUS               6
#define OLI_REFL_BAND_COUNT     7
#define OLITIRS_REFL_BAND_COUNT 7
#define NON_THERMAL_BAND_COUNT  7
#define BI_THERMAL              7
#define MAX_BAND_COUNT          8


/* Define for satellite checks */
#define IS_LANDSAT_4 0
#define IS_LANDSAT_5 1
#define IS_LANDSAT_7 2
#define IS_LANDSAT_8 3


/* Define for sensor checks */
#define IS_TM      0
#define IS_ETM     1
#define IS_OLI     2
#define IS_OLITIRS 3


/* Define cloud confidence mask values */
#define CLOUD_CONFIDENCE_NONE 0
#define CLOUD_CONFIDENCE_LOW  1
#define CLOUD_CONFIDENCE_MED  2
#define CLOUD_CONFIDENCE_HIGH 3


void usage ();

void version ();


#endif
