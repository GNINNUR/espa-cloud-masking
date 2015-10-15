
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
#include "fill_local_minima_in_image.h"

/* From IAS Math Implementation */
#define IAS_MAX(A,B)    (((A) > (B)) ? (A) : (B))
#define IAS_MIN(A,B)    (((A) < (B)) ? (A) : (B))

/* Define the number of pixel elements to allocate in each block */
#define BUFFER_BLOCK_SIZE 1000000

/* Support structures and routines for fill_minima. */
typedef struct queue_element 
{
    int i;                  /* Row of pixel */
    int j;                  /* Column of pixel */
    struct queue_element *next;  /* Pointer to next pixel */
} QUEUE_ELEMENT;

typedef struct queue_header
{
    QUEUE_ELEMENT *first;           /* Pointer in queue - first */
    QUEUE_ELEMENT *last;            /* Pointer in queue - last */
} QUEUE_HEADER;

/* Pixel queue structure.  Instead of allocating each element in the queue
   separately, a custom allocation scheme is used to allocate a block of
   elements at a time and assign each of them as needed.  When an additional
   block is needed, a new one is allocated.  The list of blocks is maintained
   by using the first element in each block as a pointer to the next block. */
typedef struct pixel_queue 
{
    int h_min;
    int num_levels;
    QUEUE_HEADER *q;
    QUEUE_ELEMENT *pix_array;  /* First allocated QUEUE_ELEMENT block */
    QUEUE_ELEMENT *current_pix_array;/* Current block of QUEUE_ELEMENT being
                                        used for allocs */
    int next_new;              /* Next element to alloc in the current block */
    QUEUE_ELEMENT *free_list;  /* Free list of elements */
} PIXEL_QUEUE;

/*-----------------------------------------------------------------------------
NAME: *create_new_pixel

PURPOSE: Allocate a new pixel queue element and initialize it.

RETURNS: Pointer to QUEUE_ELEMENT, NULL if the allocation fails
------------------------------------------------------------------------------*/
static QUEUE_ELEMENT *create_new_pixel
(
    PIXEL_QUEUE *pixel_q,   /* I: Pixel queue to use for allocations */
    int i,                  /* I: Row of pixel */
    int j                   /* I: Column of pixel */
)
{
    char *FUNC_NAME = "create_new_pixel";
    QUEUE_ELEMENT *p;
    int alloc_index = pixel_q->next_new;

    if (alloc_index >= BUFFER_BLOCK_SIZE)
    {
        /* Allocate another block of pixel elements */
        QUEUE_ELEMENT *new_block = malloc(BUFFER_BLOCK_SIZE
                * sizeof(*new_block));
        if (!new_block)
        {
            RETURN_ERROR("Allocating a new block of queue elements",
                         FUNC_NAME, NULL);
        }
        new_block[0].next = NULL;

        /* The first element in the current array is used as a pointer to the
           next block */
        pixel_q->current_pix_array[0].next = new_block;

        /* Allocating from the next block now */
        pixel_q->current_pix_array = new_block;
        pixel_q->next_new = 1;
        alloc_index = 1;
    }
    pixel_q->next_new++;

    p = &pixel_q->current_pix_array[alloc_index];
    p->i = i;
    p->j = j;
    p->next = NULL;

    return p;
}

/*-----------------------------------------------------------------------------
NAME: *initialize_pixel_queue

PURPOSE: Initialize pixel queue

RETURNS: Pointer to the PIXEL_QUEUE, NULL on error
------------------------------------------------------------------------------*/
static PIXEL_QUEUE *initialize_pixel_queue
(
    int h_min,   /* I: Minimum element level in the queue */
    int h_max    /* I: Maximum element level in the queue */
)
{
    char *FUNC_NAME = "initialize_pixel_queue";
    PIXEL_QUEUE *pixel_q;
    int num_levels;
    int i;

    pixel_q = (PIXEL_QUEUE *)calloc(1, sizeof(PIXEL_QUEUE));
    if (pixel_q == NULL)
    {
        RETURN_ERROR("Allocating memory for the pixel queue",
                     FUNC_NAME, NULL);
    }
    num_levels = h_max - h_min + 1;
    pixel_q->h_min = h_min;
    pixel_q->num_levels = num_levels;

    pixel_q->q = (QUEUE_HEADER *)calloc(num_levels, sizeof(QUEUE_HEADER));
    if (pixel_q->q == NULL)
    {
       free(pixel_q);
       RETURN_ERROR("Allocating memory for pixel queue header",
                    FUNC_NAME, NULL);
    }
    for (i = 0; i < num_levels; i++)
    {
        pixel_q->q[i].first = NULL;
        pixel_q->q[i].last = NULL;
    }

    /* Allocate a starting block of QUEUE_ELEMENT buffers */
    pixel_q->pix_array = malloc(BUFFER_BLOCK_SIZE * sizeof(QUEUE_ELEMENT));
    if (!pixel_q->pix_array)
    {
       free(pixel_q->q);
       free(pixel_q);
       RETURN_ERROR("Allocating memory for the queue element array",
                    FUNC_NAME, NULL);
    }

    /* Reserve the first entry for the next block link */
    pixel_q->next_new = 1;
    pixel_q->pix_array[0].next = NULL;
    pixel_q->current_pix_array = pixel_q->pix_array;

    return pixel_q;
}

