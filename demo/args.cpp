
#include "args.h"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>


namespace args {


const char* inFile = "";
ImageFormat imageFormat = ImageFormat::png;
const char* imagePrefix = "page_";
int maxPageSize[2] = {INT_MAX, INT_MAX};
int maxPages = 9999;
const char* outDir = "";
int padding[4];
int spacing[2];


const char* help = (
"dp_rect_pack demo\n"
"\n"
"Usage: %s [options...] input-file\n"
"\n"
"  input-file            File to read rectangles from, or \"-\" for stdin\n"
"\n"
"  -help                 Print this help and exit\n"
"  -image-format FORMAT  Output format of the image: \"png\" (default) or \"svg\"\n"
"  -image-prefix PREFIX  Prefix for image names. Default is \"%s\"\n"
"  -max-size SIZE        Maximum size of one page. Default is %i:%i\n"
"  -max-pages COUNT      Maximum number of pages. Default is %i\n"
"  -out-dir PATH         Output directory. Default is \".\"\n"
"  -padding PADDING      Page padding. Default is 0\n"
"  -spacing SPACING      Spacing between rectangles. Default is 0\n"
"\n"
"Input data format\n"
"  The contents of the input file should be whitespace-separated descriptions\n"
"  of rectangles in format WIDTHxHEIGHT.\n"
"\n"
"Formats of arguments\n"
"  The parameters that specify geometry allow to set either all values\n"
"  as colon-separated list or a single number as a shortcut, in which\n"
"  case all the remaining values will be set to that number.\n"
"  For example, -max-size 100 is the same as -max-size 100:100.\n"
"\n"
"  -max-size WIDTH[:HEIGHT]\n"
"  -padding TOP[:BOTTOM:LEFT:RIGHT]\n"
"  -spacing X[:Y]\n"
);


static void printHelp(const char* name)
{
    std::printf(
        help, name,
        imagePrefix,
        maxPageSize[0], maxPageSize[1], maxPages);
}


// We deliberately don't treat negative integers as invalid values
// for -max-size, -padding, and -spacing.
void parse(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "-help") == 0) {
            printHelp(argv[0]);
            std::exit(EXIT_SUCCESS);
        }

    if (argc < 2) {
        printHelp(argv[0]);
        std::exit(EXIT_FAILURE);
    }

    inFile = argv[argc - 1];

    // Subtract the number of positional arguments.
    argc -= 1;

    int missingArgument = 0;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-image-format") == 0) {
            ++i;
            if (i == argc) {
                missingArgument = i - 1;
                break;
            }

            if (std::strcmp(argv[i], "png") == 0)
                imageFormat = ImageFormat::png;
            else if (std::strcmp(argv[i], "svg") == 0)
                imageFormat = ImageFormat::svg;
            else {
                std::fprintf(stderr, "Unknown %s: %s\n", argv[i - 1], argv[i]);
                std::exit(EXIT_FAILURE);
            }
        } else if (std::strcmp(argv[i], "-image-prefix") == 0) {
            ++i;
            if (i == argc) {
                missingArgument = i - 1;
                break;
            }

            imagePrefix = argv[i];
        } else if (std::strcmp(argv[i], "-max-size") == 0) {
            ++i;
            if (i == argc) {
                missingArgument = i - 1;
                break;
            }

            const auto numRead = std::sscanf(
                argv[i], "%d:%d", &maxPageSize[0], &maxPageSize[1]);
            if (numRead < 1) {
                std::fprintf(stderr, "Invalid %s: %s\n", argv[i - 1], argv[i]);
                std::exit(EXIT_FAILURE);
            }

            if (numRead == 1)
                maxPageSize[1] = maxPageSize[0];
        } else if (std::strcmp(argv[i], "-max-pages") == 0) {
            ++i;
            if (i == argc) {
                missingArgument = i - 1;
                break;
            }

            char* end;
            maxPages = std::strtol(argv[i], &end, 10);
            if (argv[i] == end) {
                std::fprintf(stderr, "Invalid %s: %s\n", argv[i - 1], argv[i]);
                std::exit(EXIT_FAILURE);
            }

            if (maxPages <= 0)  {
                std::fprintf(stderr, "%s must be > 0\n", argv[i - 1]);
                std::exit(EXIT_FAILURE);
            }
        } else if (std::strcmp(argv[i], "-out-dir") == 0) {
            ++i;
            if (i == argc) {
                missingArgument = i - 1;
                break;
            }
            outDir = argv[i];
        } else if (std::strcmp(argv[i], "-padding") == 0) {
            ++i;
            if (i == argc) {
                missingArgument = i - 1;
                break;
            }

            const auto numRead = std::sscanf(
                argv[i],
                "%d:%d:%d:%d",
                &padding[0], &padding[1], &padding[2], &padding[3]);
            if (numRead != 1 && numRead != 4) {
                std::fprintf(stderr, "Invalid %s: %s\n", argv[i - 1], argv[i]);
                std::exit(EXIT_FAILURE);
            }

            if (numRead == 1)
                for (int i = 1; i < 4; ++i)
                    padding[i] = padding[0];
        } else if (std::strcmp(argv[i], "-spacing") == 0) {
            ++i;
            if (i == argc) {
                missingArgument = i - 1;
                break;
            }

            const auto numRead = std::sscanf(
                argv[i], "%d:%d", &spacing[0], &spacing[1]);
            if (numRead < 1) {
                std::fprintf(stderr, "Invalid %s: %s\n", argv[i - 1], argv[i]);
                std::exit(EXIT_FAILURE);
            }

            if (numRead == 1)
                spacing[1] = spacing[0];
        } else {
            std::fprintf(stderr, "Unknown option %s\n", argv[i]);
            std::exit(EXIT_FAILURE);
        }
    }

    if (missingArgument > 0) {
        std::fprintf(
            stderr, "%s expects an argument\n", argv[missingArgument]);
        std::exit(EXIT_FAILURE);
    }
}


}
