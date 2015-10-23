#ifndef CONST_H
#define CONST_H

#define PI (3.141592653589793238)

#define TWO_PI (2.0 * PI)
#define HALF_PI (PI / 2.0)

#define DEG (180.0 / PI)
#define RAD (PI / 180.0)

#ifndef SUCCESS
#define SUCCESS  0
#endif

#ifndef FAILURE
#define FAILURE  1
#endif

#ifndef ERROR
#define ERROR   -1
#endif

#define MINSIGMA 1e-5

#define MAX_STR_LEN (510)

#define MAX_DATE_LEN (28)

/* Fill pixel value for the input data */
#define FILL_PIXEL -9999

/* Classification for cfmask algorithm */
#define CF_WATER_BIT 0x01
#define CF_SHADOW_BIT 0x02
#define CF_SNOW_BIT 0x04
#define CF_CLOUD_BIT 0x08
#define CF_FILL_BIT 0x10

/* Clear mask bit definitions */
#define CF_CLEAR_BIT 0x01
#define CF_CLEAR_WATER_BIT 0x02
#define CF_CLEAR_LAND_BIT 0x04

/* Cfmask integer classification values */
#define CF_CLEAR_PIXEL 0
#define CF_WATER_PIXEL 1
#define CF_CLOUD_SHADOW_PIXEL 2
#define CF_SNOW_PIXEL 3
#define CF_CLOUD_PIXEL 4
#define CF_FILL_PIXEL 255

#endif
