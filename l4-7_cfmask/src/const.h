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

/* Fill pixel value for the cfmask data */
#define FILL_VALUE 255

/* Fill pixel value for the input data */
#define FILL_PIXEL -9999

#endif
