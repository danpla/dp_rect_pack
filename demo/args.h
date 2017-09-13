
#pragma once


namespace args {


enum class ImageFormat {
    png,
    svg,
};


extern const char* inFile;
extern ImageFormat imageFormat;
extern const char* imagePrefix;
extern int maxPageSize[2];
extern int maxPages;
extern const char* outDir;
extern int padding[4];
extern int spacing[2];


void parse(int argc, char* argv[]);


}