/*-----------------------------------------------------------------------------
NAME: free_pixel_queue

PURPOSE: Free a pixel queue

RETURNS: nothing
------------------------------------------------------------------------------*/
static void free_pixel_queue
(
    PIXEL_QUEUE *pixel_q   /* I: Pointer to PIXEL_QUEUE */
)
{
    QUEUE_ELEMENT *p;

    /* Free the blocks of pixel elements.  Note that the free list does not
       need to be free'd since nothing in that list was directly allocated. */
    p = pixel_q->pix_array;
    while (p)
    {
        QUEUE_ELEMENT *next = p->next;
        free(p);
        p = next;
    }
    free(pixel_q->q);
    pixel_q->q = NULL;

    free(pixel_q);
}

/*-----------------------------------------------------------------------------
NAME: add_pixel

PURPOSE: Add a pixel at level h

RETURNS: SUCCESS/ERROR

------------------------------------------------------------------------------*/
static int add_pixel
(
    PIXEL_QUEUE *pixel_q,/* I: Pointer to PIXEL_QUEUE */
    int row,             /* I: Row of pixel */
    int col,             /* I: Column of pixel */
    int h                /* I: Element level in the queue */
)
{
    char *FUNC_NAME = "add_pixel";
    int ndx;
    QUEUE_ELEMENT *current;
    QUEUE_ELEMENT *new_pixel;
    QUEUE_HEADER *level_q;

    ndx = h - pixel_q->h_min;
    if (ndx > pixel_q->num_levels)
    {
        RETURN_ERROR("Invalid element level", FUNC_NAME, ERROR);
    }
    level_q = &(pixel_q->q[ndx]);

    /* Get a pixel element from the free list, if available */
    new_pixel = pixel_q->free_list;
    if (new_pixel)
    {
        pixel_q->free_list = new_pixel->next;
        new_pixel->next = NULL;
        new_pixel->i = row;
        new_pixel->j = col;
    }
    else
    {
        /* Take a copy of the pixel structure */
        new_pixel = create_new_pixel(pixel_q, row, col);
        if (!new_pixel)
        {
            RETURN_ERROR("Adding element to the pixel queue",
                         FUNC_NAME, ERROR);
        }
    }

    /* Add to end of queue at this level */
    current = level_q->last;
    if (current != NULL)
    {
        current->next = new_pixel;
    }
    level_q->last = new_pixel;

    /* If head of queue is NULL, make the new one the head */
    if (level_q->first == NULL)
    {
        level_q->first = new_pixel;
    }

    return SUCCESS;
}

/*-----------------------------------------------------------------------------
NAME: *get_first_pixel_entry

PURPOSE: Return the first element in the queue at level h, and remove it
         from the queue

RETURNS: First element in the queue at level h
------------------------------------------------------------------------------*/
static QUEUE_ELEMENT *get_first_pixel_entry
(
    PIXEL_QUEUE *pixel_q,/* I: Pointer to PIXEL_QUEUE */
    int h                /* I: Element level in the queue */
)
{
    int ndx;
    QUEUE_ELEMENT *current;
    QUEUE_HEADER *level_q;

    ndx = h - pixel_q->h_min;
    level_q = &(pixel_q->q[ndx]);
    current = level_q->first;

    /* Remove from head of queue */
    if (current != NULL)
    {
        level_q->first = current->next;
        if (level_q->first == NULL)
        {
            level_q->last = NULL;
        }
    }

    return current;
}

/*-----------------------------------------------------------------------------
NAME:  kernel_has_fill

PURPOSE: Checks a 3x3 kernel around the line/sample passed in to see if any
         of the pixels have fill.

RETURN: TRUE if the 3x3 kernel has fill pixels, FALSE if not
-----------------------------------------------------------------------------*/
static inline int kernel_has_fill
(
    const short int *in_img,
    int nl,
    int ns,
    int center_line,
    int center_sample
)
{
    int start_line = center_line - 1;
    int end_line = center_line + 1;
    int start_samp = center_sample - 1;
    int end_samp = center_sample + 1;
    int line;

    if (start_line < 0)
        start_line = 0;
    else if (end_line >= nl)
        end_line = nl - 1;
    if (start_samp < 0)
        start_samp = 0;
    else if (end_samp >= ns)
        end_samp = ns - 1;

    for (line = start_line; line <= end_line; line++)
    {
        int samp;

        for (samp = start_samp; samp <= end_samp; samp++)
        {
            /* return 1 if fill found */
            if (in_img[line * ns + samp] == FILL_PIXEL)
                return TRUE;
        }
    }

    /* no fill found */
    return FALSE;
}

