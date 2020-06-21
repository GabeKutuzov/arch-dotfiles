/*      @(#)xglut.h 1.22 91/09/06 SMI   */
/* $Id: xglut.h 1 2008-12-19 16:56:22Z  $ */

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 * ==>, 07/01/02, GNR - Last date before committing to svn repository
 */

#include <stdio.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#ifdef OLIT
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h>
#include <Xol/DrawArea.h>
#endif

#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/textsw.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <xview/xv_xrect.h>
#include <xview/alert.h>

#include <xgl/xgl.h>

/* a data-structure-less interface that some people feel is
 * more convenient.
 *
 * Don't use these macros for performance.  They are provided
 * for ease of use.
 *
 * These macros are totally unsupported.
 */

/* a integer 2D line */
#define XGL_LINE_I2D(CTX, X0, Y0, X1, Y1)               \
    {                                                   \
        Xgl_pt_list     pl;                             \
        Xgl_pt_i2d      i2d[2];                         \
        i2d[0].x = X0; i2d[0].y = Y0;                   \
        i2d[1].x = X1; i2d[1].y = Y1;                   \
        pl.pt_type = XGL_PT_I2D;                        \
        pl.bbox = NULL;                                 \
        pl.num_pts = 2;                                 \
        pl.pts.i2d = &i2d[0];                           \
        xgl_multipolyline(CTX, NULL, 1, &pl);           \
    }

/* a float 2D line */
#define XGL_LINE_F2D(CTX, X0, Y0, X1, Y1)               \
    {                                                   \
        Xgl_pt_list     pl;                             \
        Xgl_pt_f2d      f2d[2];                         \
        f2d[0].x = X0; f2d[0].y = Y0;                   \
        f2d[1].x = X1; f2d[1].y = Y1;                   \
        pl.pt_type = XGL_PT_F2D;                        \
        pl.bbox = NULL;                                 \
        pl.num_pts = 2;                                 \
        pl.pts.f2d = &f2d[0];                           \
        xgl_multipolyline(CTX, NULL, 1, &pl);           \
    }

/* a fract 2D line */
#define XGL_LINE_B2D(CTX, X0, Y0, X1, Y1)               \
    {                                                   \
        Xgl_pt_list     pl;                             \
        Xgl_pt_b2d      b2d[2];                         \
        b2d[0].x = X0; b2d[0].y = Y0;                   \
        b2d[1].x = X1; b2d[1].y = Y1;                   \
        pl.pt_type = XGL_PT_B2D;                        \
        pl.bbox = NULL;                                 \
        pl.num_pts = 2;                                 \
        pl.pts.b2d = &b2d[0];                           \
        xgl_multipolyline(CTX, NULL, 1, &pl);           \
    }

/* a integer 2D triangle */
#define XGL_TRIANGLE_I2D(CTX, X0, Y0, X1, Y1, X2, Y2)   \
    {                                                   \
        Xgl_pt_list     pl;                             \
        Xgl_pt_i2d      i2d[3];                         \
        i2d[0].x = X0; i2d[0].y = Y0;                   \
        i2d[1].x = X1; i2d[1].y = Y1;                   \
        i2d[2].x = X2; i2d[2].y = Y2;                   \
        pl.pt_type = XGL_PT_I2D;                        \
        pl.bbox = NULL;                                 \
        pl.num_pts = 3;                                 \
        pl.pts.i2d = &i2d[0];                           \
        xgl_multi_simple_polygon(CTX,                   \
           XGL_FACET_FLAG_SIDES_ARE_3,                  \
           NULL, NULL, 1, &pl);                         \
    }

/* a float 2D triangle */
#define XGL_TRIANGLE_F2D(CTX, X0, Y0, X1, Y1, X2, Y2)   \
    {                                                   \
        Xgl_pt_list     pl;                             \
        Xgl_pt_f2d      f2d[3];                         \
        f2d[0].x = X0; f2d[0].y = Y0;                   \
        f2d[1].x = X1; f2d[1].y = Y1;                   \
        f2d[2].x = X2; f2d[2].y = Y2;                   \
        pl.pt_type = XGL_PT_F2D;                        \
        pl.bbox = NULL;                                 \
        pl.num_pts = 3;                                 \
        pl.pts.f2d = &f2d[0];                           \
        xgl_multi_simple_polygon(CTX,                   \
           XGL_FACET_FLAG_SIDES_ARE_3,                  \
           NULL, NULL, 1, &pl);                         \
    }

/* a fract 2D triangle */
#define XGL_TRIANGLE_B2D(CTX, X0, Y0, X1, Y1, X2, Y2)   \
    {                                                   \
        Xgl_pt_list     pl;                             \
        Xgl_pt_b2d      b2d[3];                         \
        b2d[0].x = X0; b2d[0].y = Y0;                   \
        b2d[1].x = X1; b2d[1].y = Y1;                   \
        b2d[2].x = X2; b2d[2].y = Y2;                   \
        pl.pt_type = XGL_PT_B2D;                        \
        pl.bbox = NULL;                                 \
        pl.num_pts = 3;                                 \
        pl.pts.b2d = &b2d[0];                           \
        xgl_multi_simple_polygon(CTX,                   \
           XGL_FACET_FLAG_SIDES_ARE_3,                  \
           NULL, NULL, 1, &pl);                         \
    }

