/***********************************************************************
*                              phantom.h                               *
*                                                                      *
*               Data structures used by phantom program                *
*                                                                      *
*  V1A, 01/23/95, GNR - Initial version                                *
***********************************************************************/

#define MAX_NOTE_CHARS  255      /* Size of largest allowed note */
#define DFLT_NSEED     1009      /* Default noise seed */

#define NORMAL            0      /* Normal subroutine return */
#define ERROR             1      /* Subroutine error return */
#define BIG          1.0E20      /* A very big number */
#define TINY         1.0E-4      /* Shortest meaningful vector */
#define X                 0      /* Index of x comp. of a vector */
#define Y                 1      /* Index of y comp. of a vector */
#define Z                 2      /* Index of z comp. of a vector */

/* Function prototypes */
void fatal(char *txt, int num);
int phrdcc(void);
int phckin(void);
int phwimg(int fd);

/* Structure to contain a lattice and its objects */
typedef struct lat {
   struct lat *pnlat;            /* Ptr to next lattice */
   struct obj *pfobj;            /* Ptr to first object */
   float Lv[3][3];               /* Up to three lattice vectors */
   float Llv[3];                 /* Lengths of the Lv vectors */
   float Lw2[3],Lw3[3];          /* Reciprocal lattice vectors */
   float Lw2w2,Lw3w3;            /* Lw2 dot Lw2 and Lw3 dot Lw3 */
   float Llw2;                   /* Length of Lw2 = sqrt(Lw2w2) */
   int nLv;                      /* Number of lattice vectors */
   } Lattice;

/* Structure to define a parallelepiped brick */
typedef struct bri {
   int rho;                      /* Pixel density */
   float Be[3][3];               /* Three edge vectors */
   float Bw[3][3];               /* Reciprocal face vectors */
   } Brick;

/* Structure to define a cylinder */
typedef struct cyl {
   int rho;                      /* Pixel density */
   float Ca[3];                  /* Cylinder axis vector */
   float Cn[3];                  /* Normalized axis vector */
   float Cr;                     /* Radius of cylinder */
   float haxis;                  /* Length of hemiaxis */
   } Cylinder;

/* Structure to define a sphere */
typedef struct sph {
   int rho;                      /* Pixel density */
   float Sd;                     /* Diameter of sphere */
   float Sr;                     /* Radius of sphere */
   } Sphere;

/* Structure to define a general object (one of the above) */
typedef struct obj {
   struct obj *pnobj;            /* Ptr to next object */
   float O[3];                   /* Origin ("handle" position) */
   int kobj;                     /* Kind of object */
/* Values kobj can take on: */
#define BRICK     1                 /* Brick */
#define CYLINDER  2                 /* Cylinder */
#define SPHERE    3

   union objinst {               /* Instance of object */
      Brick b;                      /* Brick */
      Cylinder c;                   /* Cylinder */
      Sphere s;                     /* Sphere */
      } o;

   } Object;

/* Structure to contain global phantom program parameters */
struct phglob {
   double Vox[3];                /* Size of a pixel (mm) */
   double Face[3];               /* Vectors defining image box */
   float sigma;                  /* Mean,sigma of noise dist */
   Lattice *pflat;               /* Ptr to first lattice */
   char *fnm;                    /* Name of NEMA output file */
   char *inst;                   /* Institution */
   char *dept;                   /* Department */
   char *note;                   /* Note or comment */
   long nseed;                   /* Random number seed for noise */
   long study;                   /* Name of study */
   long series;                  /* Name of series */
   long cols,rows,secs;          /* Size of image */
   int hiden;                    /* Highest density we can write */
   } ;