/*-----------------------------------------------------------------------------
NAME:  fill_local_minima_in_image

PURPOSE: Fill all local minima in the input img. The input
         array should be a 2-d array. This function returns
         an array of the same shape and datatype, with the same contents, but
         with local minima filled using the reconstruction-by-erosion algorithm.

RETURN: SUCCESS/ERROR

-----------------------------------------------------------------------------*/
int fill_local_minima_in_image
(
    const char *band_name,     /* I: Band name being filled */
    const short int *in_img,   /* I: Input image buffer */
    int num_rows,              /* I: Number of rows in the input image */
    int num_cols,              /* I: Number of columns in the input image */
    float boundary_val,        /* I: Background (land) percentage */
    short int *out_img         /* O: Output image buffer */
)
{
    char *FUNC_NAME = "add_pixel";
    int r, c;
    PIXEL_QUEUE *pixel_q;
    QUEUE_ELEMENT *p;
    int h_current;
    int hmin, hmax;
    int pixel_count = num_rows * num_cols;

    printf("minima filling setup for %s band\n", band_name);

    /* Find the min and max values in the input buffer. */
    hmin = 32767;
    hmax = -32767;
    for (r = 0; r < pixel_count; r++)
    {
         if (in_img[r] == FILL_PIXEL)
             continue;    /* Fill data */
         if (in_img[r] < hmin)
             hmin = in_img[r];
         else if (in_img[r] > hmax)
             hmax = in_img[r];
    }

    /* If hmin and hmax values have not changed, entire image is fill. */
    if (hmin == 32767 && hmax == -32767)
    {
        RETURN_ERROR("Entire image is fill", FUNC_NAME, ERROR);
    }

    /* If the boundary value is set to zero, use the maximum value. */
    if (boundary_val == 0)
        boundary_val = hmax;

    /* Fill the out_img with the max value. */
    for (r = 0; r < pixel_count; r++)
        out_img[r] = hmax;

    pixel_q = initialize_pixel_queue(hmin, hmax);
    if (!pixel_q)
    {
        RETURN_ERROR("Allocating pixel queue", FUNC_NAME, ERROR);
    }

    /* Initialize the boundary pixels. */
    for (r = 0; r < num_rows; r++)
    {
        const short int *in_line = &in_img[r * num_cols];
        short int *out_line = &out_img[r * num_cols];

        /* Find the boundary pixels on this row */
        for (c = 0; c < num_cols; c++)
        {
            /* Not a boundary if it is fill */
            if (in_line[c] == FILL_PIXEL)
            {
                /* Set the filled image to a fill pixel */
                out_line[c] = FILL_PIXEL;
                continue;
            }

            /* If the 3x3 kernel has any fill pixels, it is a boundary pixel */
            if (kernel_has_fill(in_img, num_rows, num_cols, r, c))
            {
                if (add_pixel(pixel_q, r, c, boundary_val) != SUCCESS)
                {
                    free_pixel_queue(pixel_q);
                    RETURN_ERROR("Adding pixel to queue",
                                 FUNC_NAME, ERROR);
                }

                out_line[c] = boundary_val;
            }
        }
    }

    printf("main minima filling started for %s band\n", band_name);

    /* Process until stability */
    h_current = (int)hmin;
    do
    {
        while ((p = get_first_pixel_entry(pixel_q, h_current)))
        {
            int start_row;
            int end_row;

            start_row = IAS_MAX(0, p->i - 1);
            end_row = IAS_MIN(num_rows, p->i + 2);

            for (r = start_row; r < end_row; r++)
            {
                int start_col = IAS_MAX(0, p->j - 1);
                int end_col = IAS_MIN(num_cols, p->j + 2);
                const short int *in_row = &in_img[r * num_cols];
                short int *out_row = &out_img[r * num_cols];

                for (c = start_col; c < end_col; c++)
                {
                    short int pixel;

                    /* Skip the current pixel */
                    if (r == p->i && c == p->j)
                        continue;

                    pixel = in_row[c];

                    /* Exclude null area of original image */
                    if (pixel != FILL_PIXEL && out_row[c] == hmax)
                    {
                        out_row[c] = IAS_MAX(h_current, pixel);
                        if (pixel < hmax)
                        {
                            if (add_pixel(pixel_q, r, c, out_row[c]) != SUCCESS)
                            {
                                free_pixel_queue(pixel_q);
                                RETURN_ERROR("Adding pixel to queue",
                                             FUNC_NAME, ERROR);
                            }
                        }
                    }
                }
            }

            /* Put the element on the free list */
            p->next = pixel_q->free_list;
            pixel_q->free_list = p;
        }
        h_current++;
    } while (h_current < hmax);

    free_pixel_queue(pixel_q);

    printf("main minima filling completed for %s band\n", band_name);

    return SUCCESS;
}