/* a integer 2D quad */
#define XGL_QUAD_I2D(CTX, X0, Y0, X1, Y1, X2, Y2, X3, Y3)       \
    {                                                           \
        Xgl_pt_list     pl;                                     \
        Xgl_pt_i2d      i2d[3];                                 \
        i2d[0].x = X0; i2d[0].y = Y0;                           \
        i2d[1].x = X1; i2d[1].y = Y1;                           \
        i2d[2].x = X2; i2d[2].y = Y2;                           \
        i2d[3].x = X3; i2d[3].y = Y3;                           \
        pl.pt_type = XGL_PT_I2D;                                \
        pl.bbox = NULL;                                         \
        pl.num_pts = 4;                                         \
        pl.pts.i2d = &i2d[0];                                   \
        xgl_multi_simple_polygon(CTX,                           \
           XGL_FACET_FLAG_SIDES_ARE_4,                          \
           NULL, NULL, 1, &pl);                                 \
    }

/* a float 2D quad */
#define XGL_QUAD_F2D(CTX, X0, Y0, X1, Y1, X2, Y2, X3, Y3)       \
    {                                                           \
        Xgl_pt_list     pl;                                     \
        Xgl_pt_f2d      f2d[3];                                 \
        f2d[0].x = X0; f2d[0].y = Y0;                           \
        f2d[1].x = X1; f2d[1].y = Y1;                           \
        f2d[2].x = X2; f2d[2].y = Y2;                           \
        f2d[3].x = X3; f2d[3].y = Y3;                           \
        pl.pt_type = XGL_PT_F2D;                                \
        pl.bbox = NULL;                                         \
        pl.num_pts = 4;                                         \
        pl.pts.f2d = &f2d[0];                                   \
        xgl_multi_simple_polygon(CTX,                           \
           XGL_FACET_FLAG_SIDES_ARE_4,                          \
           NULL, NULL, 1, &pl);                                 \
    }

/* a fract 2D quad */
#define XGL_QUAD_B2D(CTX, X0, Y0, X1, Y1, X2, Y2, X3, Y3)       \
    {                                                           \
        Xgl_pt_list     pl;                                     \
        Xgl_pt_b2d      b2d[3];                                 \
        b2d[0].x = X0; b2d[0].y = Y0;                           \
        b2d[1].x = X1; b2d[1].y = Y1;                           \
        b2d[2].x = X2; b2d[2].y = Y2;                           \
        b2d[3].x = X3; b2d[3].y = Y1;                           \
        pl.pt_type = XGL_PT_B2D;                                \
        pl.bbox = NULL;                                         \
        pl.num_pts = 4;                                         \
        pl.pts.b2d = &b2d[0];                                   \
        xgl_multi_simple_polygon(CTX,                           \
           XGL_FACET_FLAG_SIDES_ARE_4,                          \
           NULL, NULL, 1, &pl);                                 \
    }

/* a circle using integer 2D coordinates */
#define XGL_CIRCLE_I2D(CTX, CX, CY, RAD)                        \
    {                                                           \
        Xgl_circle_list cl;                                     \
        Xgl_circle_i2d  i2d[1];                                 \
        cl.num_circles = 1;                                     \
        cl.type = XGL_MULTICIRCLE_I2D;                          \
        cl.bbox = NULL;                                         \
        cl.circles.i2d = &i2d[0];                               \
        i2d.center.flag = 0;                                    \
        i2d.center.x = CX;                                      \
        i2d.center.y = CY;                                      \
        i2d.radius = RAD;                                       \
        xgl_multicircle(CTX, &cl);                              \
    }

/* a circle using float 2D coordinates */
#define XGL_CIRCLE_F2D(CTX, CX, CY, RAD)                        \
    {                                                           \
        Xgl_circle_list cl;                                     \
        Xgl_circle_f2d  f2d[1];                                 \
        cl.num_circles = 1;                                     \
        cl.type = XGL_MULTICIRCLE_F2D;                          \
        cl.bbox = NULL;                                         \
        cl.circles.f2d = &f2d[0];                               \
        f2d.center.flag = 0;                                    \
        f2d.center.x = CX;                                      \
        f2d.center.y = CY;                                      \
        f2d.radius = RAD;                                       \
        xgl_multicircle(CTX, &cl);                              \
    }

/* a circle using fract 2D coordinates */
#define XGL_CIRCLE_B2D(CTX, CX, CY, RAD)                        \
    {                                                           \
        Xgl_circle_list cl;                                     \
        Xgl_circle_b2d  b2d[1];                                 \
        cl.num_circles = 1;                                     \
        cl.type = XGL_MULTICIRCLE_B2D;                          \
        cl.bbox = NULL;                                         \
        cl.circles.b2d = &b2d[0];                               \
        b2d.center.flag = 0;                                    \
        b2d.center.x = CX;                                      \
        b2d.center.y = CY;                                      \
        b2d.radius = RAD;                                       \
        xgl_multicircle(CTX, &cl);                              \
    }

