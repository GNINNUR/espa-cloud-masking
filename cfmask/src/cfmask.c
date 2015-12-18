

#include <string.h>
#include <time.h>


#include "espa_metadata.h"
#include "parse_metadata.h"
#include "write_metadata.h"
#include "envi_header.h"
#include "espa_geoloc.h"
#include "raw_binary_io.h"


#include "const.h"
#include "error.h"
#include "input.h"
#include "output.h"
#include "misc.h"
#include "potential_cloud_shadow_snow_mask.h"
#include "object_cloud_shadow_match.h"
#include "cfmask.h"


/*****************************************************************************
METHOD:  cfmask

PURPOSE:  The main routine for fmask written in C

RETURN VALUE: Type = int
    Value           Description
    -----           -----------
    ERROR           An error occurred during processing of cfmask
    SUCCESS         Processing was successful
*****************************************************************************/
int
main (int argc, char *argv[])
{
    char *FUNC_NAME = "main";
    char errstr[MAX_STR_LEN];     /* error string */
    char *cptr = NULL;            /* pointer to the file extension */
    char *xml_name = NULL;        /* input XML filename */
    int ib;                       /* band counters */
    Input_t *input = NULL;        /* input data and meta data */
    char envi_file[MAX_STR_LEN];  /* output ENVI file name */
    unsigned char *pixel_mask;    /* pixel mask */
    unsigned char *conf_mask;     /* confidence mask */
    int status;               /* return value from function call */
    float clear_ptm;          /* percent of clear-sky pixels */
    float t_templ = 0.0;      /* percentile of low background temperature */
    float t_temph = 0.0;      /* percentile of high background temperature */
    Output_t *output = NULL;  /* output structure and metadata */
    bool verbose;             /* verbose flag for printing messages */
    bool use_cirrus;          /* should we use Cirrus during determination? */
    bool use_thermal;         /* should we use Thermal during determination? */
    int cldpix = 2;           /* Default buffer for cloud pixel dilate */
    int sdpix = 2;            /* Default buffer for shadow pixel dilate */
    float cloud_prob;         /* Default cloud probability */
    float sun_azi_temp = 0.0; /* Keep the original sun azimuth angle */
    Espa_internal_meta_t xml_metadata; /* XML metadata structure */
    Envi_header_t envi_hdr;            /* output ENVI header information */
    int pixel_count;
    int pixel_index;

    time_t now;
    time(&now);

    /* Read the command-line arguments, including the name of the input
       Landsat TOA reflectance product and the DEM */
    status = get_args(argc, argv, &xml_name, &cloud_prob, &cldpix,
                      &sdpix, &use_cirrus, &use_thermal, &verbose);
    if (status != SUCCESS)
    {
        sprintf(errstr, "calling get_args");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    printf("CFmask start_time=%s\n", ctime(&now));

    /* Validate the input metadata file */
    if (validate_xml_file(xml_name) != SUCCESS)
    {
        sprintf(errstr, "XML validation error");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Initialize the metadata structure */
    init_metadata_struct(&xml_metadata);

    /* Parse the metadata file into our internal metadata structure; also
       allocates space as needed for various pointers in the global and band
       metadata */
    if (parse_metadata(xml_name, &xml_metadata) != SUCCESS)
    {
        sprintf(errstr, "XML parsing error");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Open input file, read metadata, and set up buffers */
    input = OpenInput(&xml_metadata, use_thermal);
    if (input == NULL)
    {
        sprintf(errstr, "opening the TOA and brightness temp files in: %s",
                xml_name);
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    if (verbose)
    {
        /* Print some info to show how the input metadata works */
        printf("Number of input TOA bands: %d\n", input->num_toa_bands);
        printf("Number of input thermal bands: %d\n", 1);
        printf("Number of input TOA lines: %d\n", input->size.l);
        printf("Number of input TOA samples: %d\n", input->size.s);
        printf("Fill value is %d\n", input->meta.fill);
        for (ib = 0; ib < MAX_BAND_COUNT; ib++)
        {
            printf("Band %d-->\n", ib);
            printf("  band satu_value_ref: %d\n",
                   input->meta.satu_value_ref[ib]);
            printf("  band satu_value_max: %d\n",
                   input->meta.satu_value_max[ib]);
            printf("  band gain: %f, band bias: %f\n",
                   input->meta.gain[ib], input->meta.bias[ib]);
        }

        printf("SUN AZIMUTH is %f\n", input->meta.sun_az);
        printf("SUN ZENITH is %f\n", input->meta.sun_zen);
    }

    /* If the scene is an ascending polar scene (flipped upside down), then
       the solar azimuth needs to be adjusted by 180 degrees.  The scene in
       this case would be north down and the solar azimuth is based on north
       being up clock-wise direction. Flip the south to be up will not change
       the actual sun location, with the below relations, the solar azimuth
       angle will need add in 180.0 for correct sun location */
    if (input->meta.ul_corner.is_fill &&
        input->meta.lr_corner.is_fill &&
        (input->meta.ul_corner.lat - input->meta.lr_corner.lat) < MINSIGMA)
    {
        /* Keep the original solar azimuth angle */
        sun_azi_temp = input->meta.sun_az;
        input->meta.sun_az += 180.0;
        if ((input->meta.sun_az - 360.0) > MINSIGMA)
        {
            input->meta.sun_az -= 360.0;
        }
        if (verbose)
        {
            printf("Polar or ascending scene."
                    "  Readjusting solar azimuth by 180 degrees.\n"
                    "  New value: %f degrees\n", input->meta.sun_az);
        }
    }

    pixel_count = input->size.l * input->size.s;

    /* Dynamic allocate the 2d mask memory */
    pixel_mask = calloc(pixel_count, sizeof(unsigned char));
    if (pixel_mask == NULL)
    {
        sprintf (errstr, "Allocating pixel mask memory");
        CFMASK_ERROR (errstr, FUNC_NAME);
    }

    conf_mask = calloc(pixel_count, sizeof(unsigned char));
    if (conf_mask == NULL)
    {
        sprintf(errstr, "Allocating confidence mask memory");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Initialize the mask to clear data */
    for (pixel_index = 0; pixel_index < pixel_count; pixel_index++)
    {
        pixel_mask[pixel_index] = CF_NO_BITS;
        conf_mask[pixel_index] = CLOUD_CONFIDENCE_NONE;
    }

    /* Build the potential cloud, shadow, snow, water mask */
    status = potential_cloud_shadow_snow_mask(input, cloud_prob, &clear_ptm,
                                              &t_templ, &t_temph, pixel_mask,
                                              conf_mask, use_cirrus,
                                              use_thermal, verbose);
    if (status != SUCCESS)
    {
        sprintf(errstr, "processing potential_cloud_shadow_snow_mask");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    printf("Pcloud done, starting cloud/shadow match\n");

    /* Build the final cloud shadow based on geometry matching and
       combine the final cloud, shadow, snow, water masks into fmask
       the pixel_mask is a bit mask as input and a value mask as output */
    status = object_cloud_shadow_match(input, clear_ptm, t_templ, t_temph,
                                       cldpix, sdpix, pixel_mask,
                                       use_thermal, verbose);
    if (status != SUCCESS)
    {
        sprintf(errstr, "processing object_cloud_and_shadow_match");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Reassign solar azimuth angle for output purpose if south up north
       down scene is involved */
    if (input->meta.ul_corner.is_fill
        && input->meta.lr_corner.is_fill
        && (input->meta.ul_corner.lat - input->meta.lr_corner.lat) < MINSIGMA)
    {
        input->meta.sun_az = sun_azi_temp;
    }

    /* Open the output file */
    output = OpenOutput(&xml_metadata, input);
    if (output == NULL)
    {
        sprintf(errstr, "Opening output file");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    if (!PutOutput(output, pixel_mask))
    {
        sprintf(errstr, "Writing output fmask files\n");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Close the output file */
    if (!CloseOutput(output))
    {
        sprintf(errstr, "closing output file");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Create the ENVI header file this band */
    if (create_envi_struct(&output->metadata.band[0], &xml_metadata.global,
                           &envi_hdr) != SUCCESS)
    {
        sprintf(errstr, "Creating ENVI header structure.");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Write the ENVI header */
    strcpy(envi_file, output->metadata.band[0].file_name);
    cptr = strchr(envi_file, '.');
    if (cptr == NULL)
    {
        sprintf(errstr, "error in ENVI header filename");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    strcpy(cptr, ".hdr");
    if (write_envi_hdr(envi_file, &envi_hdr) != SUCCESS)
    {
        sprintf(errstr, "Writing ENVI header file.");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Append the cfmask band to the XML file */
    if (append_metadata(output->nband, output->metadata.band, xml_name)
        != SUCCESS)
    {
        sprintf(errstr, "Appending spectral index bands to XML file.");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Free the structure */
    if (!FreeOutput(output))
    {
        sprintf(errstr, "freeing output file structure");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    output = OpenOutputConfidence(&xml_metadata, input);
    if (output == NULL)
    {
        sprintf(errstr, "Opening output file");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    if (!PutOutput(output, conf_mask))
    {
        sprintf(errstr, "Writing output fmask files\n");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Close the output file */
    if (!CloseOutput(output))
    {
        sprintf(errstr, "closing output file");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Create the ENVI header file this band */
    if (create_envi_struct(&output->metadata.band[0], &xml_metadata.global,
                           &envi_hdr) != SUCCESS)
    {
        sprintf(errstr, "Creating ENVI header structure.");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Write the ENVI header */
    strcpy(envi_file, output->metadata.band[0].file_name);
    cptr = strchr(envi_file, '.');
    if (cptr == NULL)
    {
        sprintf(errstr, "error in ENVI header filename");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    strcpy(cptr, ".hdr");
    if (write_envi_hdr(envi_file, &envi_hdr) != SUCCESS)
    {
        sprintf(errstr, "Writing ENVI header file.");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Append the cfmask band to the XML file */
    if (append_metadata(output->nband, output->metadata.band,
                        xml_name) != SUCCESS)
    {
        sprintf(errstr, "Appending spectral index bands to XML file.");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Free the structure */
    if (!FreeOutput(output))
    {
        sprintf(errstr, "freeing output file structure");
        CFMASK_ERROR(errstr, FUNC_NAME);
    }

    /* Free the metadata structure */
    free_metadata(&xml_metadata);

    /* Free the pixel mask */
    free(pixel_mask);
    pixel_mask = NULL;
    free(conf_mask);
    conf_mask = NULL;

    /* Close the input file and free the structure */
    CloseInput(input);
    FreeInput(input);

    free(xml_name);
    xml_name = NULL;

    printf("Processing complete.\n");
    time(&now);
    printf("CFmask end_time=%s\n", ctime(&now));

    return SUCCESS;
}


/*****************************************************************************
MODULE:  usage

PURPOSE:  Prints the usage information for this application.

RETURN VALUE: None
*****************************************************************************/
void
usage()
{
    version();

    printf("Fmask identification of cloud, cloud shadow, snow, water, and"
           " clear pixels in a Landsat scene using the Top Of Atmosphere"
           " reflection and Brightness Temperature derived from the L1T.\n");
    printf("\n");
    printf("Usage: ./%s --xml <xml filename> [options]\n", CFMASK_APP_NAME);
    printf("\n");
    printf("where the following parameters are required:\n");
    printf("    --xml: name of the input XML file which contains the TOA"
           " reflectance and brightness temperature files\n");
    printf("\n");
    printf("where the following parameters are optional:\n");
    printf("    --prob: cloud_probability,"
           " (default value is 22.5)\n");
    printf("    --cldpix: cloud_pixel_buffer for image dilate,"
           " (default value is 3)\n");
    printf("    --sdpix: shadow_pixel_buffer for image dilate,"
           " (default value is 3)\n");
    printf("    --with-cirrus: use Cirrus data in cloud detection"
           " (default is false, meaning Boston University's dynamic cirrus"
           " band static threshold will *not* be used)\n");
    printf("    --without-thermal: don't use thermal data during cloud"
           " detection and height determination for shadows"
           " (default is false, meaning always use thermal)\n");
    printf("    --verbose: display intermediate messages"
           " (default is false)\n");
    printf("\n");
    printf("Examples:\n");
    printf("    ./%s --xml LC80330372013141LGN01.xml --prob=22.5 --cldpix=3"
           " --sdpix=3 --verbose\n\n", CFMASK_APP_NAME);
    printf("    ./%s --xml LC80330372013141LGN01.xml --without-thermal"
           " --with-cirrus --verbose\n\n", CFMASK_APP_NAME);

    printf("    ./%s --version    (prints the version information"
           " for this application)\n", CFMASK_APP_NAME);
    printf("    ./%s --help    (prints the help information"
           " for this application)\n", CFMASK_APP_NAME);
}


/*****************************************************************************
MODULE:  version

PURPOSE:  Prints the version information for this application.

RETURN VALUE: None
*****************************************************************************/
void
version()
{
    printf("%s version %s\n", CFMASK_APP_NAME, CFMASK_VERSION);
}
