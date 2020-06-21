/***********************************************************************
*                               bhost.h                                *
*                                                                      *
*  Host-server communication parameters for TI MVP implementation of   *
*  d3bench benchmark program.  Include in client and server programs.  *
*                                                                      *
*  Add, 11/22/95, GNR - New file, some pieces from GIC.h               *
***********************************************************************/

#ifndef _BHOST_H_
#define _BHOST_H_

/* Common definitions */
#define HMsgSize       128    /* Size of host message buffer */

/* A structure to hold some communication info */
typedef struct comm_t {
	long size;              /* Allocation size in bytes */
	long type;              /* Type of data sent */
	long value1;		      /* A generic value to pass */
	long value2;		      /* A generic value to pass */
	void *addr;			      /* Requested/response address */
} COMM_s;

/* And some opcodes */
typedef enum {
	CLIENT_INIT = 0,	      /* Initialization message from Client */
	SERVER_INIT,		      /* Initialization message from Server */
   SERVER_MSG,             /* Server has message for stdout */
   CLIENT_DONE_MSG,        /* Client finished last stdout message */
   ALL_DONE_MSG            /* Calculation on MVP is complete */
} COMM_e;

#endif	/* _BHOST_H_ */