/* a rectangle using integer 2D coordinates */
/* parameters coorrespond to XDrawRectangle/XFillRectangle */
#define XGL_RECTANGLE_I2D(CTX, X, Y, W, H)                      \
    {                                                           \
        Xgl_rect_list rl;                                       \
        Xgl_rect_i2d  i2d[1];                                   \
        rl.num_rects = 1;                                       \
        rl.rect_type = XGL_MULTIRECT_I2D;                       \
        rl.bbox = NULL;                                         \
        rl.rects.i2d = &i2d[0];                                 \
        i2d.corner_min.flag = 0;                                \
        i2d.corner_min.x = X;                                   \
        i2d.corner_min.y = Y;                                   \
        i2d.corner_max.x = X+W-1;                               \
        i2d.corner_max.y = Y+H-1;                               \
        xgl_multirectangle(CTX, &rl);                           \
    }

/* a rectangle using float 2D coordinates */
/* parameters coorrespond to XDrawRectangle/XFillRectangle */
#define XGL_RECTANGLE_F2D(CTX, X, Y, W, H)                      \
    {                                                           \
        Xgl_rect_list rl;                                       \
        Xgl_rect_f2d  f2d[1];                                   \
        rl.num_rects = 1;                                       \
        rl.rect_type = XGL_MULTIRECT_F2D;                       \
        rl.bbox = NULL;                                         \
        rl.rects.f2d = &f2d[0];                                 \
        f2d.corner_min.flag = 0;                                \
        f2d.corner_min.x = X;                                   \
        f2d.corner_min.y = Y;                                   \
        f2d.corner_max.x = X+W-1;                               \
        f2d.corner_max.y = Y+H-1;                               \
        xgl_multirectangle(CTX, &rl);                           \
    }

/* a rectangle using fract 2D coordinates */
/* parameters coorrespond to XDrawRectangle/XFillRectangle */
#define XGL_RECTANGLE_B2D(CTX, X, Y, W, H)                      \
    {                                                           \
        Xgl_rect_list rl;                                       \
        Xgl_rect_b2d  b2d[1];                                   \
        rl.num_rects = 1;                                       \
        rl.rect_type = XGL_MULTIRECT_B2D;                       \
        rl.bbox = NULL;                                         \
        rl.rects.b2d = &b2d[0];                                 \
        b2d.corner_min.flag = 0;                                \
        b2d.corner_min.x = X;                                   \
        b2d.corner_min.y = Y;                                   \
        b2d.corner_max.x = X+W-1;                               \
        b2d.corner_max.y = Y+H-1;                               \
        xgl_multirectangle(CTX, &rl);                           \
    }

/* a arc using integer 2D coordinates */
/* draw arc using parameters like XDrawArc and XFillArc */
#define XGL_ARC_I2D(CTX, CX, CY, W, H, ST, END)                 \
    {                                                           \
        Xgl_arc_list al;                                        \
        Xgl_arc_i2d  i2d[1];                                    \
        al.num_arcs = 1;                                        \
        al.type = XGL_MULTIARC_I2D;                             \
        al.bbox = NULL;                                         \
        al.arcs.i2d = &i2d[0];                                  \
        i2d.center.flag = 0;                                    \
        i2d.center.x = CX;                                      \
        i2d.center.y = CY;                                      \
        i2d.radius = W; /* don't do elliptical arcs */          \
        i2d.start_angle = ST;                                   \
        i2d.stop_angle = END;                                   \
        xgl_multiarc(CTX, &al);                                 \
    }

/* a arc using float 2D coordinates */
/* draw arc using parameters like XDrawArc and XFillArc */
#define XGL_ARC_F2D(CTX, CX, CY, W, H, ST, END)                 \
    {                                                           \
        Xgl_arc_list al;                                        \
        Xgl_arc_f2d  f2d[1];                                    \
        al.num_arcs = 1;                                        \
        al.type = XGL_MULTIARC_F2D;                             \
        al.bbox = NULL;                                         \
        al.arcs.f2d = &f2d[0];                                  \
        f2d.center.flag = 0;                                    \
        f2d.center.x = CX;                                      \
        f2d.center.y = CY;                                      \
        f2d.radius = W; /* don't do elliptical arcs */          \
        f2d.start_angle = ST;                                   \
        f2d.stop_angle = END;                                   \
        xgl_multiarc(CTX, &al);                                 \
    }

/* a arc using fract 2D coordinates */
/* draw arc using parameters like XDrawArc and XFillArc */
#define XGL_ARC_B2D(CTX, CX, CY, W, H, ST, END)                 \
    {                                                           \
        Xgl_arc_list al;                                        \
        Xgl_arc_b2d  b2d[1];                                    \
        al.num_arcs = 1;                                        \
        al.type = XGL_MULTIARC_B2D;                             \
        al.bbox = NULL;                                         \
        al.arcs.b2d = &b2d[0];                                  \
        b2d.center.flag = 0;                                    \
        b2d.center.x = CX;                                      \
        b2d.center.y = CY;                                      \
        b2d.radius = W; /* don't do elliptical arcs */          \
        b2d.start_angle = ST;                                   \
        b2d.stop_angle = END;                                   \
        xgl_multiarc(CTX, &al);                                 \
    }

