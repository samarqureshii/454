#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "utilities.h"  // DO NOT REMOVE this line
#include "implementation_reference.h"   // DO NOT REMOVE this line

// [x;y] = A * [u;v] + t   where A is 2x2 with entries in {-1,0,1}
static int A00 = 1, A01 = 0, A10 = 0, A11 = 1;  // identity
static int TX = 0, TY = 0;                      // translation vector
static int frame_index = 0; 

// A := G*A ; t := G*t + g

static void print_frame_transform(int frame_index, unsigned width, unsigned height)
{
    (void)width; (void)height; 
    printf("Frame %d transform:\n", frame_index);
    printf("  A = [ [%d %d]\n        [%d %d] ]\n", A00, A01, A10, A11);
    printf("  t = [ %d %d ]\n", TX, TY);

}


// unsigned char *processMoveUp(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
// unsigned char *processMoveDown(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
// unsigned char *processMoveLeft(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
// unsigned char *processMoveRight(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
// unsigned char *processRotateCW(unsigned char *buffer_frame, unsigned width, unsigned height,
//                                int rotate_iteration);
// unsigned char *processRotateCCW(unsigned char *buffer_frame, unsigned width, unsigned height,
//                                int rotate_iteration);
// unsigned char *processMirrorX(unsigned char *buffer_frame, unsigned width, unsigned height, int _unused);
// unsigned char *processMirrorY(unsigned char *buffer_frame, unsigned width, unsigned height, int _unused);


// /***********************************************************************************************************************
//  * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
//  * @param width - width of the imported 24-bit bitmap image
//  * @param height - height of the imported 24-bit bitmap image
//  * @param offset - number of pixels to shift the object in bitmap image up
//  * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
//  * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
//  * Note2: You can assume the object will never be moved off the screen
//  **********************************************************************************************************************/
// unsigned char *processMoveUp(unsigned char *buffer_frame, unsigned width, unsigned height, int offset) {
//     if (offset < 0) { return processMoveDown(buffer_frame, width, height, -offset); }
//     TY+=-offset;
//     return buffer_frame;
// }

// /***********************************************************************************************************************
//  * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
//  * @param width - width of the imported 24-bit bitmap image
//  * @param height - height of the imported 24-bit bitmap image
//  * @param offset - number of pixels to shift the object in bitmap image left
//  * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
//  * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
//  * Note2: You can assume the object will never be moved off the screen
//  **********************************************************************************************************************/
// unsigned char *processMoveRight(unsigned char *buffer_frame, unsigned width, unsigned height, int offset) {
//     if (offset < 0) { return processMoveLeft(buffer_frame, width, height, -offset); }
//     TX+=offset;
//     return buffer_frame;
// }

// /***********************************************************************************************************************
//  * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
//  * @param width - width of the imported 24-bit bitmap image
//  * @param height - height of the imported 24-bit bitmap image
//  * @param offset - number of pixels to shift the object in bitmap image up
//  * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
//  * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
//  * Note2: You can assume the object will never be moved off the screen
//  **********************************************************************************************************************/
// unsigned char *processMoveDown(unsigned char *buffer_frame, unsigned width, unsigned height, int offset) {
//     if (offset < 0) { return processMoveUp(buffer_frame, width, height, -offset); }
//     TY+=offset;
//     return buffer_frame;
// }

// /***********************************************************************************************************************
//  * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
//  * @param width - width of the imported 24-bit bitmap image
//  * @param height - height of the imported 24-bit bitmap image
//  * @param offset - number of pixels to shift the object in bitmap image right
//  * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
//  * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
//  * Note2: You can assume the object will never be moved off the screen
//  **********************************************************************************************************************/
// unsigned char *processMoveLeft(unsigned char *buffer_frame, unsigned width, unsigned height, int offset) {
//     if (offset < 0) { return processMoveRight(buffer_frame, width, height, -offset); }
//     TX+=-offset;
//     return buffer_frame;
// }

// /***********************************************************************************************************************
//  * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
//  * @param width - width of the imported 24-bit bitmap image
//  * @param height - height of the imported 24-bit bitmap image
//  * @param rotate_iteration - rotate object inside frame buffer clockwise by 90 degrees, <iteration> times
//  * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
//  * Note: You can assume the frame will always be square and you will be rotating the entire image
//  **********************************************************************************************************************/
// unsigned char *processRotateCW(unsigned char *buffer_frame, unsigned width, unsigned height,
//                                int rotate_iteration) {
//     if (rotate_iteration < 0) { return processRotateCCW(buffer_frame, width, height, -rotate_iteration); }
//     int k = rotate_iteration & 3;  // mod 4
//     for (int i = 0; i < k; ++i) {
//         // As-is: G = [[0,-1],[1,0]], g = (width-1, 0)
//         int nA00 = -A10;
//         int nA01 = -A11;
//         int nA10 =  A00;
//         int nA11 =  A01;
//         int nTX  = -TY + (int)width - 1;
//         int nTY  =  TX;

