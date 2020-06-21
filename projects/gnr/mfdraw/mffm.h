/* (c) Copyright 2008, The Rockefeller University *11114* */
/***********************************************************************
*                               mffm.h                                 *
*                      Frame Manager for mfdraw                        *
*                                                                      *
*  mffm is a package of routines that an NSI metafile drawing program  *
*  (mfdraw) can use to manage loading of and access to drawing frames. *
*  This header file provides the definitions and prototypes needed to  *
*  use mffm.  The functions and their documentation are in file mffm.c *
*                                                                      *
*  N.B.  Throughout the mffm package, the term "read-locked" means     *
*  a frame is being accessed for reading and cannot be written to or   *
*  discarded.  The term "write-locked" means the frame is being loaded *
*  and cannot be read from or discarded.  This is the opposite of how  *
*  these terms might be used in other contexts.                        *
************************************************************************
*  V1A, 05/24/08, G.N. Reeke, new program.                             *
*  V1B, 07/12/08, GNR, Add get_hdr[12], put_hdr[12], append calls      *
***********************************************************************/

#include <unistd.h>

/* Type definitions */
typedef long FrameNum;           /* Frame number */
typedef unsigned char byte;

/* Number of statistics returned by mffm_cleanup */
#define MFFM_NSTATS        6

/* Function prototypes */
#ifdef __cplusplus
extern "C" {
#endif
int mffm_init(size_t MaxMem, size_t LUHdr1, size_t LUHdr2,
      long HistSize);
int mffm_create(FrameNum frame);
int mffm_reload(FrameNum frame);
int mffm_put_hdr1(FrameNum frame, void *puhd);
int mffm_put_hdr2(FrameNum frame, void *puhd);
int mffm_store(FrameNum frame, byte *pdat, long ldat);
int mffm_append(FrameNum frame, byte *pdat, long ldat);
int mffm_end_put(FrameNum frame);
int mffm_access(FrameNum frame, FrameNum *pafn);
int mffm_get_hdr1(FrameNum frame, void *puhd);
int mffm_get_hdr2(FrameNum frame, void *puhd);
int mffm_get_data(FrameNum frame, byte **ppd, long *pld);
int mffm_end_get(FrameNum frame);
int mffm_expand(size_t newmem);
int mffm_cleanup(long stats[MFFM_NSTATS]);
#ifdef __cplusplus
   }
#endif

/* Normal return codes */
#define MFFM_OK      0           /* Normal return from all routines */
#define MFFM_EXISTS  1           /* Frame already entered, it cannot
                                 *  be created (or reloaded) now */
#define MFFM_NOHIST  1           /* View history reached oldest or
                                 *  newest entry, cannot proceed */
#define MFFM_WLOCK   2           /* Frame is write-locked, it cannot
                                 *  be accessed for reading/updating */
#define MFFM_RLOCK   3           /* Frame is read-locked, it cannot
                                 *  be accessed for reloading */
#define MFFM_NODATA  4           /* The frame exists, but has no
                                 *  data stored with it */
#define MFFM_NOFRAME 5           /* Attempt to access a frame that
                                 *  has not been created */

/* Error codes.  See discussion in mffm.c */
#define MFFM_ERR_NOMEM     170   /* Unable to allocate the amount of
                                 *  memory requested to mffm_init */
#define MFFM_ERR_APPEND    171   /* Attempt to append data to a frame
                                 *  before an mffm_store() call */
#define MFFM_ERR_SEMOP     172   /* An error was returned by a
                                 *  semaphore operation */
#define MFFM_ERR_BADFRAME  173   /* Attempt to load data to or from a
                                 *  frame that has not been created */
#define MFFM_ERR_OFLOCK    174   /* Unable to release frames because
                                 *  oldest frame is locked */
#define MFFM_ERR_NOWLOCK   175   /* The frame specified for storing
                                 *  data is not write-locked */
#define MFFM_ERR_NORLOCK   176   /* The frame specified for retrieving
                                 *  data is not read-locked */
#define MFFM_ERR_BADARG    177   /* An pointer argument was NULL or a
                                 *  count or frame argument was <= 0 */

/* Statistics returned by mffm_cleanup.
*  There must be MFFM_NSTATS of these:  */
#define MFSTAT_FIXMEM      0
#define MFSTAT_BLKMEM      1
#define MFSTAT_HDRMEM      2
#define MFSTAT_DATMEM      3
#define MFSTAT_HDRWASTE    4
#define MFSTAT_DATWASTE    5