/*
 * allocate a point list structure and use it to build a move-draw
 * type multi polyline
 */
#define XGL_START_POLYLINE_I2D(PT_LIST, NUMBER_OF_PTS)          \
    PT_LIST.pt_type = XGL_PT_I2D;                               \
    PT_LIST.bbox = NULL;                                        \
    PT_LIST.num_pts = NUMBER_OF_PTS;                            \
    PT_LIST.pts.i2d = (Xgl_pt_i2d *)malloc(sizeof(Xgl_pt_i2d) * \
        NUMBER_OF_PTS);

#define XGL_START_POLYLINE_F2D(PT_LIST, NUMBER_OF_PTS)          \
    PT_LIST.pt_type = XGL_PT_F2D;                               \
    PT_LIST.bbox = NULL;                                        \
    PT_LIST.num_pts = NUMBER_OF_PTS;                            \
    PT_LIST.pts.f2d = (Xgl_pt_f2d *)malloc(sizeof(Xgl_pt_f2d) * \
        NUMBER_OF_PTS);

#define XGL_START_POLYLINE_B2D(PT_LIST, NUMBER_OF_PTS)          \
    PT_LIST.pt_type = XGL_PT_B2D;                               \
    PT_LIST.bbox = NULL;                                        \
    PT_LIST.num_pts = NUMBER_OF_PTS;                            \
    PT_LIST.pts.b2d = (Xgl_pt_b2d *)malloc(sizeof(Xgl_pt_b2d) * \
        NUMBER_OF_PTS);

#define XGL_MOVE_POLYLINE_I2D(PT_LIST, X, Y)                    \
    PT_LIST.pts.i2d[0].x = X; PT_LIST.pts.i2d[0].y = Y;         \
    (PT_LIST.num_pts)++;

#define XGL_DRAW_POLYLINE_I2D(PT_LIST, X, Y)                    \
    {                                                           \
        Xgl_sgn32       num_pts = PT_LIST.num_pts;              \
        PT_LIST.pts.i2d[num_pts].x = X;                         \
        PT_LIST.pts.i2d[num_pts].y = Y;                         \
        (PT_LIST.num_pts)++;                                    \
    }

#define XGL_MOVE_POLYLINE_F2D(PT_LIST, X, Y)                    \
    PT_LIST.pts.f2d[0].x = X; PT_LIST.pts.f2d[0].y = Y;         \
    (PT_LIST.num_pts)++;

#define XGL_DRAW_POLYLINE_F2D(PT_LIST, X, Y)                    \
    {                                                           \
        Xgl_sgn32       num_pts = PT_LIST.num_pts;              \
        PT_LIST.pts.f2d[num_pts].x = X;                         \
        PT_LIST.pts.f2d[num_pts].y = Y;                         \
        (PT_LIST.num_pts)++;                                    \
    }

#define XGL_MOVE_POLYLINE_B2D(PT_LIST, X, Y)                    \
    PT_LIST.pts.b2d[0].x = X; PT_LIST.pts.b2d[0].y = Y;         \
    (PT_LIST.num_pts)++;

#define XGL_DRAW_POLYLINE_B2D(PT_LIST, X, Y)                    \
    {                                                           \
        Xgl_sgn32       num_pts = PT_LIST.num_pts;              \
        PT_LIST.pts.b2d[num_pts].x = X;                         \
        PT_LIST.pts.b2d[num_pts].y = Y;                         \
        (PT_LIST.num_pts)++;                                    \
    }

#define XGL_RENDER_POLYLINE(CTX, PT_LIST)                       \
    if (PT_LIST.num_pts >= 2)                                   \
        xgl_multipolyline(CTX, NULL, 1, &(PT_LIST));

#define XGL_FLUSH_POLYLINE(CTX, PT_LIST)                        \
    if (PT_LIST.num_pts >= 2) {                                 \
        if (PT_LIST.pts.i2d)                                    \
            free(PT_LIST.pts.i2d);                              \
        PT_LIST.pts.i2d = NULL;                                 \
        PT_LIST.num_pts = 0;                                    \
    }

/* set all context colors */
#define XGL_SET_COLOR_INDEX(CTX, C0)                    \
    {                                                   \
        Xgl_color       color;                          \
        color.index = C0;                               \
        xgl_object_set(CTX,                             \
            XGL_CTX_LINE_COLOR, &color,                 \
            XGL_CTX_SURF_FRONT_COLOR, &color,           \
            XGL_CTX_EDGE_COLOR, &color,                 \
            XGL_CTX_EDGE_ALT_COLOR, &color,             \
            XGL_CTX_MARKER_COLOR, &color,               \
            XGL_CTX_SFONT_TEXT_COLOR, &color,           \
            0);                                         \
    }

