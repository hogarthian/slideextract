/* slideextract -- extract slides from video
 *
 * Copyright (c) 2013, Angelo Haller <angelo@szanni.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "slideextract.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

//! Print version and exit program
static void
version ()
{
	puts("slideextract " VERSION);

	exit (0);
}

//! Print help and exit program
static void
help()
{
    puts ("Usage: slideextract [OPTION] infile outprefix\n\
\n\
Extract slides from video.\n\
\n\
  -g          Launch gui to set comparison region, press any key when done\n\
  -r X.Y:WxH  Manually set comparison region by specifying a starting point X.Y\n\
              and width & height WxH\n\
  -t THRESH   Set comparison threshold (default: 0.999)\n\
\n\
  -h  Display this help and exit\n\
  -V  Output version information and exit\n\
\n\
Select a comparision region (e.g. slide number) for faster and more\n\
accurate extraction.\n");

    exit (0);
}

int
main (int argc, char **argv)
{
    struct roi roi;
    bool gflag = 0;
    bool rflag = 0;
    int c;
    int ret;
    double threshold = 0.999;  // Default threshold

    while ((c = getopt(argc, argv, "gr:t:Vh")) != -1) {
        switch (c) {
            case 'g':
                gflag = 1;
                break;

            case 'r':
                rflag = 1;
                if (sscanf(optarg, "%d.%d:%dx%d", &roi.x, &roi.y, &roi.width, &roi.height) != 4)
                    help();
                break;

            case 't':
                threshold = atof(optarg);
                if (threshold <= 0 || threshold > 1) {
                    fprintf(stderr, "Threshold must be between 0 and 1\n");
                    exit(1);
                }
                break;

            case 'V':
                version();
                break;

            case 'h':
            case '?':
            default:
                help();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 2 || (gflag && rflag))
        help();

    const char *file = *argv++;
    const char *outprefix = *argv;

    if (rflag)
        return se_extract_slides(file, outprefix, &roi, threshold);

    if (gflag)
    {
        if ((ret = se_select_roi(file, &roi)))
            return ret;

        printf("Selected ROI: %d.%d:%dx%d\n", roi.x, roi.y, roi.width, roi.height);
        return se_extract_slides(file, outprefix, &roi, threshold);
    }

    return se_extract_slides(file, outprefix, NULL, threshold);
}
