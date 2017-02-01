/*****************************************************************************
!File: input.c
*****************************************************************************/

#include <math.h>

#include "espa_metadata.h"
#include "espa_geoloc.h"
#include "raw_binary_io.h"

#include "const.h"
#include "error.h"
#include "cfmask.h"
#include "misc.h"
#include "input.h"


/*****************************************************************************
MODULE:  dn_to_bt_saturation

PURPOSE:  Convert saturated Digital Number to Brightness Temperature

NOTES: The constants and formulas used are from BU's matlab code
       & G. Chander et al. RSE 113 (2009) 893-903
*****************************************************************************/
void
dn_to_bt_saturation
(
    Input_t *input /* I: input reflectance band data */
)
{
    float k1 = 0.0;
    float k2 = 0.0;
    float max_dn = 0.0;
    float temp;

    if (input->satellite == IS_LANDSAT_8)
    {
        k1 = input->meta.k1;
        k2 = input->meta.k2;
        max_dn = 65535;
    }
    else if (input->satellite == IS_LANDSAT_7)
    {
        k1 = 666.09;
        k2 = 1282.71;
        max_dn = 255;
    }
    else if (input->satellite == IS_LANDSAT_5)
    {
        k1 = 607.76;
        k2 = 1260.56;
        max_dn = 255;
    }
    else if (input->satellite == IS_LANDSAT_4)
    {
        k1 = 671.62;
        k2 = 1284.30;
        max_dn = 255;
    }

    temp = input->meta.gain[BI_THERMAL] * max_dn
           + input->meta.bias[BI_THERMAL];
    temp = k2 / log ((k1 / temp) + 1.0);
    /* Convert from Kelvin back to degrees Celsius since the application is
       based on the unscaled Celsius values originally produced. */
    input->meta.satu_value_max[BI_THERMAL] =
        (int)round(100.0 * (temp - 273.15));
}


/*****************************************************************************
MODULE:  dn_to_toa_saturation

PURPOSE: Convert saturated Digital Number to TOA reflectance

NOTES: The constants and formulas used are from BU's matlab code
       & G. Chander et al. RSE 113 (2009) 893-903
*****************************************************************************/
void
dn_to_toa_saturation
(
    Input_t *input /* I: input reflectance band data */
)
{
    int band_index;
    float max_dn;
    float temp;

    if (input->satellite == IS_LANDSAT_8)
    {
        max_dn = 65535;

        for (band_index = 0; band_index < input->num_toa_bands; band_index++)
        {
            temp = input->meta.gain[band_index] * max_dn
                   + input->meta.bias[band_index];
            input->meta.satu_value_max[band_index] =
                (int)round((10000.0 * temp)
                           / cos(input->meta.sun_zen * (PI / 180.0)));
        }
    }
    else
    {
        /* mean solar exoatmospheric spectral irradiance for each band */
        float esun[ETM_REFL_BAND_COUNT];
        float sun_zen_deg = cos(input->meta.sun_zen * (PI / 180.0));

        max_dn = 255;

        if (input->satellite == IS_LANDSAT_7)
        {
            esun[BI_BLUE] = 1997.0;
            esun[BI_GREEN] = 1812.0;
            esun[BI_RED] = 1533.0;
            esun[BI_NIR] = 1039.0;
            esun[BI_SWIR_1] = 230.8;
            esun[BI_SWIR_2] = 84.9;
        }
        else if (input->satellite == IS_LANDSAT_5)
        {
            esun[BI_BLUE] = 1983.0;
            esun[BI_GREEN] = 1796.0;
            esun[BI_RED] = 1536.0;
            esun[BI_NIR] = 1031.0;
            esun[BI_SWIR_1] = 220.0;
            esun[BI_SWIR_2] = 83.44;
        }
        else if (input->satellite == IS_LANDSAT_4)
        {
            esun[BI_BLUE] = 1983.0;
            esun[BI_GREEN] = 1795.0;
            esun[BI_RED] = 1539.0;
            esun[BI_NIR] = 1028.0;
            esun[BI_SWIR_1] = 219.8;
            esun[BI_SWIR_2] = 83.49;
        }

        for (band_index = 0; band_index < input->num_toa_bands; band_index++)
        {
            temp = input->meta.gain[band_index] * max_dn
                   + input->meta.bias[band_index];
            input->meta.satu_value_max[band_index] =
                (int)round((10000.0 * PI * temp
                            * input->dsun_doy[input->meta.day_of_year - 1]
                            * input->dsun_doy[input->meta.day_of_year - 1])
                           / (esun[band_index] * sun_zen_deg));
        }
    }
}