/* set all context colors */
#define XGL_SET_COLOR_RGB(CTX, R, G, B)                 \
    {                                                   \
        Xgl_color       color;                          \
        color.rgb.r = R;                                \
        color.rgb.g = G;                                \
        color.rgb.b = B;                                \
        xgl_object_set(CTX,                             \
            XGL_CTX_LINE_COLOR, &color,                 \
            XGL_CTX_SURF_FRONT_COLOR, &color,           \
            XGL_CTX_EDGE_COLOR, &color,                 \
            XGL_CTX_EDGE_ALT_COLOR, &color,             \
            XGL_CTX_MARKER_COLOR, &color,               \
            XGL_CTX_SFONT_TEXT_COLOR, &color,           \
            0);                                         \
    }

/* set all surface fill styles */
#define XGL_SET_FILL_STYLE(CTX, FST)                    \
    xgl_object_set(CTX,                                 \
        XGL_CTX_SURF_FRONT_FILL_STYLE, FST,             \
        0);

/* used in xglut.c */
#define XGLUT_GEN_PGON                  0
#define XGLUT_MSMP_PGON                 1
#define XGLUT_TRI_STRIP                 2
#define XGLUT_QUAD_MESH                 3

/* global variables used everywhere and defined in xglut.c */
extern Xgl_color_type                  xglut_color_type;
extern Xgl_color_type                  xglut_hw_color_type;
extern Xgl_sgn32                       xglut_depth;
extern Xgl_sgn32                       xglut_visual_class;
extern Xgl_boolean                     xglut_hw_zbuffer;
extern Xgl_boolean                     xglut_hw_shading;

/*
 * Stroke Text related functions and variables
 */
typedef struct {
  char          *str;
  Xgl_pt_f3d    pos;
  Xgl_pt_f3d    dir[2];
} xglut_text_info;

/*
 * used in xglut_bsp.c
 * Binary Space Partition data structures and function prototypes
 */

/* **** BSP types **** */
/* bounding sphere */
typedef struct {
    float   x, y, z, r;
} BSphere;

/* plane equation */
typedef struct{
    float   a, b, c, d;
} Plane;

/* node structure for the BSP tree */
typedef struct bsp_node {
    BSphere     bsphere;        /* approximate bounding sphere of polygon */
    Plane       plane;          /* plane equation of polygon */
    Xgl_sgn32   num_pts;        /* # of points in following array */
    Xgl_pt_f3d  *points;        /* ptr to first of array of pts */
    struct bsp_node *next;      /* for lists of bsp_nodes */
    struct bsp_node *front;     /* for BSP tree */
    struct bsp_node *back;      /*      "       */
    Xgl_color   color;          /* color of polygon */
    Xgl_boolean freeme;         /* TRUE if this node is freeable */
} bsp_node;

/* a structure to hold all the key information about an object */
typedef struct {
    Xgl_pt_list         *pgpl;  /* polygon point list structure */
    Xgl_sgn32           pgpl_count;     /* count of point lists */
    Xgl_facet_flags     flags;
    Xgl_facet_list      *facets;
    Xgl_trans           GLOBAL_TRANS;
    Xgl_trans           local_trans;
    bsp_node            *root;
    float               xmin, xmax, ymin, ymax, zmin, zmax; /* bbox for obj */
    Xgl_boolean         type_is;
} bsp_object;

void            xglut_init_object();
void            xglut_init_object_position();
void            xglut_display_object();
void            xglut_paint_object_geom();

/* **** Prototypes for BSP code **** */
bsp_node        *bsp_build_list();
void            bsp_free_tree();
void            bsp_free_object();
void            bsp_traverse_tree();
void            bsp_build_tree();
bsp_node        *bsp_build_nodes();
Xgl_sgn32       bsp_quad_in_place();
Xgl_sgn32       bsp_convert_to_quad();
Xgl_sgn32       bsp_tri_in_place();
Xgl_sgn32       bsp_convert_to_tri();
Xgl_sgn32       bsp_split();
Xgl_sgn32       bsp_calc();
void            bsp_translate_object();
/*
 * used in xglut_dbuf.c
 * Double buffering data structures
 */

/*
 * various double buffering defines and data structures
 * description of fields:
 *   NOTE -> USER SUPPLIED FIELDS:
 *      ctx = USER supplied Xgl graphics context
 *      number_of_colors_per_buffer = USER supplied color count in application
 *      color_table = USER supplied colors for each buffer
 *   NOTE -> COMPUTED FIELDS:
 *      cmap_dbuffering = TRUE if we are doing color map double buffering
 *      xgl_dbuffering = TRUE if we are doing Xgl double buffering in HW
 *      current_buffer_is_buffer_0 = TRUE if buffer 0 is displayed and FALSE
 *              if buffer 1 is displayed
 *      buf0_pm = plane mask for buffer 0 when using color map double buffering
 *      buf1_pm = plane mask for buffer 1 when using color map double buffering
 *      cmap0 = color map for buffer 0 when using color map double buffering
 *      cmap1 = color map for buffer 1 when using color map double buffering
 *      buffers_requested = double buffering buffers requested (usually 2)
 *      buffers_allocated = 2 if we have Xgl double buffering and 1 if we don't
 *      buf_draw = buffer graphics will be drawn into
 *      buf_display = buffer displayed
 *      buf_min_dealy = time in msec to wait for buffer switch synchronization:
 *              NOTE: this has no affect when doing color map double buffering
 *                    and its value will always be zero
 */
