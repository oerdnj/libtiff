/* $Id: short_tag.c,v 1.2 2004-09-14 15:14:21 dron Exp $ */

/*
 * Copyright (c) 2004, Andrey Kiselev  <dron@remotesensing.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library
 *
 * Module to test SHORT tags read/write functions.
 */

#include "tif_config.h"

#include <stdio.h>

#ifdef HAVE_UNISTD_H 
# include <unistd.h> 
#endif 

#include "tiffio.h"

const char	*filename = "short_test.tiff";

#define	SPP	3		/* Samples per pixel */
const uint16	width = 1;
const uint16	length = 1;
const uint16	bps = 8;
const uint16	photometric = PHOTOMETRIC_RGB;
const uint16	rows_per_strip = 1;
const uint16	planarconfig = PLANARCONFIG_CONTIG;

static struct SingleTags {
	ttag_t		tag;
	uint16		value;
} short_single_tags[] = {
	{ TIFFTAG_COMPRESSION, COMPRESSION_NONE },
	{ TIFFTAG_FILLORDER, FILLORDER_MSB2LSB },
	{ TIFFTAG_ORIENTATION, ORIENTATION_BOTRIGHT },
	{ TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH },
	{ TIFFTAG_INKSET, INKSET_MULTIINK },
	{ TIFFTAG_MINSAMPLEVALUE, 23 },
	{ TIFFTAG_MAXSAMPLEVALUE, 241 },
	{ TIFFTAG_NUMBEROFINKS, SPP },
	{ TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT },
	{ TIFFTAG_IMAGEDEPTH, 1 },
	{ TIFFTAG_TILEDEPTH, 1 }
};
#define NSINGLETAGS   (sizeof(short_single_tags) / sizeof(short_single_tags[0]))

int
main(int argc, char **argv)
{
	TIFF		*tif;
	int		i;
	unsigned char	buf[3] = { 0, 127, 255 };
	uint16		value;

	/* Test whether we can write tags. */
	tif = TIFFOpen(filename, "w");
	if (!tif) {
		fprintf (stderr, "Can't create test TIFF file %s.\n", filename);
		return 1;
	}

	if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width)) {
		fprintf (stderr, "Can't set ImageWidth tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, length)) {
		fprintf (stderr, "Can't set ImageLength tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps)) {
		fprintf (stderr, "Can't set BitsPerSample tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, SPP)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, planarconfig)) {
		fprintf (stderr, "Can't set PlanarConfiguration tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric)) {
		fprintf (stderr, "Can't set PhotometricInterpretation tag.\n");
		goto failure;
	}

	for (i = 0; i < NSINGLETAGS; i++) {
		if (!TIFFSetField(tif, short_single_tags[i].tag,
				  short_single_tags[i].value)) {
			fprintf(stderr, "Can't set tag %d.\n",
				(int)short_single_tags[i].tag);
			goto failure;
		}
	}

	/* Write dummy pixel data. */
	if (!TIFFWriteScanline(tif, buf, 0, 0) < 0) {
		fprintf (stderr, "Can't write image data.\n");
		goto failure;
	}

	TIFFClose(tif);
	
	/* Ok, now test whether we can read written values. */
	tif = TIFFOpen(filename, "r");
	if (!tif) {
		fprintf (stderr, "Can't open test TIFF file %s.\n", filename);
		return 1;
	}
	if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &value)
	    || value != width) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_IMAGEWIDTH);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &value)
	    || value != length) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_IMAGELENGTH);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &value)
	    || value != bps) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_BITSPERSAMPLE);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &value)
	    || value != photometric) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_PHOTOMETRIC);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &value)
	    || value != SPP) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_SAMPLESPERPIXEL);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &value)
	    || value != rows_per_strip) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_ROWSPERSTRIP);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &value)
	    || value != planarconfig) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_PLANARCONFIG);
		goto failure;
	}

	for (i = 0; i < NSINGLETAGS; i++) {
		if (!TIFFGetField(tif, short_single_tags[i].tag, &value)
		    || value != short_single_tags[i].value) {
			fprintf(stderr, "Can't get tag %d.\n",
				(int)short_single_tags[i].tag);
			goto failure;
		}
	}

	TIFFClose(tif);
	
	/* All tests passed; delete file and exit with success status. */
	unlink(filename);
	return 0;

failure:
	/* Something goes wrong; close file and return unsuccessful status. */
	TIFFClose(tif);
	unlink(filename);
	return 1;
}

/* vim: set ts=8 sts=8 sw=8 noet: */
