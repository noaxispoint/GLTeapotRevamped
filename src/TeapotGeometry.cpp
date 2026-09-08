// Utah teapot patch data and evaluator-based tessellation.
//
// The control-point/patch data below is the classic 10-patch teapot data
// set (mirrored to produce the full 32-patch teapot), originally released
// by Silicon Graphics, Inc. and long distributed as public/free data with
// GLUT and freeglut's glutSolidTeapot()/glutWireTeapot() (freeglut is
// MIT-licensed; SGI's original permission notice for the data itself is
// reproduced below as it appeared in that codebase).
//
//  (c) Copyright 1993, Silicon Graphics, Inc.
//  Permission to use, copy, modify, and distribute this software for any
//  purpose and without fee is hereby granted, provided that the above
//  copyright notice appears in all copies and that both the copyright
//  notice and this permission notice appear in supporting documentation,
//  and that the name of Silicon Graphics, Inc. not be used in advertising
//  or publicity pertaining to distribution of the software without
//  specific, written prior permission.
//
// The tessellation routine here is a fresh implementation of the standard
// "evaluate 10 patches, mirror across the axes" algorithm that has
// accompanied this data set for decades (see e.g. Mark Kilgard's GLUT and
// its descendants), written against Haiku's GLTeapot behavior (which draws
// pre-tessellated static geometry) so we get an equivalent teapot without
// depending on GLUT/freeglut at all.

#include "TeapotGeometry.h"

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace TeapotGeometry {

namespace {

const int kPatchData[10][16] = {
    { 102, 103, 104, 105,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15 }, // rim
    {  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27 }, // body
    {  24,  25,  26,  27,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40 },
    {  96,  96,  96,  96,  97,  98,  99, 100, 101, 101, 101, 101,   0,   1,   2,   3 }, // lid
    {   0,   1,   2,   3, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117 },
    { 118, 118, 118, 118, 124, 122, 119, 121, 123, 126, 125, 120,  40,  39,  38,  37 }, // bottom
    {  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56 }, // handle
    {  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  28,  65,  66,  67 },
    {  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83 }, // spout
    {  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95 },
};

const double kControlPoints[127][3] = {
    {0.2, 0, 2.7}, {0.2, -0.112, 2.7}, {0.112, -0.2, 2.7}, {0, -0.2, 2.7},
    {1.3375, 0, 2.53125}, {1.3375, -0.749, 2.53125}, {0.749, -1.3375, 2.53125}, {0, -1.3375, 2.53125},
    {1.4375, 0, 2.53125}, {1.4375, -0.805, 2.53125}, {0.805, -1.4375, 2.53125}, {0, -1.4375, 2.53125},
    {1.5, 0, 2.4}, {1.5, -0.84, 2.4}, {0.84, -1.5, 2.4}, {0, -1.5, 2.4},
    {1.75, 0, 1.875}, {1.75, -0.98, 1.875}, {0.98, -1.75, 1.875}, {0, -1.75, 1.875},
    {2, 0, 1.35}, {2, -1.12, 1.35}, {1.12, -2, 1.35}, {0, -2, 1.35},
    {2, 0, 0.9}, {2, -1.12, 0.9}, {1.12, -2, 0.9}, {0, -2, 0.9},
    {-2, 0, 0.9}, {2, 0, 0.45}, {2, -1.12, 0.45}, {1.12, -2, 0.45},
    {0, -2, 0.45}, {1.5, 0, 0.225}, {1.5, -0.84, 0.225}, {0.84, -1.5, 0.225},
    {0, -1.5, 0.225}, {1.5, 0, 0.15}, {1.5, -0.84, 0.15}, {0.84, -1.5, 0.15},
    {0, -1.5, 0.15}, {-1.6, 0, 2.025}, {-1.6, -0.3, 2.025}, {-1.5, -0.3, 2.25},
    {-1.5, 0, 2.25}, {-2.3, 0, 2.025}, {-2.3, -0.3, 2.025}, {-2.5, -0.3, 2.25},
    {-2.5, 0, 2.25}, {-2.7, 0, 2.025}, {-2.7, -0.3, 2.025}, {-3, -0.3, 2.25},
    {-3, 0, 2.25}, {-2.7, 0, 1.8}, {-2.7, -0.3, 1.8}, {-3, -0.3, 1.8},
    {-3, 0, 1.8}, {-2.7, 0, 1.575}, {-2.7, -0.3, 1.575}, {-3, -0.3, 1.35},
    {-3, 0, 1.35}, {-2.5, 0, 1.125}, {-2.5, -0.3, 1.125}, {-2.65, -0.3, 0.9375},
    {-2.65, 0, 0.9375}, {-2, -0.3, 0.9}, {-1.9, -0.3, 0.6}, {-1.9, 0, 0.6},
    {1.7, 0, 1.425}, {1.7, -0.66, 1.425}, {1.7, -0.66, 0.6}, {1.7, 0, 0.6},
    {2.6, 0, 1.425}, {2.6, -0.66, 1.425}, {3.1, -0.66, 0.825}, {3.1, 0, 0.825},
    {2.3, 0, 2.1}, {2.3, -0.25, 2.1}, {2.4, -0.25, 2.025}, {2.4, 0, 2.025},
    {2.7, 0, 2.4}, {2.7, -0.25, 2.4}, {3.3, -0.25, 2.4}, {3.3, 0, 2.4},
    {2.8, 0, 2.475}, {2.8, -0.25, 2.475}, {3.525, -0.25, 2.49375}, {3.525, 0, 2.49375},
    {2.9, 0, 2.475}, {2.9, -0.15, 2.475}, {3.45, -0.15, 2.5125}, {3.45, 0, 2.5125},
    {2.8, 0, 2.4}, {2.8, -0.15, 2.4}, {3.2, -0.15, 2.4}, {3.2, 0, 2.4},
    {0, 0, 3.15}, {0.8, 0, 3.15}, {0.8, -0.45, 3.15}, {0.45, -0.8, 3.15},
    {0, -0.8, 3.15}, {0, 0, 2.85}, {1.4, 0, 2.4}, {1.4, -0.784, 2.4},
    {0.784, -1.4, 2.4}, {0, -1.4, 2.4}, {0.4, 0, 2.55}, {0.4, -0.224, 2.55},
    {0.224, -0.4, 2.55}, {0, -0.4, 2.55}, {1.3, 0, 2.55}, {1.3, -0.728, 2.55},
    {0.728, -1.3, 2.55}, {0, -1.3, 2.55}, {1.3, 0, 2.4}, {1.3, -0.728, 2.4},
    {0.728, -1.3, 2.4}, {0, -1.3, 2.4}, {0, 0, 0}, {1.425, -0.798, 0},
    {1.5, 0, 0.075}, {1.425, 0, 0}, {0.798, -1.425, 0}, {0, -1.5, 0.075},
    {0, -1.425, 0}, {1.5, -0.84, 0.075}, {0.84, -1.5, 0.075},
};

const double kTexCoords[2][2][2] = {
    { {0.0, 0.0}, {1.0, 0.0} },
    { {0.0, 1.0}, {1.0, 1.0} },
};

// Evaluates one 4x4 control-point patch, optionally negating the X and/or Y
// control-point coordinates to mirror it, and optionally reversing the
// column (k) traversal order. The column reversal is required whenever
// exactly one axis is mirrored, to keep the resulting surface's winding
// (and thus its GL_AUTO_NORMAL-derived normals) consistent.
void EvalPatch(int patchIndex, int grid, bool negateX, bool negateY, bool reverseColumns) {
    double p[4][4][3];

    for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 4; k++) {
            int col = reverseColumns ? (3 - k) : k;
            int cpIndex = kPatchData[patchIndex][j * 4 + col];
            for (int l = 0; l < 3; l++) {
                double v = kControlPoints[cpIndex][l];
                if (l == 0 && negateX) v = -v;
                if (l == 1 && negateY) v = -v;
                p[j][k][l] = v;
            }
        }
    }

    glMap2d(GL_MAP2_TEXTURE_COORD_2, 0.0, 1.0, 2, 2, 0.0, 1.0, 4, 2, &kTexCoords[0][0][0]);
    glMap2d(GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4, 0.0, 1.0, 12, 4, &p[0][0][0]);
    glMapGrid2d(grid, 0.0, 1.0, grid, 0.0, 1.0);
    glEvalMesh2(GL_FILL, 0, grid, 0, grid);
}

} // namespace

