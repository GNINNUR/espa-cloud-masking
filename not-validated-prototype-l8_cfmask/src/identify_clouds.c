
/*
    NOTE: Taken from IAS_3_6_0_IT and modified a bit for logging and such
          related items.  Tried to keep as identical as possible.
*/

/* System Includes */
#include <stdio.h>
#include <stdlib.h>

/* Local Includes */
#include "const.h"
#include "error.h"
#include "identify_clouds.h"

/*****************************************************************************
Name: create_cloud_runs

Purpose: Given a pixel mask, generate the set of run-length encoded line
    segments that cover the clouds.

Returns: SUCCESS/ERROR

*****************************************************************************/
static int create_cloud_runs
(
    unsigned char *pixel_mask, /* I: Cloud pixel mask */
    int nrows,                  /* I: Number of rows */
    int ncols,                  /* I: Number of columns */
    RLE_T **cloud_runs,         /* I/O: Array of cloud runs */
    int *out_run_count          /* O: Count of cloud runs found */
)
{
    char *FUNC_NAME = "create_cloud_runs";
    int runs_allocated = 0;
    RLE_T *runs = NULL;
    RLE_T *temp_runs = NULL;
    int row;
    int run_count = 0;

    *out_run_count = 0;
    *cloud_runs = NULL;

    /* Loop over all the rows */
    for (row = 0; row < nrows; row++)
    {
        int col;
        const unsigned char *mask_row = &pixel_mask[row * ncols];

        /* Loop over the columns in this row */
        col = 0;
        while (col < ncols)
        {
            int run_len;
            RLE_T *run;

            /* Search for a starting cloud pixel */
            if (!(mask_row[col] & CF_CLOUD_BIT))
            {
                col++;
                continue;
            }

            /* Found a starting cloud pixel, so find the length of the cloud
               run */
            for (run_len = 1; run_len < ncols; run_len++)
            {
                if (!(mask_row[col + run_len] & CF_CLOUD_BIT))
                    break;
            }

            /* Grow the run array if needed */
            if (run_count >= runs_allocated)
            {
                /* Double the size of the array when more memory is needed */
                if (runs_allocated == 0)
                    runs_allocated = 10000;
                else
                    runs_allocated *= 2;

                temp_runs = realloc(runs, runs_allocated * sizeof(*runs));
                if (!temp_runs)
                {
                    free(runs);
                    RETURN_ERROR("Allocating memory for identifying clouds",
                                 FUNC_NAME, ERROR);
                }
                runs = temp_runs;
            }

            run = &runs[run_count];
            run_count++;

            run->row = row;
            run->start_col = col;
            run->col_count = run_len;
            run->next_index = -1;

            /* Resume checking at the next unchecked pixel */
            col += run_len + 1;
        }
    }

    /* Use realloc to shrink set of runs to the actual number found */
    temp_runs = realloc(runs, run_count * sizeof(*runs));
    if (!temp_runs)
    {
        free(runs);
        RETURN_ERROR("Failed shrinking cloud pixel run allocation",
                     FUNC_NAME, ERROR);
    }
    runs = temp_runs;

    *cloud_runs = runs;
    *out_run_count = run_count;

    return SUCCESS;
}