/*****************************************************************************
MODULE:  OpenInput

PURPOSE: Sets up the 'input' data structure, opens the input file for read
         access, allocates space, and stores some of the metadata.

RETURN: Type = Input_t *
    A populated Input_t data structure or NULL when an error occurs
*****************************************************************************/
Input_t *
OpenInput
(
    Espa_internal_meta_t *metadata, /* I: input metadata */
    bool use_thermal                /* I: value to indicate if thermal data
                                          should be used */
)
{
    Input_t *input = NULL;
    char *error_string = NULL;
    int band_index;
    int esun_index;
    FILE *dsun_fd = NULL; /* EarthSunDistance.txt file pointer */
    char *esun_path = NULL;
    char full_path[PATH_MAX];

    /* Check the environment first */
    esun_path = getenv("ESUN");
    if (esun_path == NULL)
    {
        error_string = "ESUN environment variable is not set";
        RETURN_ERROR(error_string, "OpenInput", NULL);
    }

    /* Create the Input data structure */
    input = malloc(sizeof(Input_t));
    if (input == NULL)
    {
        RETURN_ERROR("allocating Input data structure", "OpenInput", NULL);
    }

    /* Initialize some of the array structure elements */
    for (band_index = 0; band_index < MAX_BAND_COUNT; band_index++)
    {
        input->file_name[band_index] = NULL;
        input->fp_bin[band_index] = NULL;
        input->open[band_index] = false;
        /* Initialize to NULL, memory is allocated later */
        input->buf[band_index] = NULL;
    }

    /* Initialize and get input from header file */
    if (!GetXMLInput(input, metadata))
    {
        free(input);
        input = NULL;
        RETURN_ERROR("getting input from header file", "OpenInput", NULL);
    }

    /* Open TOA reflectance files for access */
    for (band_index = 0; band_index < input->num_toa_bands; band_index++)
    {
        printf("Band %d Filename: %s\n",
               band_index, input->file_name[band_index]);
        input->fp_bin[band_index] =
            open_raw_binary(input->file_name[band_index], "r");
        if (input->fp_bin[band_index] == NULL)
        {
            RETURN_ERROR("opening input TOA binary file", "OpenInput", NULL);
        }
        input->open[band_index] = true;
    }

    if (use_thermal)
    {
        /* Open thermal file for access */
        printf("Thermal Band Filename: %s\n",
               input->file_name[BI_THERMAL]);
        input->fp_bin[BI_THERMAL] =
            open_raw_binary(input->file_name[BI_THERMAL], "r");
        if (input->fp_bin[BI_THERMAL] == NULL)
        {
            error_string = "opening thermal binary file";
        }
        else
        {
            input->open[BI_THERMAL] = true;
        }
    }
    else
    {
        input->fp_bin[BI_THERMAL] = NULL;
        input->open[BI_THERMAL] = false;
    }

    /* Allocate input buffers.  Thermal band only has one band.  Image and QA
       buffers have multiple bands. */
    for (band_index = 0; band_index < input->num_toa_bands; band_index++)
    {
        input->buf[band_index] = calloc(input->size.s, sizeof(int16));
        if (input->buf[band_index] == NULL)
        {
            error_string = "allocating input band buffer";
        }
    }

    if (use_thermal)
    {
        input->buf[BI_THERMAL] = calloc(input->size.s, sizeof(int16));
        if (input->buf[BI_THERMAL] == NULL)
        {
            error_string = "allocating input thermal band buffer";
        }
    }
    else
    {
        input->buf[BI_THERMAL] = NULL;
    }

    snprintf(full_path, sizeof(full_path), "%s/%s",
             esun_path, "EarthSunDistance.txt");
    dsun_fd = fopen(full_path, "r");
    if (dsun_fd == NULL)
    {
        error_string = "Can't open EarthSunDistance.txt file";
    }

    for (esun_index = 0; esun_index < 366; esun_index++)
    {
        if (fscanf(dsun_fd, "%f", &input->dsun_doy[esun_index]) == EOF)
        {
            error_string = "End of file (EOF) is met before 336 lines";
        }
    }
    fclose(dsun_fd);
    dsun_fd = NULL;

    if (error_string != NULL)
    {
        FreeInput(input);
        CloseInput(input);
        RETURN_ERROR(error_string, "OpenInput", NULL);
    }

    if (input->satellite != IS_LANDSAT_8)
    {
        /* Landsat 8 doesn't have saturation issues */
        /* Calculate maximum TOA reflectance values and put them in metadata */
        dn_to_toa_saturation(input);

        if (use_thermal)
        {
            /* Calculate maximum BT values and put them in metadata */
            dn_to_bt_saturation(input);
        }
    }

    return input;
}