void Draw(int grid) {
    glPushAttrib(GL_ENABLE_BIT | GL_EVAL_BIT);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_MAP2_VERTEX_3);
    glEnable(GL_MAP2_TEXTURE_COORD_2);

    glPushMatrix();
    // Stand the teapot upright and center it roughly at the origin. The
    // scale is smaller than glutSolidTeapot()'s traditional 0.5 because
    // GLTeapot's camera/ortho volume was tuned for a unit-radius model.
    glRotated(270.0, 1.0, 0.0, 0.0);
    glScaled(0.25, 0.25, 0.25);
    glTranslated(0.0, 0.0, -1.5);

    for (int i = 0; i < 10; i++) {
        // Body/lid/rim/bottom (i < 6) mirror into all four quadrants;
        // handle/spout (i >= 6) only mirror across the Y axis (2 pieces).
        EvalPatch(i, grid, /*negateX=*/false, /*negateY=*/false, /*reverseColumns=*/false);
        EvalPatch(i, grid, /*negateX=*/false, /*negateY=*/true,  /*reverseColumns=*/true);
        if (i < 6) {
            EvalPatch(i, grid, /*negateX=*/true, /*negateY=*/false, /*reverseColumns=*/true);
            EvalPatch(i, grid, /*negateX=*/true, /*negateY=*/true,  /*reverseColumns=*/false);
        }
    }

    glPopMatrix();
    glPopAttrib();
}

} // namespace TeapotGeometry