typedef struct {
    /* USER SUPPLIED FIELDS */
    Xgl_ctx             ctx;
    Xgl_sgn32           number_of_colors_per_buffer;
    Xgl_color           *color_table;
    /* COMPUTED FIELDS */
    Xgl_boolean         cmap_dbuffering;
    Xgl_boolean         xgl_dbuffering;
    Xgl_boolean         current_buffer_is_buffer_0;
    Xgl_sgn32           bits_per_buffer;
    Xgl_sgn32           buf0_pm, buf1_pm;
    Xgl_cmap            cmap0, cmap1;
    Xgl_sgn32           buffers_requested;
    Xgl_sgn32           buffers_allocated;
    Xgl_sgn32           buf_draw;
    Xgl_sgn32           buf_display;
    Xgl_sgn32           buf_min_delay;
} Xglut_dbuf_info;

#define XGLUT_DBUF_IDX_PTR(DBufInfo, CLrIDx, IDx)               \
    if ((DBufInfo)->cmap_dbuffering == TRUE) {                  \
        (CLrIDx) = ((IDx << DBufInfo->bits_per_buffer) | IDx);  \
    }                                                           \
    else {                                                      \
        (CLrIDx) = IDx;                                         \
    }

#define XGLUT_DBUF_IDX(DBufInfo, CLrIDx, IDx)                   \
    if ((DBufInfo).cmap_dbuffering == TRUE) {                   \
        (CLrIDx) = ((IDx << DBufInfo.bits_per_buffer) | IDx);   \
    }                                                           \
    else {                                                      \
        (CLrIDx) = IDx;                                         \
    }

/*
 * used in xglut_quat.c
 * quaternion data structures and defines
 */

#define EPSILON .0001

typedef struct {
  float         pos[3];         /* The [x,y,z] of the 3D eye position, in WC */
  float         rop[4];         /* The quaternion associated to the eye in WC */
} P3_internxgl_quat;


typedef struct {
  float         ori[4][4];      /* The orientation matrix for visualization */
} P3_visualize_sys;


typedef struct {
  float         g;              /* The type of lens:                    */
                                /*      g != 0: real perspective,       */
                                /*              x & y used for eye devi */
                                /*              ation.                  */
                                /*      g = 0:  linear projection,      */
                                /*              x & y define the projec */
                                /*              tion vector.            */
  float         x;              /* The x lens value */
  float         y;              /* The y lens value */
} P3_internxgl_lens;


typedef struct {
  float         coords[4];      /* The [w,x,y,z] coordsinates of a 3D point */
} P3_internxgl_point;


typedef struct {
  P3_internxgl_quat     O;              /* The eye's position & direction */
  P3_visualize_sys      S;              /* The window scale factors */
  P3_internxgl_lens     L;              /* The lens for perspective */
  float         M_quat[4][4];           /* The eye's resulting matrix */
  float         M_lens[4][4];           /* The optical resulting matrix */
  float         M_all[4][4];            /* The total transformation matrix */
  unsigned short        M_of_0;         /* The optimization flags toward 0 */
  unsigned short        M_of_1;         /* The optimization flags toward 1 */
} P3_internxgl_ctx, *P_ctx;

/*
 * function prototypes which are in xglut_quat.c
 */
P_ctx           P_mk_ctx();
void            P3_set_error();
void            P_set_pos();
void            P_set_lens();
void            P_set_orient();
void            P_look_to();
void            P3_tra_mov();
void            P3_rot_mov();
void            P_3D_move();
void            P3_quat_mat();
void            P3_lens_mat();
void            P3_u_4x4();
void            P3_matrix();

/*
 * from xglut_dbuf.c
 * xglut_dbuf_init - initialize double buffering.
 * xglut_dbuf_switch_buffer - switch displayed and draw buffers
 * xglut_dbuf_set_draw_buffer - set buffer for rendering
 * xglut_dbuf_set_display_buffer - set display buffer
 */
void    xglut_dbuf_init( /* dbuf_information */ );
void    xglut_dbuf_switch_buffer( /* dbuf_information */ );
void    xglut_dbuf_set_draw_buffer( /* dbuf_information */ );
void    xglut_dbuf_set_display_buffer( /* dbuf_information */ );

/* from xglut_xv.c */
Xgl_win_ras     xglut_create_window_raster_from_xv_canvas();
void            xglut_get_x_window_from_xv_canvas();

/* from xglut_olit.c */
Xgl_win_ras     xglut_create_window_raster_from_widget();
void            xglut_get_x_window_from_widget();