//         A00 = nA00; A01 = nA01; A10 = nA10; A11 = nA11;
//         TX  = nTX;  TY  = nTY;
//     }
//     (void)height; // keep signature parity
//     return buffer_frame;
// }

// /***********************************************************************************************************************
//  * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
//  * @param width - width of the imported 24-bit bitmap image
//  * @param height - height of the imported 24-bit bitmap image
//  * @param rotate_iteration - rotate object inside frame buffer counter clockwise by 90 degrees, <iteration> times
//  * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
//  * Note: You can assume the frame will always be square and you will be rotating the entire image
//  **********************************************************************************************************************/
// unsigned char *processRotateCCW(unsigned char *buffer_frame, unsigned width, unsigned height,
//                                 int rotate_iteration) {
//     if (rotate_iteration < 0) { return processRotateCW(buffer_frame, width, height, -rotate_iteration); }
//     int k = rotate_iteration & 3;  // mod 4
//     for (int i = 0; i < k; ++i) {
//         // As-is: G = [[0,1],[-1,0]], g = (0, height-1)
//         int nA00 =  A10;
//         int nA01 =  A11;
//         int nA10 = -A00;
//         int nA11 = -A01;
//         int nTX  =  TY;
//         int nTY  = -TX + (int)height - 1;

//         A00 = nA00; A01 = nA01; A10 = nA10; A11 = nA11;
//         TX  = nTX;  TY  = nTY;
//     }
//     (void)width; // keep signature parity
//     return buffer_frame;
// }


// /***********************************************************************************************************************
//  * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
//  * @param width - width of the imported 24-bit bitmap image
//  * @param height - height of the imported 24-bit bitmap image
//  * @param _unused - this field is unused
//  * @return
//  **********************************************************************************************************************/
// unsigned char *processMirrorX(unsigned char *buffer_frame, unsigned width, unsigned height, int _unused) {
//     // As-is: G = diag(1,-1), g = (0, height-1)
//     int nA00 =  A00;
//     int nA01 =  A01;
//     int nA10 = -A10;
//     int nA11 = -A11;
//     int nTX  =  TX;
//     int nTY  = -TY + (int)height - 1;

//     A00 = nA00; A01 = nA01; A10 = nA10; A11 = nA11;
//     TX  = nTX;  TY  = nTY;
//     (void)width; (void)_unused;
//     return buffer_frame;
// }


// /***********************************************************************************************************************
//  * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
//  * @param width - width of the imported 24-bit bitmap image
//  * @param height - height of the imported 24-bit bitmap image
//  * @param _unused - this field is unused
//  * @return
//  **********************************************************************************************************************/
// unsigned char *processMirrorY(unsigned char *buffer_frame, unsigned width, unsigned height, int _unused) {
//     // As-is: G = diag(-1,1), g = (width-1, 0)
//     int nA00 = -A00;
//     int nA01 = -A01;
//     int nA10 =  A10;
//     int nA11 =  A11;
//     int nTX  = -TX + (int)width - 1;
//     int nTY  =  TY;

//     A00 = nA00; A01 = nA01; A10 = nA10; A11 = nA11;
//     TX  = nTX;  TY  = nTY;
//     (void)height; (void)_unused;
//     return buffer_frame;
// }


/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          Do not forget to modify the team_name and team member information !!!
 **********************************************************************************************************************/
void print_team_info(){
    // Please modify this field with something interesting
    char team_name[] = "The Real Ashvin Goel";

    // Please fill in your information
    char student_first_name[] = "Samar";
    char student_last_name[] = "Qureshi";
    char student_student_number[] = "1009149583";

    // Printing out team information
    printf("*******************************************************************************************************\n");
    printf("Team Information:\n");
    printf("\tteam_name: %s\n", team_name);
    printf("\tstudent_first_name: %s\n", student_first_name);
    printf("\tstudent_last_name: %s\n", student_last_name);
    printf("\tstudent_student_number: %s\n", student_student_number);
}

