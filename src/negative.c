/**************************************************************

	The program reads an BMP image file and creates a new
	image that is the negative of the input file.

**************************************************************/

#include "qdbmp.h"
#include <stdio.h>
#include "negative.h"
/* Creates a negative image of the input bitmap file */
int convert(char *input, char *output)
{
    UCHAR	r, g, b;
    UINT	width, height;
    UINT	x, y;
    BMP*	bmp;

    /* Check arguments */


    /* Read an image file */
    bmp = BMP_ReadFile( input );
    BMP_CHECK_ERROR( stdout, -1 );

    /* Get image's dimensions */
    width = BMP_GetWidth( bmp );
    height = BMP_GetHeight( bmp );

    /* Iterate through all the image's pixels */
    for ( x = 0 ; x < width ; ++x )
    {
        for ( y = 0 ; y < height ; ++y )
        {
            /* Get pixel's RGB values */
            BMP_GetPixelRGB( bmp, x, y, &r, &g, &b );

            /* Invert RGB values */
            BMP_SetPixelRGB( bmp, x, y, 255 - r, 255 - g, 255 - b );
        }
    }

    /* Save result */
    BMP_WriteFile( bmp, output );
    BMP_CHECK_ERROR( stdout, -2 );


    /* Free all memory allocated for the image */
    BMP_Free( bmp );

    return 0;
}