/* from xglut_xlib.c */
Window          xglut_xlib_window_create();
Visual          *xglut_xlib_getpseudocolorvisual();
Visual          *xglut_xlib_getdirectcolorvisual();
Visual          *xglut_xlib_gettruecolorvisual();
Visual          *xglut_xlib_getvisual();
void            xglut_xlib_mapwindow();
void            xglut_xlib_wait_for_left_mouse_button();
Colormap        xglut_xlib_color_map_create();
void            xglut_get_xlib_gc_from_xgl_ctx();
void            xglut_get_xgl_ctx_from_xlib_gc();

/* from xglu_view_3d_set.c */
void            xglut_set_perspective();
void            xglut_set_eye_position();
int             xglut_look_at_point();
int             xglut_set_3d_view();

/* from xglut.c */
void            xglut_argprocess();

/* from xglut_clr.c */
Xgl_cmap        xglut_simple_color_map_8();
void            xglut_simple_color_table_8();
void            xglut_simple_set_color();
void            xglut_simple_set_gray_color();
void            xglut_make_ramp();
void            xglut_make_ramp();
Xglut_dbuf_info *xglut_ramp_color_map_128();
void            xglut_ramp_context_color_init();
void            xglut_ramp_context_ramp_colors();
void            xglut_ramp_set_color();
void            xglut_simple_context_color_init();

/* from xglut_pnl.c */
void            xglut_color_panel_init();
void            xglut_line_color_update();
void            xglut_line_alt_color_update();
void            xglut_edge_color_update();
void            xglut_edge_alt_color_update();
void            xglut_surf_front_color_update();
void            xglut_surf_back_color_update();
void            xglut_marker_color_update();
void            xglut_stext_color_update();
void            xglut_depth_cue_color_update();
void            xglut_background_color_update();
void            xglut_ras_stipple_color_update();
void            xglut_front_specular_color_color_update();
void            xglut_back_specular_color_color_update();

/* defines for ramp making */
#define XGLUT_RED_RAMP          1
#define XGLUT_GREEN_RAMP        2
#define XGLUT_BLUE_RAMP         4

/* index color values for the simple color map */
#define XGLUT_BLACK             0
#define XGLUT_RED               1
#define XGLUT_GREEN             2
#define XGLUT_BLUE              3
#define XGLUT_YELLOW            4
#define XGLUT_MAGENTA           5
#define XGLUT_CYAN              6
#define XGLUT_WHITE             7
#define XGLUT_DARK_GRAY         8
#define XGLUT_MIDDLE_GRAY       9
#define XGLUT_LIGHT_GRAY        10
#define XGLUT_COLOR_OTHER       11      /* used for RGB entered values */

/* index color values for the ramp color map */
#define XGLUT_RAMP_BLACK        0
#define XGLUT_RAMP_RED          1
#define XGLUT_RAMP_GREEN        2
#define XGLUT_RAMP_WHITE        3
#define XGLUT_RAMP_COLOR_OTHER  4

/* these are indexes that are the top of the ramps created by the xglut
   ramp creation function */
#define XGLUT_RAMP_GREEN_TOP    31
#define XGLUT_RAMP_RED_TOP      63
#define XGLUT_RAMP_GRAY_TOP     126

#define XGLUT_SET_COLOR(ITEM, COLOR, TABLE)     \
    if (xglut_color_type == XGL_COLOR_INDEX) {  \
        ITEM.index = COLOR;                     \
    }                                           \
    else {                                      \
        ITEM.rgb = TABLE[COLOR];                \
    }

#define XGLUT_SIMPLE_COLOR_COUNT        8
#define XGLUT_SIMPLE_COLOR_CHOICE "black", "red", "green", "blue", "yellow",\
        "magenta", "cyan", "white", "dark gray", "middle gray", "light gray", 0
#define XGLUT_RAMP_COLOR_CHOICE "black", "red", "green", "white", "other", 0

/* helpful macros for filling Xgl data structures */
#define XGLUT_SETCMAP_INFO(arg, arg_start, arg_length, arg_info) do { \
                (arg).start_index = (arg_start);        \
                (arg).length = (arg_length);            \
                (arg).colors = (arg_info);              \
         } while (0)

#define XGLUT_SETCOLOR_RGB(arg, arg_r, arg_g, arg_b) do {  \
                (arg).r = (arg_r);                      \
                (arg).g = (arg_g);                      \
                (arg).b = (arg_b);                      \
        } while (0)

#define XGLUT_SET_PT_LIST(arg, argtype, argbb, argcnt, PTS_TYPE, argpts) do { \
                (arg).pt_type = (argtype);                              \
                (arg).bbox = (argbb);                                   \
                (arg).num_pts = (argcnt);                               \
                (arg).pts.PTS_TYPE = (argpts);                          \
        } while (0)

/* some common colors that are defined in xglut.c */
extern  Xgl_color_rgb   xglut_black, xglut_red, xglut_green;
extern  Xgl_color_rgb   xglut_blue, xglut_magenta, xglut_yellow;
extern  Xgl_color_rgb   xglut_cyan, xglut_white;
extern  Xgl_color_rgb   xglut_dark_gray, xglut_middle_gray;
extern  Xgl_color_rgb   xglut_light_gray;
extern  Xgl_color_rgb   xglut_other;