/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          You can modify anything else in this file
 ***********************************************************************************************************************
 * @param sensor_values - structure stores parsed key value pairs of program instructions
 * @param sensor_values_count - number of valid sensor values parsed from sensor log file or commandline console
 * @param frame_buffer - pointer pointing to a buffer storing the imported  24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param grading_mode - turns off verification and turn on instrumentation
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
void implementation_driver(struct kv *sensor_values, int sensor_values_count, unsigned char *frame_buffer,
                           unsigned int width, unsigned int height, bool grading_mode)
{
    unsigned char *bufA = allocateFrame(width, height);
    unsigned char *bufB = allocateFrame(width, height);
    unsigned char *src  = bufA;   // current readable image
    unsigned char *dst  = bufB;   // render target
    const size_t rowBytes = (size_t)width * 3;

    // 1) initialize src from the incoming frame
    copyFrame(frame_buffer, src, width, height);

    // 2) reset affine accumulator
    A00 = 1; A01 = 0; A10 = 0; A11 = 1; TX = 0; TY = 0;

    int processed_ops = 0;

    for (int i = 0; i < sensor_values_count; ++i) {
        const char *k = sensor_values[i].key;
        int v = sensor_values[i].value;

        // accmulate only
        if (!strcmp(k, "W"))            { TY -= v; }
        else if (!strcmp(k, "A"))       { TX -= v; }
        else if (!strcmp(k, "S"))       { TY += v; }
        else if (!strcmp(k, "D"))       { TX += v; }
        else if (!strcmp(k, "CW")) {
            int k4 = ((v & 3) + 4) & 3;
            for (int r = 0; r < k4; ++r) {
                int nA00 = -A10, nA01 = -A11, nA10 = A00, nA11 = A01;
                int nTX = -TY + (int)width - 1, nTY = TX;
                A00=nA00; 
                A01=nA01; 
                A10=nA10; 
                A11=nA11; 
                TX=nTX; 
                TY=nTY;
            }
        } else if (!strcmp(k, "CCW")) {
            int k4 = ((v & 3) + 4) & 3;
            for (int r = 0; r < k4; ++r) {
                int nA00 = A10, nA01 = A11, nA10 = -A00, nA11 = -A01;
                int nTX = TY, nTY = -TX + (int)height - 1;
                A00=nA00; 
                A01=nA01; 
                A10=nA10; 
                A11=nA11; 
                TX=nTX; 
                TY=nTY;
            }
        } else if (!strcmp(k, "MX")) {
            A10 = -A10; A11 = -A11; TY = -TY + (int)height - 1;
        } else if (!strcmp(k, "MY")) {
            A00 = -A00; A01 = -A01; TX = -TX + (int)width - 1;
        }

        processed_ops++;

        if (processed_ops % 25 == 0) {
            // clear destination
            for (unsigned y = 0; y < height; ++y)
                memset(dst + (size_t)y * rowBytes, 255, rowBytes);

            // render source
            if (A01 == 0 && A10 == 0) { //horizontal translation
                if (A00 == 1) { //identity x scale
                    for (unsigned vrow = 0; vrow < height; ++vrow) {
                        int ydst = A11 * (int)vrow + TY;
                        if ((unsigned)ydst >= height) continue;
                        int xdst0 = TX;
                        int xdstLo = xdst0 < 0 ? 0 : xdst0;
                        int xdstHi = xdst0 + (int)width - 1;
                        if (xdstHi >= (int)width) xdstHi = (int)width - 1;
                        if (xdstLo > xdstHi) continue;
                        unsigned run_len = (unsigned)(xdstHi - xdstLo + 1);
                        size_t sp = ((size_t)vrow * width + (size_t)(xdstLo - xdst0)) * 3;
                        size_t dp = ((size_t)ydst * width + (size_t)xdstLo) * 3;
                        memcpy(dst + dp, src + sp, (size_t)run_len * 3);
                    }
                } else { // A00 == -1, x miror
                    for (unsigned vrow = 0; vrow < height; ++vrow) {
                        int ydst = A11 * (int)vrow + TY;
                        if ((unsigned)ydst >= height) continue;
                        int u_lo = 0, u_hi = (int)width - 1;
                        if (u_lo < TX - ((int)width - 1)) u_lo = TX - ((int)width - 1);
                        if (u_hi > TX)                    u_hi = TX;
                        if (u_lo < 0) u_lo = 0;
                        if (u_hi > (int)width - 1) u_hi = (int)width - 1;
                        if (u_lo > u_hi) continue;
                        unsigned run_len = (unsigned)(u_hi - u_lo + 1);
                        int xdstLo = TX - u_hi;
                        size_t dp = ((size_t)ydst * width + (size_t)xdstLo) * 3;
                        size_t sp0 = ((size_t)vrow * width + (size_t)u_hi) * 3;
                        for (unsigned k = 0; k < run_len; ++k) {
                            size_t sp = sp0 - (size_t)k * 3;
                            size_t dpk = dp + (size_t)k * 3;
                            //3B pixel cp
                            dst[dpk + 0] = src[sp + 0];
                            dst[dpk + 1] = src[sp + 1];
                            dst[dpk + 2] = src[sp + 2];
                        }
                    }
                }
            } 
            else { // general affine transformation
                for (unsigned vrow = 0; vrow < height; ++vrow) { //
                    int x = A01 * (int)vrow + TX;
                    int y = A11 * (int)vrow + TY;
                    const int dx = A00, dy = A10;
                    size_t sp = ((size_t)vrow * width + 0) * 3;
                    for (unsigned u = 0; u < width; ++u) {
                        if ((unsigned)x < width && (unsigned)y < height) {
                            size_t dp = ((size_t)y * width + (size_t)x) * 3;
                            dst[dp + 0] = src[sp + 0];
                            dst[dp + 1] = src[sp + 1];
                            dst[dp + 2] = src[sp + 2];
                        }
                        sp += 3; x += dx; y += dy;
                    }
                }
            }

            // swap the source and destination buffers
            unsigned char *tmp = src; src = dst; dst = tmp;

            copyFrame(src, frame_buffer, width, height);
            verifyFrame(frame_buffer, width, height, grading_mode);
            //print_frame_transform(frame_index++, width, height);

            // reset affine
            A00 = 1; A01 = 0; A10 = 0; A11 = 1; TX = 0; TY = 0;
        }
    }

    // if we dont have a clean % 25
    if (processed_ops % 25 != 0) {
        for (unsigned y = 0; y < height; ++y)
            memset(dst + (size_t)y * rowBytes, 255, rowBytes);
        if (A01 == 0 && A10 == 0) {
            if (A00 == 1) {
                for (unsigned vrow = 0; vrow < height; ++vrow) {
                    int ydst = A11 * (int)vrow + TY;
                    if ((unsigned)ydst >= height) continue;
                    int xdst0 = TX;
                    int xdstLo = xdst0 < 0 ? 0 : xdst0;
                    int xdstHi = xdst0 + (int)width - 1;
                    if (xdstHi >= (int)width) xdstHi = (int)width - 1;
                    if (xdstLo > xdstHi) continue;
                    unsigned run_len = (unsigned)(xdstHi - xdstLo + 1);
                    size_t sp = ((size_t)vrow * width + (size_t)(xdstLo - xdst0)) * 3;
                    size_t dp = ((size_t)ydst * width + (size_t)xdstLo) * 3;
                    memcpy(dst + dp, src + sp, (size_t)run_len * 3);
                }
            } else {
                for (unsigned vrow = 0; vrow < height; ++vrow) {
                    int ydst = A11 * (int)vrow + TY;
                    if ((unsigned)ydst >= height) continue;
                    int u_lo = 0, u_hi = (int)width - 1;
                    if (u_lo < TX - ((int)width - 1)) u_lo = TX - ((int)width - 1);
                    if (u_hi > TX)                    u_hi = TX;
                    if (u_lo < 0) u_lo = 0;
                    if (u_hi > (int)width - 1) u_hi = (int)width - 1;
                    if (u_lo > u_hi) continue;
                    unsigned run_len = (unsigned)(u_hi - u_lo + 1);
                    int xdstLo = TX - u_hi;
                    size_t dp = ((size_t)ydst * width + (size_t)xdstLo) * 3;
                    size_t sp0 = ((size_t)vrow * width + (size_t)u_hi) * 3;
                    for (unsigned k = 0; k < run_len; ++k) {
                        size_t sp = sp0 - (size_t)k * 3;
                        size_t dpk = dp + (size_t)k * 3;
                        dst[dpk + 0] = src[sp + 0];
                        dst[dpk + 1] = src[sp + 1];
                        dst[dpk + 2] = src[sp + 2];
                    }
                }
            }
        } else {
            for (unsigned vrow = 0; vrow < height; ++vrow) {
                int x = A01 * (int)vrow + TX;
                int y = A11 * (int)vrow + TY;
                const int dx = A00, dy = A10;
                size_t sp = ((size_t)vrow * width + 0) * 3;
                for (unsigned u = 0; u < width; ++u) {
                    if ((unsigned)x < width && (unsigned)y < height) {
                        size_t dp = ((size_t)y * width + (size_t)x) * 3;
                        dst[dp + 0] = src[sp + 0];
                        dst[dp + 1] = src[sp + 1];
                        dst[dp + 2] = src[sp + 2];
                    }
                    sp += 3; x += dx; y += dy;
                }
            }
        }
        // swap so src always holds the latest image
        unsigned char *tmp = src; src = dst; dst = tmp;

        copyFrame(src, frame_buffer, width, height);
        verifyFrame(frame_buffer, width, height, grading_mode);

        A00 = 1; A01 = 0; A10 = 0; A11 = 1; TX = 0; TY = 0;
    }

    deallocateFrame(bufA);
    deallocateFrame(bufB);
}
