#ifndef CFMASK_H
#define CFMASK_H

#define CFMASK_APP_NAME "cfmask"
#define CFMASK_VERSION "1.7.0"

typedef signed short int16;

typedef enum {
    BI_BLUE   = 0,
    BI_GREEN  = 1,
    BI_RED    = 2,
    BI_NIR    = 3,
    BI_SWIR_1 = 4,
    BI_SWIR_2 = 5,
    BI_REFL_BAND_COUNT,
    BI_TIR    = 6,
    BI_BAND_COUNT
} BAND_INDEX;

typedef enum
{
    CLOUD_CONFIDENCE_NONE = 0,
    CLOUD_CONFIDENCE_LOW = 1,
    CLOUD_CONFIDENCE_MED = 2,
    CLOUD_CONFIDENCE_HIGH = 3
} CONFIDENCE_MASK_VALUE;

void usage ();
void version ();

#endif