/* Xgl colors for simple color map; these are setup in
   xglut_simple_color_table_8 */
extern  Xgl_color       xglut_black_color, xglut_red_color, xglut_green_color;
extern  Xgl_color       xglut_blue_color, xglut_magenta_color;
extern  Xgl_color       xglut_yellow_color;
extern  Xgl_color       xglut_cyan_color, xglut_white_color;
extern  Xgl_color       xglut_dark_gray_color, xglut_middle_gray_color;
extern  Xgl_color       xglut_light_gray_color;
extern  Xgl_color       xglut_other_color;

/* common 1-bit deep stipple fills for STIPPLE polygons */
extern unsigned char xglut_stpl_dots[], xglut_stpl_cross_hatch[];
extern unsigned char xglut_stpl_screen[];

/* other macros used alot in Xgl test programs */
#define XGLUT_ITIMER_NULL       ((struct itimerval *)0)
#define XGLUT_START_TIMER(x)  (void)gettimeofday(x, (struct timezone *) 0)
#define XGLUT_STOP_TIMER(x)   (void)gettimeofday(x, (struct timezone *) 0)
#define XGLUT_ELAPSED_SECONDS(x,y) ( ((y)->tv_sec - (x)->tv_sec) + \
                   ((y)->tv_usec - (x)->tv_usec)*1e-6)

/* defined in xglut.c */
extern short    xglut_icon_data[];
extern Pixrect  xglut_icon_image;

/* defined and used in xglut_pnl.c */
extern  Panel           xglut_time_display_panel;
extern  Frame           xglut_time_display_window;
extern  Panel_item      xglut_time_display_item, xglut_per_sec_item;
extern  Panel_item      xglut_count_item, xglut_time_item;
extern  Xgl_boolean     xglut_time_display_up;

void    xglut_time_display_panel_create();
void    xglut_show_xyz_create();
void    xglut_show_xyz_destroy();
void    xglut_show_xyz();

/* old test_utils.h defines which will be replaced with functions in
   the distance future */

#define TEST_COLOR_STRING_C "Color: "
#define TEST_COLOR_STRING_T "Other: "
#define TEST_COLOR_COL_OFFSET  19
#define TEST_COLOR_DISP_LENGTH 15

#define TEST_COLOR_UPDATE(ctx, pc_attr, pc_other, pc_other_used, pc_item_c, pc_color_other_update) \
  xgl_object_get(ctx, pc_attr, &pc_other); \
  pc_other_used = TRUE; \
  xv_set(pc_item_c, PANEL_VALUE, 4,0,0); \
  pc_color_other_update();

#define TEST_COLOR_PROC(ctx, pc_proc, pc_other, pc_other_used, pc_attr) \
void \
pc_proc(item, value, event) \
Panel_item      item;   \
int             value;  \
Event           *event; \
{  \
    Xgl_color   temp_color; \
    xglut_ramp_dbuf_color_assign(value, &temp_color, \
        &pc_other, &pc_other_used); \
    xgl_object_set(ctx, pc_attr, &temp_color, 0); \
}

#define TEST_SIMPLE_COLOR_PROC(ctx, pc_proc, pc_other, pc_other_used, pc_attr) \
void \
pc_proc(item, value, event) \
Panel_item      item;   \
int             value;  \
Event           *event; \
{  \
    Xgl_color   temp_color; \
    xglut_simple_color_assign(value, &temp_color, \
        &pc_other, &pc_other_used); \
    xgl_object_set(ctx, pc_attr, &temp_color, 0); \
}

#define TEST_COLOR_OTHER_UPDATE(pc_other_update, pc_item_t, pc_other) \
void \
pc_other_update() \
{ \
    char temp_str[50]; \
    if (xglut_color_type == XGL_COLOR_INDEX) { \
        sscanf(xv_get(pc_item_t, PANEL_VALUE),"%d", &(pc_other.index)); \
        sprintf(temp_str, "%d", pc_other.index); \
    } \
    else { \
        sscanf(xv_get(pc_item_t, PANEL_VALUE),"%f %f %f", \
            &(pc_other.rgb.r), &(pc_other.rgb.g), &(pc_other.rgb.b)); \
        sprintf(temp_str, "%4.1f,%4.1f,%4.1f", \
            pc_other.rgb.r, pc_other.rgb.g, pc_other.rgb.b); \
    } \
    xv_set(pc_item_t, PANEL_VALUE, temp_str,0,0); \
}

#define TEST_COLOR_OTHER_PROC(ctx, pc_other_proc, pc_item_t, pc_other, pc_other_used, pc_other_update, pc_attr) \
void \
pc_other_proc(pattern_item) \
    Panel_item  pattern_item; \
{ \
    xglut_color_read(pc_item_t, &pc_other); \
    pc_other_update(); \
    if (pc_other_used) { \
        xgl_object_set(ctx, pc_attr, &pc_other, 0); \
    }  \
}