/*****************************************************************************
MODULE:  CloseInput

PURPOSE: Ends access and closes the input files.

RETURN:  Type = Bool,  Updated Input_T data structure.
    Input_t: The following field is modified: open
    Value  Description
    -----  -------------------------------------------------------------------
    true   No Errors
    false  Errors encountered
*****************************************************************************/
bool
CloseInput
(
    Input_t *input /* I: input reflectance band data */
)
{
    int band_index;
    bool none_open;

    if (input != NULL)
    {
        none_open = true;
        for (band_index = 0; band_index < input->num_toa_bands; band_index++)
        {
            if (input->open[band_index])
            {
                none_open = false;
                close_raw_binary(input->fp_bin[band_index]);
                input->open[band_index] = false;
            }
        }

        if (input->open[BI_THERMAL])
        {
            close_raw_binary(input->fp_bin[BI_THERMAL]);
            input->open[BI_THERMAL] = false;
        }

        if (none_open)
        {
            RETURN_ERROR("no files open", "CloseInput", false);
        }
    }

    return true;
}


/*****************************************************************************
MODULE:  FreeInput

PURPOSE: Frees the Input_t data structure memory.

RETURN:  Type = Bool,  Updated Input_T data structure.
    Input_t:  Memory fields reset to NULL
    Value  Description
    -----  -------------------------------------------------------------------
    true   No Errors
    false  Errors encountered
*****************************************************************************/
bool
FreeInput
(
    Input_t *input /* I: input reflectance band data */
)
{
    int band_index;

    if (input != NULL)
    {
        for (band_index = 0; band_index < MAX_BAND_COUNT; band_index++)
        {
            if (input->open[band_index])
            {
                RETURN_ERROR("file still open", "FreeInput", false);
            }
            free(input->file_name[band_index]);
            input->file_name[band_index] = NULL;
            free(input->buf[band_index]);
        }

        free(input);
        input = NULL;
    }

    return true;
}


/*****************************************************************************
MODULE:  GetInputLine

PURPOSE: Reads the data for the current band and line

RETURN:  Type = Bool,  Updated Input_T data structure.
    Input_t:  Updated memory buffer for the specified band
    Value  Description
    -----  -------------------------------------------------------------------
    true   No Errors
    false  Errors encountered
*****************************************************************************/
bool
GetInputLine
(
    Input_t *input, /* I: input reflectance band data */
    int band_index, /* I: the band to read */
    int iline       /* I: the line to read in the band */
)
{
    void *buf = NULL;
    long loc; /* pointer location in the raw binary file */

    /* Check the parameters */
    if (input == NULL)
    {
        RETURN_ERROR("invalid input structure", "GetIntputLine", false);
    }
    if (band_index < 0 || band_index >= input->num_toa_bands)
    {
        RETURN_ERROR("invalid band number", "GetInputLine", false);
    }
    if (!input->open[band_index])
    {
        RETURN_ERROR("file not open", "GetInputLine", false);
    }
    if (iline < 0 || iline >= input->size.l)
    {
        RETURN_ERROR("invalid line number", "GetInputLine", false);
    }

    /* Read the data */
    buf = input->buf[band_index];
    loc = (long)(iline * input->size.s * sizeof(int16));
    if (fseek(input->fp_bin[band_index], loc, SEEK_SET))
    {
        RETURN_ERROR("error seeking line (binary)", "GetInputLine", false);
    }

    if (read_raw_binary(input->fp_bin[band_index], 1, input->size.s,
                        sizeof(int16), buf) != SUCCESS)
    {
        RETURN_ERROR("error reading line (binary)", "GetInputLine", false);
    }

    return true;
}