/*****************************************************************************
Name: identify_clouds

Purpose: Given a pixel mask identifying individual pixels as clouds, group
    the individual pixels into cloud objects using run-length encoding.

Notes:
    - cloud number zero is reserved for the "no cloud" condition

Returns: SUCCESS/ERROR

*****************************************************************************/
int identify_clouds
(
    unsigned char *pixel_mask, /* I: Cloud pixel mask */
    int nrows,                  /* I: Number of rows */
    int ncols,                  /* I: Number of columns */
    RLE_T **out_cloud_runs,     /* O: Array of cloud run-length encoded
                                      segments */
    int **out_cloud_lookup,     /* O: Array to map clouds to cloud runs */
    int **out_cloud_pixel_count,/* O: Cloud number */
    int *out_cloud_count,       /* O: Number of clouds in the cloud_lookup and
                                      cloud_pixel_count arrays */
    int *cloud_map              /* I/O: Image containing the cloud number for
                                        each pixel */
)
{
    char *FUNC_NAME = "identify_clouds";
    RLE_T *runs = NULL;
    int run_count;
    int *cloud_lookup = NULL;  /* Array that points to the first RLE for each
                                  cloud number */
    int *temp_ptr;
    int next_cloud_number = 1;
    int run_index;
    int cloud_index;
    int cloud_count;
    int *cloud_pixel_count = NULL;

    *out_cloud_runs = NULL;
    *out_cloud_lookup = NULL;
    *out_cloud_pixel_count = NULL;
    *out_cloud_count = 0;

    /* Identify the set of "runs" of cloud pixels */
    if (create_cloud_runs(pixel_mask, nrows, ncols, &runs, &run_count)
        != SUCCESS)
    {
        RETURN_ERROR("Failed identifying clouds", FUNC_NAME, ERROR);
    }

    /* If no runs of clouds found, no need to do anything else */
    if (run_count == 0)
        return SUCCESS;

    /* Allocate the cloud lookup table sized to assume each run is a separate
       cloud */
    cloud_lookup = malloc(run_count * sizeof(*cloud_lookup));
    if (!cloud_lookup)
    {
        free(runs);
        RETURN_ERROR("Failed allocating cloud lookup table",
                     FUNC_NAME, ERROR);
    }

    /* The first entry in the lookup is reserved for "no clouds" */
    cloud_lookup[0] = -1;

    /* Walk the cloud runs to group them into clouds */
    for (run_index = 0; run_index < run_count; run_index++)
    {
        RLE_T *run = &runs[run_index];
        int use_next = 1;
        int assigned_cloud_number = 0;
        int end_col = run->start_col + run->col_count;

        /* Check for overlap with clouds from the previous row if not the first
           row.  Note that the overlap includes cloud pixels on the diagonal,
           so the column range for the check includes an extra pixel on both
           ends. */
        if (run->row > 0)
        {
            int *prev_cloud_map_row = &cloud_map[(run->row - 1) * ncols];
            int col;
            int start = run->start_col - 1;
            if (start < 0)
                start = 0;

            for (col = start; col <= end_col; col++)
            {
                int cloud_number = prev_cloud_map_row[col];

                /* Check for a cloud in the previous row */
                if (cloud_number != 0)
                {
                    int fill_col;
                    int *cloud_map_row = &cloud_map[run->row * ncols];

                    /* Found a cloud, so assign this run to that cloud */
                    run->next_index = cloud_lookup[cloud_number];
                    cloud_lookup[cloud_number] = run_index;

                    /* Set this cloud number for the run in the cloud map */
                    for (fill_col = run->start_col; fill_col < end_col;
                        fill_col++)
                    {
                        cloud_map_row[fill_col] = cloud_number;
                    }

                    /* Assigned a cloud number, so no need to look */
                    assigned_cloud_number = cloud_number;
                    use_next = 0;
                    break;
                }
            }

            /* If an overlapping cloud from the previous row was found, look
               for additional overlapping clouds to merge with, continuing at
               the next column */
            for (col = col + 1; col <= end_col; col++)
            {
                int cloud_number = prev_cloud_map_row[col];

                /* Check for a cloud in the previous row */
                if (cloud_number != 0 && cloud_number != assigned_cloud_number)
                {
                    int fill_col;
                    int last_index;
                    int *cloud_map_row = &cloud_map[run->row * ncols];

                    /* Found another cloud to merge with, so renumber the newly
                       found cloud to match this number.  The cloud map update
                       is "lazy" since it just updates the cloud numbers in
                       the previous row and pixels to the left in the current
                       row.  A final pass will update the cloud_map to make
                       sure it has the correct cloud numbers for every pixel */
                    for (fill_col = 0; fill_col < ncols; fill_col++)
                    {
                        if (prev_cloud_map_row[fill_col] == cloud_number)
                        {
                            prev_cloud_map_row[fill_col]
                                = assigned_cloud_number;
                        }
                    }

                    /* Also fill the pixels on the current row that may belong
                       to the same cloud */
                    for (fill_col = 0; fill_col < col; fill_col++)
                    {
                        if (cloud_map_row[fill_col] == cloud_number)
                        {
                            cloud_map_row[fill_col] = assigned_cloud_number;
                        }
                    }

                    /* Find the last node in the cloud being merged */
                    last_index = cloud_lookup[cloud_number];
                    while (runs[last_index].next_index != -1)
                    {
                        last_index = runs[last_index].next_index;
                    }

                    /* Merge the newly found cloud list into the current
                       cloud's list */
                    runs[last_index].next_index
                        = cloud_lookup[assigned_cloud_number];
                    cloud_lookup[assigned_cloud_number]
                        = cloud_lookup[cloud_number];
                    cloud_lookup[cloud_number] = -1;
                }
            }
        }

        /* If no cloud number was assigned, use the next one */
        if (use_next)
        {
            int fill_col;
            int *cloud_map_row = &cloud_map[run->row * ncols];

            for (fill_col = run->start_col; fill_col < end_col; fill_col++)
            {
                cloud_map_row[fill_col] = next_cloud_number;
            }

            run->next_index = -1;
            cloud_lookup[next_cloud_number] = run_index;

            next_cloud_number++;

            /* Make sure we never overflow the capacity of the cloud counter
               since that would break this implementation.  This is incredibly
               unlikely since it would require much larger scenes than are
               currently processed. */
            if (next_cloud_number < 0)
            {
                free(cloud_lookup);
                free(runs);
                RETURN_ERROR("Too many clouds identified", FUNC_NAME, ERROR);
            }
        }
    }

    /* Condense the cloud lookup table */
    cloud_count = next_cloud_number;
    next_cloud_number = 1;
    for (cloud_index = 1; cloud_index < cloud_count; cloud_index++)
    {
        if (cloud_lookup[cloud_index] != -1)
        {
            cloud_lookup[next_cloud_number] = cloud_lookup[cloud_index];
            next_cloud_number++;
        }
    }
    cloud_count = next_cloud_number;

    /* Shrink cloud_lookup array to only use what it needs */
    temp_ptr = realloc(cloud_lookup, sizeof(*cloud_lookup) * cloud_count);
    if (!temp_ptr)
    {
        free(cloud_lookup);
        free(runs);
        RETURN_ERROR("Failed shrinking cloud lookup array", FUNC_NAME, ERROR);
    }
    cloud_lookup = temp_ptr;

    /* Allocate memory for the count of pixels in each cloud */
    cloud_pixel_count = malloc(cloud_count * sizeof(*cloud_pixel_count));
    if (!cloud_pixel_count)
    {
        free(cloud_lookup);
        free(runs);
        RETURN_ERROR("Failed allocating memory for the cloud pixel count "
                     "array", FUNC_NAME, ERROR);
    }

    /* Update the cloud map with the latest cloud numbers and to cover any
       cloud merges that occurred */
    for (cloud_index = 1; cloud_index < cloud_count; cloud_index++)
    {
        int run_index = cloud_lookup[cloud_index];
        int pixel_count = 0;

        while (run_index != -1)
        {
            RLE_T *run = &runs[run_index];
            int fill_col;
            int *cloud_map_row = &cloud_map[run->row * ncols];
            int end_col = run->start_col + run->col_count;

            for (fill_col = run->start_col; fill_col < end_col; fill_col++)
            {
                cloud_map_row[fill_col] = cloud_index;
            }
            pixel_count += run->col_count;

            run_index = run->next_index;
        }

        cloud_pixel_count[cloud_index] = pixel_count;
    }

    *out_cloud_runs = runs;
    *out_cloud_lookup = cloud_lookup;
    *out_cloud_pixel_count = cloud_pixel_count;
    *out_cloud_count = cloud_count;

    return SUCCESS;
}