/*****************************************************************************
MODULE:  GetInputThermLine

PURPOSE: Reads the thermal brightness data for the current line.

RETURN:  Type = Bool,  Updated Input_T data structure.
    Input_t:  Updated memory buffer for the specified band
    Value  Description
    -----  -------------------------------------------------------------------
    true   No Errors
    false  Errors encountered
*****************************************************************************/
bool
GetInputThermLine
(
    Input_t *input, /* I: input reflectance band data */
    int iline       /* I: the line to read in the band */
)
{
    void *buf = NULL;
    int i;            /* looping variable */
    long loc;         /* pointer location in the raw binary file */
    float therm_val;  /* tempoary thermal value for conversion from Kelvin to
                         Celsius */

    /* Check the parameters */
    if (input == NULL)
    {
        RETURN_ERROR("invalid input structure", "GetIntputThermLine", false);
    }
    if (!input->open[BI_THERMAL])
    {
        RETURN_ERROR("file not open", "GetInputThermLine", false);
    }
    if (iline < 0 || iline >= input->size.l)
    {
        RETURN_ERROR("invalid line number", "GetInputThermLine", false);
    }

    /* Read the data */
    buf = input->buf[BI_THERMAL];
    loc = (long) (iline * input->size.s * sizeof(int16));
    if (fseek(input->fp_bin[BI_THERMAL], loc, SEEK_SET))
    {
        RETURN_ERROR("error seeking thermal line (binary)",
                     "GetInputThermLine", false);
    }

    if (read_raw_binary(input->fp_bin[BI_THERMAL], 1, input->size.s,
                        sizeof (int16), buf) != SUCCESS)
    {
        RETURN_ERROR("error reading thermal line (binary)",
                     "GetInputThermLine", false);
    }

    /* Convert from Kelvin back to degrees Celsius since the application is
       based on the unscaled Celsius values originally produced.  If input is
       fill or saturated, then leave as fill or saturated. */
    for (i = 0; i < input->size.s; i++)
    {
        if (input->buf[BI_THERMAL][i] != input->meta.fill &&
            input->buf[BI_THERMAL][i] != input->meta.satu_value_ref[BI_THERMAL])
        {
            /* unscale and convert to celsius */
            therm_val = input->buf[BI_THERMAL][i] * input->meta.therm_scale_fact;
            therm_val -= 273.15;

            /* apply the old scale factor that the cfmask processing is based
               upon, to get the original unscaled Celsius values */
            therm_val *= 100.0;
            input->buf[BI_THERMAL][i] = (int)round(therm_val);
        }
    }

    return true;
}


#define DATE_STRING_LEN (50)
#define TIME_STRING_LEN (50)

/*****************************************************************************
MODULE:  GetXMLInput

PURPOSE:  Pulls input values from the XML structure.

RETURN:  Type = Bool
    Value  Description
    -----  -------------------------------------------------------------------
    true   No Errors
    false  Errors encountered
*****************************************************************************/
bool
GetXMLInput
(
    Input_t *input,                /* I: input reflectance band data */
    Espa_internal_meta_t *metadata /* I: input metadata */
)
{
    char *error_string = NULL;
    int band_index;
    char acq_date[DATE_STRING_LEN + 1];
    char acq_time[TIME_STRING_LEN + 1];
    int i;
    int ref_index = -1; /* band index in XML file to use as the reference
                           band */
    Espa_global_meta_t *gmeta = &metadata->global;
    int year;
    int month;
    int day;
    char *meta_band_name[MAX_BAND_COUNT];
    char *meta_band_toa_name[MAX_BAND_COUNT];

    /* Initialize the input fields */
    input->num_toa_bands = 0;

    /* Pull the appropriate data from the XML file */
    snprintf(acq_date, sizeof(acq_date), "%s", gmeta->acquisition_date);
    snprintf(acq_time, sizeof(acq_time), "%s", gmeta->scene_center_time);

    /* Make sure the acquisition time is not too long (i.e. contains too
       many decimal points for the date/time routines).  The time should be
       hh:mm:ss.ssssssZ (see DATE_FORMAT_DATEA_TIME in date.h) which is 16
       characters long.  If the time is longer than that, just chop it off. */
    if (strlen(acq_time) > 16)
    {
        snprintf(&acq_time[15], 2, "Z");
    }

    input->meta.sun_zen = gmeta->solar_zenith;
    if (input->meta.sun_zen < -90.0 || input->meta.sun_zen > 90.0)
    {
        error_string = "solar zenith angle out of range";
        RETURN_ERROR(error_string, "GetXMLInput", false);
    }

    input->meta.sun_az = gmeta->solar_azimuth;
    if (input->meta.sun_az < -360.0 || input->meta.sun_az > 360.0)
    {
        error_string = "solar azimuth angle out of range";
        RETURN_ERROR(error_string, "GetXMLInput", false);
    }

    /* Get the geographic coordinates */
    input->meta.ul_corner.lat = gmeta->ul_corner[0];
    input->meta.ul_corner.lon = gmeta->ul_corner[1];
    input->meta.lr_corner.lat = gmeta->lr_corner[0];
    input->meta.lr_corner.lon = gmeta->lr_corner[1];

    if (strcmp(gmeta->satellite, "LANDSAT_8") == 0)
    {
        input->satellite = IS_LANDSAT_8;
    }
    else if (strcmp(gmeta->satellite, "LANDSAT_7") == 0)
    {
        input->satellite = IS_LANDSAT_7;
    }
    else if (strcmp(gmeta->satellite, "LANDSAT_5") == 0)
    {
        input->satellite = IS_LANDSAT_5;
    }
    else if (strcmp(gmeta->satellite, "LANDSAT_4") == 0)
    {
        input->satellite = IS_LANDSAT_4;
    }
    else
    {
        error_string = "invalid satellite";
        RETURN_ERROR(error_string, "GetXMLInput", false);
    }

    /* Determine the number of TOA reflectance bands */
    if (!strcmp(gmeta->instrument, "OLI"))
    {
        input->sensor = IS_OLI;
        input->num_toa_bands = OLI_REFL_BAND_COUNT;

        meta_band_name[BI_BLUE] = strdup("band2");
        meta_band_toa_name[BI_BLUE] = strdup("toa_band2");

        meta_band_name[BI_GREEN] = strdup("band3");
        meta_band_toa_name[BI_GREEN] = strdup("toa_band3");

        meta_band_name[BI_RED] = strdup("band4");
        meta_band_toa_name[BI_RED] = strdup("toa_band4");

        meta_band_name[BI_NIR] = strdup("band5");
        meta_band_toa_name[BI_NIR] = strdup("toa_band5");

        meta_band_name[BI_SWIR_1] = strdup("band6");
        meta_band_toa_name[BI_SWIR_1] = strdup("toa_band6");

        meta_band_name[BI_SWIR_2] = strdup("band7");
        meta_band_toa_name[BI_SWIR_2] = strdup("toa_band7");
        /* Cirrus */
        meta_band_name[BI_CIRRUS] = strdup("band9");
        meta_band_toa_name[BI_CIRRUS] = strdup("toa_band9");
        /* Thermal */
        meta_band_name[BI_THERMAL] = strdup("NA");
        meta_band_toa_name[BI_THERMAL] = strdup("NA");
    }
    else if (!strcmp(gmeta->instrument, "OLI_TIRS"))
    {
        input->sensor = IS_OLITIRS;
        input->num_toa_bands = OLITIRS_REFL_BAND_COUNT;

        meta_band_name[BI_BLUE] = strdup("band2");
        meta_band_toa_name[BI_BLUE] = strdup("toa_band2");

        meta_band_name[BI_GREEN] = strdup("band3");
        meta_band_toa_name[BI_GREEN] = strdup("toa_band3");

        meta_band_name[BI_RED] = strdup("band4");
        meta_band_toa_name[BI_RED] = strdup("toa_band4");

        meta_band_name[BI_NIR] = strdup("band5");
        meta_band_toa_name[BI_NIR] = strdup("toa_band5");

        meta_band_name[BI_SWIR_1] = strdup("band6");
        meta_band_toa_name[BI_SWIR_1] = strdup("toa_band6");

        meta_band_name[BI_SWIR_2] = strdup("band7");
        meta_band_toa_name[BI_SWIR_2] = strdup("toa_band7");
        /* Cirrus */
        meta_band_name[BI_CIRRUS] = strdup("band9");
        meta_band_toa_name[BI_CIRRUS] = strdup("toa_band9");
        /* Thermal */
        meta_band_name[BI_THERMAL] = strdup("band10");
        meta_band_toa_name[BI_THERMAL] = strdup("bt_band10");
    }
    else if (!strcmp(gmeta->instrument, "TM"))
    {
        input->sensor = IS_TM;
        input->num_toa_bands = TM_REFL_BAND_COUNT;

        meta_band_name[BI_BLUE] = strdup("band1");
        meta_band_toa_name[BI_BLUE] = strdup("toa_band1");

        meta_band_name[BI_GREEN] = strdup("band2");
        meta_band_toa_name[BI_GREEN] = strdup("toa_band2");

        meta_band_name[BI_RED] = strdup("band3");
        meta_band_toa_name[BI_RED] = strdup("toa_band3");

        meta_band_name[BI_NIR] = strdup("band4");
        meta_band_toa_name[BI_NIR] = strdup("toa_band4");

        meta_band_name[BI_SWIR_1] = strdup("band5");
        meta_band_toa_name[BI_SWIR_1] = strdup("toa_band5");

        meta_band_name[BI_SWIR_2] = strdup("band7");
        meta_band_toa_name[BI_SWIR_2] = strdup("toa_band7");
        /* Cirrus */
        meta_band_name[BI_CIRRUS] = strdup("NA");
        meta_band_toa_name[BI_CIRRUS] = strdup("NA");
        /* Thermal */
        meta_band_name[BI_THERMAL] = strdup("band6");
        meta_band_toa_name[BI_THERMAL] = strdup("bt_band6");
    }
    else if (!strncmp(gmeta->instrument, "ETM", 3))
    {
        input->sensor = IS_ETM;
        input->num_toa_bands = ETM_REFL_BAND_COUNT;

        meta_band_name[BI_BLUE] = strdup("band1");
        meta_band_toa_name[BI_BLUE] = strdup("toa_band1");

        meta_band_name[BI_GREEN] = strdup("band2");
        meta_band_toa_name[BI_GREEN] = strdup("toa_band2");

        meta_band_name[BI_RED] = strdup("band3");
        meta_band_toa_name[BI_RED] = strdup("toa_band3");

        meta_band_name[BI_NIR] = strdup("band4");
        meta_band_toa_name[BI_NIR] = strdup("toa_band4");

        meta_band_name[BI_SWIR_1] = strdup("band5");
        meta_band_toa_name[BI_SWIR_1] = strdup("toa_band5");

        meta_band_name[BI_SWIR_2] = strdup("band7");
        meta_band_toa_name[BI_SWIR_2] = strdup("toa_band7");
        /* Cirrus */
        meta_band_name[BI_CIRRUS] = strdup("NA");
        meta_band_toa_name[BI_CIRRUS] = strdup("NA");
        /* Thermal */
        meta_band_name[BI_THERMAL] = strdup("band6");
        meta_band_toa_name[BI_THERMAL] = strdup("bt_band6");
    }
    else
    {
        error_string = "invalid instrument";
        RETURN_ERROR(error_string, "GetXMLInput", false);
    }

    /* Initialize these */
    for (band_index = 0; band_index < MAX_BAND_COUNT; band_index++)
    {
        input->meta.gain[band_index] = 1.0;
        input->meta.bias[band_index] = 0.0;
        input->meta.satu_value_ref[band_index] = -9999;
        input->meta.satu_value_max[band_index] = -9999;
    }

    for (i = 0; i < metadata->nbands; i++)
    {
        /* Find L1G/T band 1 in the input XML file to obtain gain/bias
           information */
        if (input->sensor == IS_OLI || input->sensor == IS_OLITIRS)
        {
            for (band_index = 0; band_index < MAX_BAND_COUNT; band_index++)
            {
                if (!strcmp(metadata->band[i].name, meta_band_name[band_index])
                    && !strncmp(metadata->band[i].product, "L1", 2))
                {
                    if (band_index == BI_THERMAL)
                    {
                        input->meta.gain[BI_THERMAL] =
                            metadata->band[i].rad_gain;
                        input->meta.bias[BI_THERMAL] =
                            metadata->band[i].rad_bias;
                        input->meta.k1 = metadata->band[i].k1_const;
                        input->meta.k2 = metadata->band[i].k2_const;
                    }
                    else
                    {
                        input->meta.gain[band_index] =
                            metadata->band[i].refl_gain;
                        input->meta.bias[band_index] =
                            metadata->band[i].refl_bias;
                    }
                    break;
                }
            }
        }
        else
        {
            for (band_index = 0; band_index < MAX_BAND_COUNT; band_index++)
            {
                if (!strcmp(metadata->band[i].name, meta_band_name[band_index])
                    && !strncmp(metadata->band[i].product, "L1", 2))
                {
                    input->meta.gain[band_index] = metadata->band[i].rad_gain;
                    input->meta.bias[band_index] = metadata->band[i].rad_bias;
                    break;
                }
            }
        }

        /* Find TOA bands in the input XML file to obtain band-related
           information */
        for (band_index = 0; band_index < NON_THERMAL_BAND_COUNT; band_index++)
        {
            if (!strcmp(metadata->band[i].name, meta_band_toa_name[band_index])
                && !strcmp(metadata->band[i].product, "toa_refl"))
            {
                input->file_name[band_index] =
                    strdup(metadata->band[i].file_name);
                input->meta.satu_value_ref[band_index] =
                    metadata->band[i].saturate_value;
                break;
            }
        }

        if (!strcmp(metadata->band[i].name, meta_band_toa_name[BI_BLUE])
            && !strcmp(metadata->band[i].product, "toa_refl"))
        {
            /* this is the index we'll use for reflectance band info */
            ref_index = i;
        }
        else if (!strcmp(metadata->band[i].name,
                         meta_band_toa_name[BI_THERMAL])
                 && !strcmp(metadata->band[i].product, "toa_bt"))
        {
            input->file_name[BI_THERMAL] = strdup(metadata->band[i].file_name);
            input->meta.satu_value_ref[BI_THERMAL] =
                metadata->band[i].saturate_value;
            input->meta.therm_scale_fact = metadata->band[i].scale_factor;
        }
    } /* for i */

    for (band_index = 0; band_index < MAX_BAND_COUNT; band_index++)
    {
        free(meta_band_name[band_index]);
        free(meta_band_toa_name[band_index]);
    }

    if (ref_index == -1)
    {
        error_string = "not able to find the reflectance index band";
        RETURN_ERROR(error_string, "GetXMLInput", false);
    }

    /* Pull the reflectance info from band1 in the XML file */
    input->size.s = metadata->band[ref_index].nsamps;
    input->size.l = metadata->band[ref_index].nlines;
    input->meta.fill = metadata->band[ref_index].fill_value;
    input->meta.pixel_size[0] = metadata->band[ref_index].pixel_size[0];
    input->meta.pixel_size[1] = metadata->band[ref_index].pixel_size[1];

    if (sscanf(acq_date, "%4d-%2d-%2d", &year, &month, &day) != 3)
    {
        RETURN_ERROR("invalid date format", "GetXMLInput", false);
    }

    if (!convert_year_month_day_to_doy(year, month, day,
                                       &input->meta.day_of_year))
    {
        error_string = "converting acquisition date to day of year";
        RETURN_ERROR(error_string, "GetXMLInput", false);
    }

    return true;
}
