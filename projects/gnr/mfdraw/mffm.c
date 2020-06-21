/* (c) Copyright 2008, The Rockefeller University *11114* */
/***********************************************************************
*                               mffm.c                                 *
*                      Frame Manager for mfdraw                        *
*                                                                      *
*  mffm is a package of routines that an NSI metafile drawing program  *
*  (mfdraw) can use to manage loading of and access to drawing frames. *
*  This version requires an environment that has POSIX semaphores.     *
*                                                                      *
*  General considerations:                                             *
*  ------- --------------                                              *
*  This package provides (1) a mechanism to store drawing frames in    *
*  RAM memory, including both user-specified permanent ("header")      *
*  information and other information ("drawing commands") that may be  *
*  discarded when necessary, (2) a mechanism to specify the maximum    *
*  amount of RAM that may be used by stored frames and to release      *
*  frames in least-recently-accessed order when that memory is full,   *
*  (3) a mechanism to provide exclusive access to frames for loading   *
*  or viewing, (4) a frame index that allows access to stored frames   *
*  by frame number, (5) a view history that allows a caller to navi-   *
*  gate through stored frames in the order previously viewed.          *
*                                                                      *
*  (Note that the view history is not the same as the access history.  *
*  The view history refers to the order in which frames were viewed    *
*  by the user and is a circular buffer that can contain the same      *
*  frame in more than one position, but with a fixed total length.     *
*  The access history is used to keep track of frames in order of      *
*  eligibility for deletion.  It is a double-linked list kept in the   *
*  frame headers.  Each frame can appear no more than once.)           *
*                                                                      *
*  It is assumed that loading and viewing of frames may be carried     *
*  out by distinct threads and protection is provided against viewing  *
*  a frame that is being loaded or releasing a frame that is being     *
*  viewed.  A key assumption is that only one frame at a time may be   *
*  accessed for loading and one for viewing, consistent with the       *
*  expected mode of operation of mfdraw.  No routine is provided to    *
*  inquire the status of a frame.  Instead, a return code is gene-     *
*  rated when a requested action cannot be carried out.  This pro-     *
*  tects against the status changing between a status inquiry and a    *
*  following action request due to action in the other thread.  The    *
*  mffm_init() routine should be called before spawning threads so all *
*  threads will share the same global mffm data.  The current version  *
*  will not work if loading and viewing are carried out in separate    *
*  processes that do not share the same memory space.                  *
*                                                                      *
*  When a frame is discarded to make room for a newer frame, and a     *
*  later request to access that frame for reading fails with return    *
*  code MFFM_NODATA, it is the responsibility of the calling program   *
*  to determine whether or not it is possible to reload the frame      *
*  (i.e. was the source a transient socket connection or a permanent   *
*  file), and, if the frame is available, to reload it via the         *
*  mffm_reload() sequence.                                             *
*                                                                      *
*  This package makes no assumptions about the contents of the user-   *
*  supplied frame headers or drawing commands.  In order to allow for  *
*  arbitrary-sized bitmaps and polylines, drawing commands may be any  *
*  length that fits in memory.  However, the user frame header data    *
*  must fit in contiguous memory blocks no larger than MfdBlockSize    *
*  minus the size of about 8 pointers (see MfdBlockDef and FrameHdrDef *
*  structures for exact overhead), but the smaller the better, because *
*  these data are retained throughout a run (unless mffm is changed to *
*  allow header release).  Provision is made to store and retrieve     *
*  header information of two exclusive classes, the idea being that    *
*  the viewing (GUI) component of the client can use one category      *
*  (e.g. frame location, zoom factor, etc.) and the frame acquisition  *
*  component can use the other (e.g. file location of a frame, etc.)   *
*                                                                      *
*  A header file mffm.h is provided which contains prototypes for all  *
*  the "public" routines in the package, definitions for all return    *
*  codes, and a typedef for a FrameNum type that should be used for    *
*  all references to frame numbers.  All routines are written in C,    *
*  but the mffm.h header provides interfaces definitions for C++       *
*  callers.  The code should work correctly if compiled for a 32-bit   *
*  or a 64-bit environment, provided only that the type 'long' is a    *
*  32-bit integer in the first case and a 64-bit integer in the        *
*  second case.  If this is not true, the GNR header "sysdef.h" can    *
*  be included to provide suitable typedefs.                           *
*                                                                      *
*  Every routine in the package returns a status code that is defined  *
*  in the mffm.h header file.  Any other information returned by the   *
*  call is stored via pointers in the argument list for that routine.  *
*  The return code is always MFFM_OK (0) for successful operations     *
*  and a defined small integer for recoverable errors.  Return codes   *
*  for fatal errors are defined in the range 170-179, which is         *
*  allocated to this program in the laboratory abexit error system.    *
*  It is left to the caller what to do with these errors, possibly     *
*  writing a message to the log or to the client application via a     *
*  socket.                                                             *
*                                                                      *
*  N.B.  Throughout the mffm package, the term "read-locked" means     *
*  a frame is being accessed for reading and cannot be written to or   *
*  discarded.  The term "write-locked" means the frame is being loaded *
*  and cannot be read from or discarded.  This is the opposite of how  *
*  these terms might be used in other contexts.                        *
*                                                                      *
*----------------------------------------------------------------------*
*                              mffm_init                               *
*                                                                      *
*  This routine initializes the mffm package.  It must be called       *
*  before any of the other routines and before any threads are         *
*  spawned that might call any of the other routines.  (mffm_init()    *
*  should only be called once, from the parent process.)  All the      *
*  other routines assume this without checking (but will quickly get   *
*  a segmentation error if mffm_init() was not called).                *
*                                                                      *
*  Syntax:  int mffm_init(size_t MaxMem, size_t LUHdr1, size_t LUHdr2, *
*     long HistSize)                                                   *
*                                                                      *
*  Arguments:                                                          *
*     MaxMem      Maximum amount of memory (in bytes) to be devoted    *
*                 to storage of frame headers and drawing commands.    *
*                 (Does not include main index and viewing history.)   *
*     LUHdr1      Length of user header data of first type that will   *
*                 be stored with each frame (bytes).  Must fit in one  *
*                 memory block but should be as small as possible.     *
*     LUHdr2      Length of user header data of second type.           *
*     HistSize    Number of frames to retain in viewing history.       *
*                                                                      *
*  Return Values:                                                      *
*     MFFM_OK           Initialization successful.                     *
*     MFFM_ERR_NOMEM    Not enough memory for initial index, etc.      *
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*                       Here and elsewhere, the system-provided        *
*                       error number in 'errno' should be valid.       *
*----------------------------------------------------------------------*
*                             mffm_create                              *
*                                                                      *
*  This routine checks whether a given frame already exists in the     *
*  data structure.  If it does not, space to store the frame is        *
*  created and the frame is locked for data storage until unlocked     *
*  by a call to mffm_end_put().  The frame memory cannot be released   *
*  or accessed for viewing until the mffm_end_put() call occurs.       *
*                                                                      *
*  Syntax:  int mffm_create(FrameNum frame)                            *
*                                                                      *
*  Arguments:                                                          *
*     frame       Number of the frame to be created (requires          *
*                 0 < frame number <= largest positive long integer).  *
*                 Frame numbers do not need to be in sequential order, *
*                 however, if a frame is created beyond the current    *
*                 highest numbered frame in the data structure, space  *
*                 will be allocated in the index for all intervening   *
*                 frames.                                              *
*                                                                      *
*  Return Values:                                                      *
*     MFFM_OK           Empty frame was created and added to index.    *
*     MFFM_EXISTS       This frame already exists.  No action taken.   *
*     MFFM_ERR_NOMEM    Not enough memory for header or expanded index.*
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*     MFFM_ERR_BADARG   Attempt to create a frame with number <= 0.    *
*     MFFM_ERR_OFLOCK   An attempt was made to release the least-      *
*                       recently accessed frame to make space for the  *
*                       new frame header, but that frame was locked    *
*                       for reading.  The entire available memory is   *
*                       taken up with frame headers and/or the frame   *
*                       being viewed.  Either allocate more memory or  *
*                       rewrite this package to release frame headers  *
*                       when needed.                                   *
*----------------------------------------------------------------------*
*                             mffm_reload                              *
*                                                                      *
*  This routine is a variant of mffm_create() that should be called    *
*  when a frame has previously been created with mffm_create() but an  *
*  attempt to access it for viewing reveals that the data memory for   *
*  the frame has been released and the application wishes to reload    *
*  the data (presumably from a metafile).  This routine updates the    *
*  data structure as needed and locks the frame for loading.           *
*                                                                      *
*  Syntax:  int mffm_reload(FrameNum frame)                            *
*                                                                      *
*  Arguments:                                                          *
*     frame       Number of the frame to be reloaded.                  *
*                                                                      *
*  Return Values:                                                      *
*     MFFM_OK           It is OK now to make calls to mffm_store().    *
*     MFFM_EXISTS       The requested frame has already been loaded    *
*                       with data.                                     *
*     MFFM_WLOCK        This or some other frame is already locked     *
*                       for writing.                                   *
*     MFFM_RLOCK        This frame is locked for reading.              *
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*     MFFM_ERR_BADFRAME The requested frame does not exist.            *
*----------------------------------------------------------------------*
*                    mffm_put_hdr1, mffm_put_hdr2                      *
*                                                                      *
*  These routines store two different kinds of header information for  *
*  a specified frame.  These operations will fail if the frame has not *
*  yet been created, but will otherwise succeed even if the frame is   *
*  locked for writing or reading or if its data have been released     *
*  (to make space for other frames).  Typically, mffm_put_hdr1() is    *
*  used to store data related to the display of a frame (e.g. zoom     *
*  origin and zoom factor) and would be invoked from the GUI thread    *
*  when a frame is being displayed in order to record changes that     *
*  result from users' actions at the GUI, while mffm_put_hdr2 is used  *
*  to store data related to the acquisition of a frame (e.g. ftell/    *
*  fseek location of the frame in a metafile) and would be invoked     *
*  from the file reader thread at frame creation time.                 *
*                                                                      *
*  Syntax:  int mffm_put_hdr1(FrameNum frame, void *puhd)              *
*           int mffm_put_hdr2(FrameNum frame, void *puhd)              *
*                                                                      *
*  Arguments:                                                          *
*     frame       Number of the frame to be updated.                   *
*     puhd        Pointer to user's header data.  The number of bytes  *
*                 transferred will be the number given in the LUHdr1   *
*                 argument to the mffm_init() call for mffm_put_hdr1() *
*                 and LUHdr2 for mffm_put_hdr2().                      *
*                                                                      *
*  Return Values:                                                      *
*     MFFM_OK           Header information copied successfully.        *
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*     MFFM_ERR_BADFRAME The requested frame does not exist.            *
*     MFFM_ERR_BADARG   The length of the header data was <= 0 in      *
*                       the mffm_init() call.                          *
*----------------------------------------------------------------------*
*                       mffm_store, mffm_append                        *
*                                                                      *
*  These routines store arbitrary data, normally drawing commands, in  *
*  the releasable storage area devoted to a given frame.  The frame    *
*  must first be created with mffm_create(), which leaves the frame    *
*  in the write-locked state.  mffm_store() initiates storage of one   *
*  drawing command and may store all or only part of the data for that *
*  command.  mffm_append() may be used to add more data to a command   *
*  already started with mffm_store().  It is intended to be used when  *
*  data are read from a file in units of buffers and one command, e.g. *
*  a bitmap, extends across multiple file buffers.  When all the data  *
*  for the frame have been stored, the user must call mffm_end_put()   *
*  to unlock the frame and make it available for read access.  Data    *
*  are stored in such a manner that the same chunks can be retrieved   *
*  in FIFO order, one at a time, by calls to mffm_get_data().          *
*                                                                      *
*  Syntax:  int mffm_store(FrameNum frame, byte *pdat, long ldat)      *
*           int mffm_append(FrameNum frame, byte *pdat, long ldat)     *
*                                                                      *
*  Arguments:                                                          *
*     frame       Number of the frame in which data are to be stored.  *
*                 An MFFM_ERR_NOWLOCK error will occur if this does    *
*                 not match the frame number in the most recent        *
*                 mffm_create() or mffm_reload() call.                 *
*     pdat        Pointer to the data to be stored in the frame.       *
*     ldat        Length of the data in bytes (must be < 2**31).       *
*                                                                      *
*  Return values:                                                      *
*     MFFM_OK           Data successfully stored in frame.             *
*     MFFM_ERR_NOMEM    Not enough memory for MaxMem data storage.     *
*     MFFM_ERR_APPEND   mffm_append() was called before mffm_store()   *
*                       for the specified frame.                       *
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*     MFFM_ERR_BADFRAME The requested frame does not exist.            *
*     MFFM_ERR_OFLOCK   An attempt was made to release the least-      *
*                       recently accessed frame to make space, but     *
*                       that frame was locked for reading or writing.  *
*                       The entire available memory is taken up with   *
*                       frame headers, the frame being viewed, and/or  *
*                       the frame being written.  Either allocate more *
*                       memory or rewrite this package to release      *
*                       frame headers when needed.                     *
*     MFFM_ERR_NOWLOCK  The requested frame is not write-locked.       *
*     MFFM_ERR_BADARG   The data count (ldat) is 0 or negative or      *
*                       the data pointer (pdat) is NULL.               *
*----------------------------------------------------------------------*
*                            mffm_end_put                              *
*                                                                      *
*  Call this routine when finished loading data into a frame in order  *
*  to update data structures (thus keeping track of the total amount   *
*  of data associated with this frame), and release the write lock.    *
*  The frame cannot be accessed for reading until this is done.        *
*                                                                      *
*  Syntax:  int mffm_end_put(FrameNum frame)                           *
*                                                                      *
*  Argument:                                                           *
*     frame       Number of the frame whose write lock should be       *
*                 removed.  An MFFM_ERR_NOWLOCK error will occur if    *
*                 this does not match the frame number in the most     *
*                 recent mffm_create() or mffm_reload() call.          *
*                                                                      *
*  Return values:                                                      *
*     MFFM_OK           Operation successful.                          *
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*     MFFM_ERR_BADFRAME The specified frame does not exist.            *
*     MFFM_ERR_NOWLOCK  The specified frame is not write-locked.       *
*----------------------------------------------------------------------*
*                             mffm_access                              *
*                                                                      *
*  Call this routine to access a specific frame for viewing.  If it    *
*  is successful (the requested frame exists, has data stored with it, *
*  and is not already locked for reading or for writing), then the     *
*  frame is locked for reading.  The frame's data can be retrieved by  *
*  a series of calls to mffm_get_data(), which must specify the same   *
*  frame number.  When no further access to this frame is required,    *
*  call mffm_end_get() to release the read lock.  Until this is done,  *
*  no other frame can be accessed for reading and the data for this    *
*  frame cannot be released to make room for new frames.  However,     *
*  note that it is not necessary to access all or any of the data      *
*  with mffm_get_data before calling mffm_end_get().                   *
*                                                                      *
*  Syntax:  int mffm_access(FrameNum frame, FrameNum *pafn)            *
*                                                                      *
*  Arguments:                                                          *
*     frame       Number of the frame to be accessed.  The special     *
*                 value -1 means get the previous frame in viewing     *
*                 order and the special value 0 means get the next     *
*                 frame in viewing order.                              *
*     pafn        Pointer to location for return of the frame number   *
*                 actually accessed.  The frame number will be stored  *
*                 in this location if MFFM_OK is returned, otherwise   *
*                 the value is undefined.  This allows the caller to   *
*                 find out what frame was accessed by a call with      *
*                 frame = -1 or frame = 0.                             *
*                                                                      *
*  Return values:                                                      *
*     MFFM_OK           The frame is now available for reading.        *
*     MFFM_NOHIST       Either there is no viewing history (this is    *
*                       the first mffm_access call) or on a -1 call    *
*                       the oldest frame in the viewing history was    *
*                       the frame just accessed, or on a 0 call the    *
*                       newest frame in the viewing history was the    *
*                       frame just accessed.                           *
*     MFFM_WLOCK        The frame is write-locked.                     *
*     MFFM_RLOCK        The frame is already read-locked.              *
*     MFFM_NODATA       The frame has no data associated with it.      *
*                       Either it was created but never loaded, or     *
*                       else the data were discarded to make room      *
*                       for newer frames.                              *
*     MFFM_NOFRAME      The requested frame does not exist.            *
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*----------------------------------------------------------------------*
*                    mffm_get_hdr1, mffm_get_hdr2                      *
*                                                                      *
*  These routines retrieve, respectively, the first or second type of  *
*  caller's header information stored with a specified frame.  These   *
*  operations will fail if the frame has not yet been created or if    *
*  the requested header information has not been loaded, but otherwise *
*  will succeed even if the frame is locked for writing or reading or  *
*  if its data have been released (to make space for other frames).    *
*                                                                      *
*  Syntax:  int mffm_get_hdr1(FrameNum frame, void *puhd)              *
*  Syntax:  int mffm_get_hdr2(FrameNum frame, void *puhd)              *
*                                                                      *
*  Arguments:                                                          *
*     frame       Number of the frame whose header is wanted.          *
*     puhd        Pointer to area where the user's header data will    *
*                 be stored when MFFM_OK is returned.  The number of   *
*                 bytes transferred will be the number given in the    *
*                 LUHdr1 argument of the mffm_init() call in the case  *
*                 of mffm_get_hdr1(), and the number given by LUHdr2   *
*                 in the case of mffm_get_hdr2().  If the return code  *
*                 is anything other than MFFM_OK, this memory area is  *
*                 left unchanged.                                      *
*                                                                      *
*  Return Values:                                                      *
*     MFFM_OK           Header information copied successfully.        *
*     MFFM_NODATA       The frame exists but has no header data.       *
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*     MFFM_ERR_BADFRAME The requested frame does not exist.            *
*     MFFM_ERR_BADARG   The length of the header data was <= 0 in      *
*                       the mffm_init() call.                          *
*----------------------------------------------------------------------*
*                            mffm_get_data                             *
*                                                                      *
*  This routine retrieves data, normally drawing commands, previously  *
*  stored in the releasable storage area devoted to a given frame.     *
*  The frame must first have been accessed with mffm_access(), which   *
*  leaves the frame in the read-locked state.  When no more data need  *
*  to be retrieved from this frame (any or all or none of the data may *
*  have been retrieved), the user must call mffm_end_get() to unlock   *
*  the frame, possibly making it available for release, and allowing   *
*  a different frame to be accessed.  Data are retrieved in the same   *
*  order as previously stored.                                         *
*                                                                      *
*  Syntax:  int mffm_get_data(FrameNum frame, byte **ppd, long *pld)   *
*                                                                      *
*  Arguments:                                                          *
*     frame       Number of the frame from which data are to be        *
*                 retrieved.  An MFFM_ERR_NORLOCK error will occur     *
*                 if this does not match the frame number in the most  *
*                 recent mffm_access() call.                           *
*     ppd         Pointer to a pointer in the caller's program where   *
*                 the address of the data will be returned.            *
*     pld         Pointer to a long word in the caller's program       *
*                 where the length of the data will be returned.       *
*                 This word will be set to 0 when there are no more    *
*                 data for this frame to be returned.                  *
*                                                                      *
*  Return values:                                                      *
*     MFFM_OK           Data successfully stored in frame.             *
*     MFFM_NODATA       There are no (or no more) data to be returned. *
*     MFFM_ERR_NOMEM    Not enough memory for data assembly area.      *
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*     MFFM_ERR_BADFRAME The requested frame does not exist.            *
*     MFFM_ERR_NORLOCK  The requested frame is not read-locked.        *
*     MFFM_ERR_BADARG   Either ppd or pld is a NULL pointer.           *
*----------------------------------------------------------------------*
*                            mffm_end_get                              *
*                                                                      *
*  Call this routine when finished retrieving data from a frame in     *
*  order to release the read lock.                                     *
*                                                                      *
*  Syntax:  int mffm_end_get(FrameNum frame)                           *
*                                                                      *
*  Argument:                                                           *
*     frame       Number of the frame whose read lock should be        *
*                 removed.  An MFFM_ERR_NORLOCK error will occur if    *
*                 this does not match the frame number in the most     *
*                 recent mffm_access() call.                           *
*                                                                      *
*  Return values:                                                      *
*     MFFM_OK           Operation successful.                          *
*     MFFM_ERR_SEMOP    A system semaphore call returned an error.     *
*     MFFM_ERR_BADFRAME The requested frame does not exist.            *
*     MFFM_ERR_NORLOCK  The specified frame is not read-locked.        *
*----------------------------------------------------------------------*
*                          mffm_last_viewed                            *
*                                                                      *
*  Call this routine to retrieve the number of the frame most recently *
*  accessed for viewing.                                               *
*                                                                      *
*  Syntax:  FrameNum mffm_last_viewed(void)                            *
*                                                                      *
*  Return values:                                                      *
*     0                 No frames have yet been viewed.                *
*     >0                Number of the last frame for which             *
*                       mffm_access was called.                        *
*----------------------------------------------------------------------*
*                             mffm_expand                              *
*                                                                      *
*  This routine may be called to add additional frame memory to the    *
*  space reserved by the mffm_init() call.  The memory is not actually *
*  allocated from the OS unless needed, therefore, no test is made to  *
*  determine whether the requested memory is actually available.       *
*                                                                      *
*  Syntax:  int mffm_expand(size_t newmem)                             *
*                                                                      *
*  Argument:                                                           *
*     newmem      Amount of memory in bytes to be added to the         *
*                 maximum memory allowed to be consumed by frame       *
*                 data.                                                *
*                                                                      *
*  Return value:                                                       *
*     MFFM_OK           This routine always succeeds.                  *
*----------------------------------------------------------------------*
*                            mffm_cleanup                              *
*                                                                      *
*  Call this routine to free up all memory and semaphores used by the  *
*  mffm package and to return statistics that may be useful for opti-  *
*  mizing memory management parameters.  This routine is optional when *
*  the application is ready to exit, as the operating system should    *
*  perform the cleanup, but it may be useful if an application wants   *
*  to recover frame memory for some other reason (for example, to try  *
*  different configurations in a test program).                        *
*                                                                      *
*  Syntax:  int mffm_cleanup(long stats[MFFM_NSTATS])                  *
*                                                                      *
*  Argument:                                                           *
*     stats             An array of MFFM_NSTATS (6) long words in      *
*                       which usage statistics are returned.  A NULL   *
*                       pointer may be passed if statistics are not    *
*                       wanted.                                        *
*                                                                      *
*  Statistics returned:                                                *
*     stats[MFSTAT_FIXMEM]   Amount of memory used for fixed overhead  *
*                       (i.e. outside the MaxMem limit, for globals,   *
*                       the index, view history, and an assembly area  *
*                       for fragmented frame data).                    *
*     stats[MFSTAT_BLKMEM]   Total block memory (i.e. memory within    *
*                       the MaxMem limit, used for frame headers,      *
*                       frame data (drawing commands) and unused.      *
*     stats[MFSTAT_HDRMEM]   Memory in MFSTAT_BLKMEM blocks that is    *
*                       set aside for fixed frame header data.         *
*                       (The rest is used for drawing commands.)       *
*     stats[MFSTAT_DATMEM]   Memory in MFSTAT_BLKMEM blocks that is    *
*                       set aside for frame data (drawing commands).   *
*     stats[MFSTAT_HDRWASTE]   Memory within MFSTAT_HDRMEM memory      *
*                       that is wasted due to packing headers into     *
*                       fixed-size blocks.                             *
*     stats[MFSTAT_DATWASTE]   Memory within MFSTAT_DATMEM memory      *
*                       that is wasted due to packing frame data       *
*                       into fixed-size blocks.                        *
*                                                                      *
*  Usage in a threaded environment:                                    *
*     If the application uses separate threads for storing and         *
*     retrieving frame data, this routine should be called only        *
*     once, from the last thread to terminate.                         *
*                                                                      *
*  Return values:                                                      *
*     MFFM_OK           Memory and semaphores successfully removed.    *
*     MFFM_ERR_NOMEM    There is nothing to remove--presumably         *
*                       mffm_init() was never called since the         *
*                       start of the run or since the last cleanup.    *
************************************************************************
*  V1A, 05/24/08, G.N. Reeke, new program.                             *
*  Rev, 05/31/08, GNR, Eliminate double-linked data block list, rename *
*                      BADCT error to BADARG, recalc data block length *
*                      to eliminate header waste, bug fixes            *
*  Rev, 07/04/08, GNR, Add mffm_last_viewed(), expand comments, clean  *
*                      up -Wall compilation warnings                   *
*  V1B, 07/12/08, GNR, Add get_hdr[12], put_hdr[12], append calls      *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include "mffm.h"

/* Configuration parameters -- adjustable within reason */

#define MfdBlockSize     1024    /* Size of one data block */
#define MfdBlockPack    32768    /* Size of block pack allocation */
#define MfdDCAAExtra     2048    /* Extra space in dcaa added when
                                 *  expanded to minimize reallocs */
#define InitIndexSize   10000    /* No. of frames in initial index */
#define MoreIndexSize   20000    /* Frames to add when index full */

/* Internal definitions -- fixed by code assumptions, do not change */

#define EndSignal           0    /* Length code for end of frame */
#define SemPShare           0    /* Semaphore process share flag */
#define SemCount            1    /* Semaphore resource count */

/* Macros from GNR sysdef.h */
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
/* Align data size to next higher multiple of pointer size */
#ifndef ALIGN_UP
#define BYTE_ALIGN sizeof(void *)
#define ALIGN_UP(s) (((s)+BYTE_ALIGN-1)&~(BYTE_ALIGN-1))
#endif

/* Internal data structures */

typedef char BlkPack;            /* Bookkeeping for releasing
                                 *  block packs */

typedef struct MfdBlockDef {     /* Metafile data block */
   struct MfdBlockDef *pndb;     /* Ptr to next data block */
   byte               *pnfd;     /* Ptr to next free space in block */
   } MfdBlock;

typedef struct FrameHdrDef {     /* Frame header */
   struct MfdBlockDef *pfdb;     /* Ptr to frame's first data block */
   struct MfdBlockDef *pldb;     /* Ptr to frame's last data block */
   struct FrameHdrDef *pnfa;     /* Ptr to next frame on access hist */
   struct FrameHdrDef *ppfa;     /* Ptr to prev frame on access hist */
   int    hflgs;                 /* Header flags */
#define MFHF_UHDRLD1  1             /* User header1 loaded */
#define MFHF_UHDRLD2  2             /* User header2 loaded */
#define MFHF_STORED   4             /* Some data were stored */
#define MFHF_APPNDED  8             /* Some data were appended */
   } FrameHdr;

typedef struct CmdCountDef {     /* Stored data count finder */
   struct MfdBlockDef *pccb;     /* Ptr to block where data starts */
   byte               *pccl;     /* Ptr to current data length */
   long               lccmd;     /* Current value of data length */
   } CmdCount;

struct MFMMGlob {
   BlkPack     *pfbp;            /* Ptr to first block pack */
   FrameHdr    **pI;             /* Ptr to frame index */
   MfdBlock    *phbc;            /* Ptr to header block chain */
   MfdBlock    *pfbc;            /* Ptr to free block chain */
   MfdBlock    *pcrb;            /* Ptr to current read block */
   FrameNum    *pvhl;            /* Ptr to view history list */
   FrameNum    *pvhe;            /* Ptr to view history end */
   FrameNum    *pvhn;            /* Ptr to newest on view history */
   FrameNum    *pvho;            /* Ptr to oldest on view history */
   FrameNum    *pvhv;            /* Ptr to view history being viewed */
   byte        *pdcaa;           /* Ptr to drawing command assy area */

   sem_t       mfsem;            /* POSIX semaphore for mffm */
   FrameHdr    OldNew;           /* Start and end of access history */
   CmdCount    CCC;              /* Current command store count info */
   size_t      lhdr1,lhdr2;      /* Length of user header data */
   size_t      lchdr;            /* Length of a complete header */
   size_t      ldblk;            /* Length of a data block adjusted
                                 *  up to multiple of lchdr */
   size_t      ldcaa;            /* Current size of dcaa */
   long        nbata;            /* Number blocks avail to allocate */
   long        npack;            /* Number blocks in a pack */
   FrameNum    lindx;            /* Current size (# frames) of index */
   FrameNum    lockw;            /* Frame locked for writing */
   FrameNum    lockr;            /* Frame locked for reading */
   long        lhist;            /* Length of history list */
   };

/* Global data */

   static struct MFMMGlob *pMG;  /* Ptr to global data */


/***********************************************************************
*                    Internal ("Private") Routines                     *
***********************************************************************/


/*=====================================================================*
*                            mfp_get_free                              *
*                                                                      *
*  Add one or more memory blocks to the free list.  If the maximum     *
*  memory allocation has already been used, release data from one      *
*  or more frames until a block can be freed.  Otherwise, allocate     *
*  a pack of data blocks (to minimize system calls) and format the     *
*  pack into one or more blocks.  Assumes caller holds semaphore.      *
*=====================================================================*/

static int mfp_get_free(void) {

   if (pMG->nbata > 0) {         /* Allocate new data blocks */
      BlkPack *pack;
      MfdBlock *pblk,*pnxt,**pprv;
      long iblk,numblks = min(pMG->nbata, pMG->npack);
      pack = (BlkPack *)malloc(numblks*pMG->ldblk + sizeof(BlkPack *));
      if (!pack) return MFFM_ERR_NOMEM;
      *(BlkPack **)pack = pMG->pfbp;
      pMG->pfbp = pack;
      pblk = (MfdBlock *)(pack + sizeof(BlkPack *));
      pnxt = *(pprv = &pMG->pfbc);
      for (iblk=0; iblk<numblks; ++iblk) {
         *pprv = pblk;
         pprv = &pblk->pndb;
         pblk = (MfdBlock *)((byte *)pblk + pMG->ldblk);
         }
      *pprv = pnxt;
      pMG->nbata -= numblks;
      }
   else {                        /* Release old data blocks */
      FrameHdr *phdr = pMG->OldNew.pnfa;
      if (phdr == &pMG->OldNew || phdr == pMG->pI[pMG->lockr] ||
           phdr == pMG->pI[pMG->lockw] || !phdr->pfdb)
         return MFFM_ERR_OFLOCK;
      /* Move data blocks to free chain */
      phdr->pldb->pndb = pMG->pfbc;
      pMG->pfbc = phdr->pfdb;
      /* Indicate empty chain */
      phdr->pfdb = phdr->pldb = NULL;
      /* Remove from access history */
      phdr->ppfa->pnfa = phdr->pnfa;
      phdr->pnfa->ppfa = phdr->ppfa;
      phdr->pnfa = phdr->ppfa = NULL;
      }

   return MFFM_OK;

   } /* End mfp_get_free() */


/*=====================================================================*
*                            mfp_put_data                              *
*                                                                      *
*  Add user data (drawing commands) to a frame.  If pccd argument      *
*  is not a NULL pointer, store info on start of data there.           *
*  Note:  The frame should be locked for writing, but the caller       *
*  should free (or not acquire) the semaphore before calling this      *
*  routine.                                                            *
*=====================================================================*/

static int mfp_put_data(FrameHdr *phdr, CmdCount *pccd,
      byte *pdat, long ldat) {

   MfdBlock *pdbk = phdr->pldb;
   long lrem;                    /* Empty space remaining in block */
   long lseg;                    /* Length of one data segment */
   int rc = MFFM_OK;

   while (ldat) {
      if (!pdbk ||
            (lrem = (byte *)pdbk + pMG->ldblk - pdbk->pnfd) <= 0) {

         /* Need another data block.
         *  Acquire exclusive access to data structure */
         if (sem_wait(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

         /* If there are no free blocks, try to make some */
         if (!pMG->pfbc && (rc = mfp_get_free()) != 0)
            goto ClearAndReturnOnError;

         /* Move a free block to the frame data block chain at a
         *  position following the current last block if there is
         *  one, otherwise at the head of the drawing chain and
         *  make a subchain of one out of it.  */
         if (phdr->pldb)
            phdr->pldb = phdr->pldb->pndb = pMG->pfbc;
         else
            phdr->pfdb = phdr->pldb = pMG->pfbc;
         pMG->pfbc = phdr->pldb->pndb;
         phdr->pldb->pndb = NULL;

         pdbk = phdr->pldb;
         pdbk->pnfd = (byte *)pdbk + sizeof(MfdBlock);
         lrem = (byte *)pdbk + pMG->ldblk - pdbk->pnfd;

         /* Now we can relinquish exclusive access */
ClearAndReturnOnError:
         if (sem_post(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;
         if (rc != MFFM_OK) return rc;
         } /* End making new data space */

      /* If requested, save location of this data (count) field */
      if (pccd) {
         pccd->pccb = pdbk;
         pccd->pccl = pdbk->pnfd;
         pccd = NULL;
         }

      /* Move what will fit */
      lseg = min(ldat, lrem);
      memcpy((char *)pdbk->pnfd, (char *)pdat, lseg);
      pdbk->pnfd += lseg;
      pdat += lseg;
      ldat -= lseg;
      } /* End while ldat */

   return rc;

   } /* End mfp_put_data() */


/*=====================================================================*
*                            mfp_get_data                              *
*                                                                      *
*  Retrieve user data (drawing commands) from a frame.                 *
*  Note:  The frame must be locked for reading and the current data    *
*  block pointer set in pMG->pcrb.                                     *
*=====================================================================*/

static int mfp_get_data(byte *pdat, long ldat) {

   MfdBlock *pdbk = pMG->pcrb;   /* Locate current read block */
   long lrem;                    /* Empty space remaining in block */
   long lseg;                    /* Length of one data segment */

   while (ldat) {
      if ((lrem = (byte *)pdbk + pMG->ldblk - pdbk->pnfd) <= 0) {
         pdbk = pdbk->pndb;
         if (!pdbk) return MFFM_ERR_NOMEM;   /* JIC */
         pMG->pcrb = pdbk;
         pdbk->pnfd = (byte *)pdbk + sizeof(MfdBlock);
         lrem = (byte *)pdbk + pMG->ldblk - pdbk->pnfd;
         } /* End making new data space */

      /* Move part in current block */
      lseg = min(ldat, lrem);
      memcpy((char *)pdat, (char *)pdbk->pnfd, lseg);
      pdbk->pnfd += lseg;
      pdat += lseg;
      ldat -= lseg;
      } /* End while ldat */

   return MFFM_OK;

   } /* End mfp_get_data() */


/***********************************************************************
*                  User-Visible ("Public") Routines                    *
***********************************************************************/

/*=====================================================================*
*                              mffm_init                               *
*=====================================================================*/

int mffm_init(size_t MaxMem, size_t LUHdr1, size_t LUHdr2,
      long HistSize) {

   long tnb;                     /* Temp for block size calcs */

   pMG = (struct MFMMGlob *)calloc(1, sizeof(struct MFMMGlob));
   if (!pMG) return MFFM_ERR_NOMEM;

   /* Allocate index, with extra space for nonexistent frame 0.
   *  (This frame may be accessed to check a lock that does not
   *  exist, so it should always contain a NULL pointer.)  */
   pMG->pI = (FrameHdr **)calloc(InitIndexSize+1, sizeof(FrameHdr *));
   if (!pMG->pI) return MFFM_ERR_NOMEM;
   pMG->lindx = InitIndexSize;

   /* Decide how big to make a header block and a data block */
   pMG->lhdr1 = LUHdr1, pMG->lhdr2 = LUHdr2;
   pMG->lchdr = ALIGN_UP(LUHdr1+LUHdr2) + sizeof(FrameHdr);
   tnb = (MfdBlockSize + pMG->lchdr - 1)/pMG->lchdr;
   pMG->ldblk = tnb*pMG->lchdr + sizeof(MfdBlock);
   pMG->npack = (MfdBlockPack-sizeof(BlkPack *))/pMG->ldblk;

   pMG->nbata = (MaxMem + pMG->ldblk - 1)/pMG->ldblk;
   pMG->lhist = HistSize;

   /* Create empty view history */
   pMG->pvhl = (FrameNum *)calloc(HistSize, sizeof(FrameNum));
   if (!pMG->pvhl) return MFFM_ERR_NOMEM;
   pMG->pvhv = pMG->pvhe = pMG->pvhl + HistSize - 1;
   pMG->pvhn = pMG->pvho = pMG->pvhl;

   /* Create empty access history */
   pMG->OldNew.pnfa = pMG->OldNew.ppfa = &pMG->OldNew;

   /* Set up semaphore */
   if (sem_init(&pMG->mfsem, SemPShare, SemCount) < 0)
      return MFFM_ERR_SEMOP;

   return MFFM_OK;

   } /* End mffm_init() */


/*=====================================================================*
*                             mffm_create                              *
*=====================================================================*/

int mffm_create(FrameNum frame) {

   MfdBlock *phbk;
   FrameHdr *phdr,*phub;
   int rc = MFFM_OK;             /* Return code */

   if (frame <= 0) return MFFM_ERR_BADFRAME;

   /* Acquire exclusive access to data structure */
   if (sem_wait(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   /* Lock this frame for writing */
   pMG->lockw = frame;

   if (frame > pMG->lindx) {     /* Expand the index */
      size_t oldsz = pMG->lindx + 1;
      size_t newsz = oldsz + MoreIndexSize;
      pMG->pI =
         (FrameHdr **)realloc(pMG->pI, newsz*sizeof(FrameHdr *));
      if (!pMG->pI) {            /* No memory for index */
         rc = MFFM_ERR_NOMEM; goto ClearAndReturn; }
      memset((char *)(pMG->pI + oldsz), 0,
         MoreIndexSize*sizeof(FrameHdr *));
      pMG->lindx += MoreIndexSize;
      }
   else if (pMG->pI[frame]) {    /* Frame already exists */
      rc = MFFM_EXISTS; goto ClearAndReturn; }

   /* Find space for the header block */
   phbk = pMG->phbc;
   if (!phbk || phbk->pnfd+pMG->lchdr > (byte *)phbk+pMG->ldblk) {
      /* No header block or no room in current header block.
      *  If there are no free blocks, try to make some.  */
      if (!pMG->pfbc && (rc = mfp_get_free()) != 0)
         goto ClearAndReturn;
      /* Move a free block to header block chain */
      phbk = pMG->pfbc;
      pMG->pfbc = phbk->pndb;
      phbk->pndb = pMG->phbc;
      pMG->phbc = phbk;
      /* Indicate no data in this block */
      phbk->pnfd = (byte *)phbk + sizeof(MfdBlock);
      }

   /* Generate and clear the header block */
   phdr = (FrameHdr *)phbk->pnfd;
   phbk->pnfd += pMG->lchdr;
   memset((char *)phdr, 0, pMG->lchdr);

   /* Add to access history at end of list */
   phub = &pMG->OldNew;
   phub->ppfa->pnfa = phdr;
   phdr->ppfa = phub->ppfa;
   phdr->pnfa = phub;
   phub->ppfa = phdr;

   /* Add empty frame to master index */
   pMG->pI[frame] = phdr;

ClearAndReturn:                  /* Clear semaphore and return */
   if (sem_post(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   return rc;

   } /* End mffm_create() */


/*=====================================================================*
*                             mffm_reload                              *
*=====================================================================*/

int mffm_reload(FrameNum frame) {

   FrameHdr *phdr,*phub;
   int rc = MFFM_OK;             /* Return code */

   if (frame <= 0) return MFFM_ERR_BADFRAME;

   /* Acquire exclusive access to data structure */
   if (sem_wait(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   phdr = (frame <= pMG->lindx) ? pMG->pI[frame] : NULL;
   if (!phdr) {                  /* Frame not yet created */
      rc = MFFM_ERR_BADFRAME; goto ClearAndReturn; }
   if (phdr->pfdb) {             /* Frame has data loaded */
      rc = MFFM_EXISTS; goto ClearAndReturn; }
   if (pMG->lockw) {             /* There is already a write lock */
      rc = MFFM_WLOCK; goto ClearAndReturn; }
   if (pMG->lockr == frame) {    /* This very frame is read-locked */
      rc = MFFM_RLOCK; goto ClearAndReturn; }

   /* Add to access history at end of list */
   phub = &pMG->OldNew;
   phub->ppfa->pnfa = phdr;
   phdr->ppfa = phub->ppfa;
   phdr->pnfa = phub;
   phub->ppfa = phdr;

   /* Allow error to be detected if mffm_append() called too soon */
   phdr->hflgs &= ~(MFHF_STORED|MFHF_APPNDED);

   /* Lock this frame for writing */
   pMG->lockw = frame;

ClearAndReturn:                  /* Clear semaphore and return */
   if (sem_post(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   return rc;

   } /* End mffm_reload() */


/*=====================================================================*
*                    mffm_put_hdr1, mffm_put_hdr2                      *
*                                                                      *
*  Note:  It might appear that no semaphore is needed here, because    *
*  frame headers are never removed once installed and these routines   *
*  do not change anything in the data structure other than the user    *
*  data.   However, memcpy() might not be atomic and one thread might  *
*  try to read a header while the other is storing the same header     *
*  (although the intent is that each thread read and write only its    *
*  own header type).  Also, there is the remote possibility that while *
*  the reader thread is executing one of these routines, a new frame   *
*  that requires expanding the index is created by the writer thread,  *
*  thereby causing pMG->pI to obtain a stale value here.               *
*=====================================================================*/

int mffm_put_hdr1(FrameNum frame, void *puhd) {

   FrameHdr *phdr;
   int rc;

   if (frame <= 0) return MFFM_ERR_BADFRAME;

   /* Acquire exclusive access to data structure */
   if (sem_wait(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   if (pMG->lhdr1 <= 0) {        /* JIC */
      rc = MFFM_ERR_BADARG; goto ClearAndReturn; }
   /* Locate header info for this frame */
   phdr = (frame <= pMG->lindx) ? pMG->pI[frame] : NULL;
   if (!phdr) {
      rc = MFFM_ERR_BADFRAME; goto ClearAndReturn; }
   /* Store header and set flag */
   memcpy ((char *)phdr+sizeof(FrameHdr), (char *)puhd, pMG->lhdr1);
   phdr->hflgs |= MFHF_UHDRLD1;
   rc = MFFM_OK;

ClearAndReturn:                  /* Clear semaphore and return */
   if (sem_post(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;
   return rc;

   } /* End mffm_put_hdr1() */


int mffm_put_hdr2(FrameNum frame, void *puhd) {

   FrameHdr *phdr;
   int rc;

   if (frame <= 0) return MFFM_ERR_BADFRAME;

   /* Acquire exclusive access to data structure */
   if (sem_wait(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   if (pMG->lhdr2 <= 0) {        /* JIC */
      rc = MFFM_ERR_BADARG; goto ClearAndReturn; }
   /* Locate header info for this frame */
   phdr = (frame <= pMG->lindx) ? pMG->pI[frame] : NULL;
   if (!phdr) {
      rc = MFFM_ERR_BADFRAME; goto ClearAndReturn; }
   /* Store header and set flag */
   memcpy ((char *)phdr+pMG->lhdr1+sizeof(FrameHdr),
      (char *)puhd, pMG->lhdr2);
   phdr->hflgs |= MFHF_UHDRLD2;
   rc = MFFM_OK;

ClearAndReturn:                  /* Clear semaphore and return */
   if (sem_post(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;
   return rc;

   } /* End mffm_put_hdr2() */


/*=====================================================================*
*                             mffm_store                               *
*=====================================================================*/

int mffm_store(FrameNum frame, byte *pdat, long ldat) {

   FrameHdr *phdr;
   int rc;                       /* Return code */

   /* Argument checking */
   if (!pdat || ldat <= 0) return MFFM_ERR_BADARG;

   /* If the frame does not exist, the lock test will perforce fail,
   *  so a separate existence test is not needed, but one is done
   *  anyway just to give the user a more informative error code.  */
   if (frame != pMG->lockw)
      return (frame <= 0 || frame > pMG->lindx || !pMG->pI[frame]) ?
         MFFM_ERR_BADFRAME : MFFM_ERR_NOWLOCK;

   phdr = pMG->pI[frame];        /* Locate frame header */

   /* Save the count, then move the data */
   pMG->CCC.lccmd = ldat;
   rc = mfp_put_data(phdr, &pMG->CCC, (byte *)&ldat, sizeof(long));
   if (rc) return rc;
   rc = mfp_put_data(phdr, NULL, pdat, ldat);
   phdr->hflgs |= MFHF_STORED;
   return rc;

   } /* End mffm_store() */


/*=====================================================================*
*                             mffm_append                              *
*=====================================================================*/

int mffm_append(FrameNum frame, byte *pdat, long ldat) {

   FrameHdr *phdr;
   int rc;                       /* Return code */

   /* Argument checking */
   if (!pdat || ldat <= 0) return MFFM_ERR_BADARG;

   /* If the frame does not exist, the lock test will perforce fail,
   *  so a separate existence test is not needed, but one is done
   *  anyway just to give the user a more informative error code.  */
   if (frame != pMG->lockw)
      return (frame <= 0 || frame > pMG->lindx || !pMG->pI[frame]) ?
         MFFM_ERR_BADFRAME : MFFM_ERR_NOWLOCK;

   phdr = pMG->pI[frame];        /* Locate frame header */

   /* Error if mffm_store() was not called first for this frame */
   if (!(phdr->hflgs & MFHF_STORED)) return MFFM_ERR_APPEND;

   /* Increment the count, then store the data.  The updated count is
   *  stored in the data structure by mffm_end_put()--there is no need
   *  to do this messy operation during each of a series of appends. */
   pMG->CCC.lccmd += ldat;
   rc = mfp_put_data(phdr, NULL, pdat, ldat);

   return rc;

   } /* End mffm_append() */


/*=====================================================================*
*                            mffm_end_put                              *
*=====================================================================*/

int mffm_end_put(FrameNum frame) {

   FrameHdr *phdr;
   long endsig = EndSignal;
   int rc;                       /* Return code */

   /* If the frame does not exist, the lock test will perforce fail,
   *  so a separate existence test is not needed, but one is done
   *  anyway just to give the user a more informative error code.  */
   if (frame != pMG->lockw)
      return (frame <= 0 || frame > pMG->lindx || !pMG->pI[frame]) ?
         MFFM_ERR_BADFRAME : MFFM_ERR_NOWLOCK;

   phdr = pMG->pI[frame];        /* Locate frame header */
   rc = MFFM_OK;

   /* Update the stored command count if there was an append.
   *  Note that the count may be split across two blocks.  */
   if (phdr->hflgs & MFHF_APPNDED) {
      char *pscc = (char *)pMG->CCC.pccl;    /* Ptr to stored count */
      char *ptcc = (char *)&pMG->CCC.lccmd;  /* Ptr to temp count */
      long ldat = sizeof(long);
      long lseg = (char *)pMG->CCC.pccb + pMG->ldblk - pscc;
      if (lseg < ldat) {                     /* Move in two pieces */
         memcpy(pscc, ptcc, lseg);
         pscc = (char *)pMG->CCC.pccb->pndb + sizeof(MfdBlock);
         ptcc += lseg;
         ldat -= lseg;
         }
      memcpy(pscc, ptcc, ldat);
      } /* End storing updated count */

   /* Store end signal.  Assignment intended in next if */
   rc = mfp_put_data(phdr, NULL, (byte *)&endsig, sizeof(long));
   if (rc) return rc;

   /* Remove write lock */
   pMG->lockw = 0;

   return rc;

   } /* End mffm_end_put() */


/*=====================================================================*
*                             mffm_access                              *
*=====================================================================*/

int mffm_access(FrameNum frame, FrameNum *pafn) {

   FrameNum *pnxt;
   FrameHdr *phdr,*phub;
   MfdBlock *pblk;
   int rc = MFFM_OK;             /* Return code */

   /* Acquire exclusive access to data structure */
   if (sem_wait(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   /* Traverse view history.  Treat any negative frame as a
   *  request to go back one, rather than that number.  */
   if (frame < 0) {
      if (pMG->pvhv == pMG->pvho) {
         rc = MFFM_NOHIST; goto ClearAndReturn; }
      pMG->pvhv = (pMG->pvhv == pMG->pvhl) ? pMG->pvhe : pMG->pvhv-1;
      if ((frame = *pMG->pvhv) <= 0) {
         rc = MFFM_NOHIST; goto ClearAndReturn; }
      }
   else if (frame == 0) {
      pnxt = (pMG->pvhv == pMG->pvhe) ? pMG->pvhl : pMG->pvhv+1;
      if (pnxt == pMG->pvhn) {
         rc = MFFM_NOHIST; goto ClearAndReturn; }
      pMG->pvhv = pnxt;
      if ((frame = *pnxt) <= 0) {
         rc = MFFM_NOHIST; goto ClearAndReturn; }
      }
   else {
      if (frame > pMG->lindx) {
         rc = MFFM_NOFRAME; goto ClearAndReturn; }
      *(pMG->pvhv = pMG->pvhn) = frame;
      pnxt = (pMG->pvhn == pMG->pvhe) ? pMG->pvhl : pMG->pvhn+1;
      if (pMG->pvhn == pMG->pvho) pMG->pvho = pnxt;
      pMG->pvhn = pnxt;
     }

   phdr = pMG->pI[frame];
   if (!phdr) {                  /* Frame not yet created */
      rc = MFFM_NOFRAME; goto ClearAndReturn; }
   if (!phdr->pfdb) {            /* Frame has no data loaded */
      rc = MFFM_NODATA; goto ClearAndReturn; }
   if (pMG->lockw == frame) {    /* This frame is write-locked */
      rc = MFFM_WLOCK; goto ClearAndReturn; }
   if (pMG->lockr) {             /* Any frame is read-locked */
      rc = MFFM_RLOCK; goto ClearAndReturn; }

   /* Lock this frame for reading */
   pMG->lockr = frame;

   /* Initialize read block pointer scheme */
   pMG->pcrb = pblk = phdr->pfdb;
   pblk->pnfd = (byte *)pblk + sizeof(MfdBlock);

   /* Make this frame newest on access history if not already */
   phub = &pMG->OldNew;
   if (phdr->pnfa != phub) {
      phdr->ppfa->pnfa = phdr->pnfa;
      phdr->pnfa->ppfa = phdr->ppfa;
      phub->ppfa->pnfa = phdr;
      phdr->ppfa = phub->ppfa;
      phdr->pnfa = phub;
      phub->ppfa = phdr;
      }

ClearAndReturn:                  /* Clear semaphore and return */
   if (sem_post(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   *pafn = frame;

   return rc;

   } /* End mffm_access() */


/*=====================================================================*
*                    mffm_get_hdr1, mffm_get_hdr2                     *
*=====================================================================*/

int mffm_get_hdr1(FrameNum frame, void *puhd) {

   FrameHdr *phdr;
   int rc;

   if (frame <= 0) return MFFM_ERR_BADFRAME;

   /* Acquire exclusive access to data structure */
   if (sem_wait(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   if (pMG->lhdr1 <= 0) {        /* JIC */
      rc = MFFM_ERR_BADARG; goto ClearAndReturn; }
   /* Locate header info for this frame */
   phdr = (frame <= pMG->lindx) ? pMG->pI[frame] : NULL;
   if (!phdr) {
      rc = MFFM_ERR_BADFRAME; goto ClearAndReturn; }
   if (!(phdr->hflgs & MFHF_UHDRLD1)) {
      rc = MFFM_NODATA; goto ClearAndReturn; }

   /* If header is found, copy the data.  Continue to hold the
   *  semaphore here just in case another thread tries to replace
   *  this header at the same time.  */
   memcpy ((char *)puhd, (char *)phdr+sizeof(FrameHdr), pMG->lhdr1);
   rc = MFFM_OK;

ClearAndReturn:                  /* Clear semaphore and return */
   if (sem_post(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;
   return rc;

   } /* End mffm_get_hdr1() */


int mffm_get_hdr2(FrameNum frame, void *puhd) {

   FrameHdr *phdr;
   int rc;

   if (frame <= 0) return MFFM_ERR_BADFRAME;

   /* Acquire exclusive access to data structure */
   if (sem_wait(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   if (pMG->lhdr2 <= 0) {        /* JIC */
      rc = MFFM_ERR_BADARG; goto ClearAndReturn; }
   /* Locate header info for this frame */
   phdr = (frame <= pMG->lindx) ? pMG->pI[frame] : NULL;
   if (!phdr) {
      rc = MFFM_ERR_BADFRAME; goto ClearAndReturn; }
   if (!(phdr->hflgs & MFHF_UHDRLD2)) {
      rc = MFFM_NODATA; goto ClearAndReturn; }

   /* If header is found, copy the data.  Continue to hold the
   *  semaphore here just in case another thread tries to replace
   *  this header at the same time.  */
   memcpy ((char *)puhd, (char *)phdr+pMG->lhdr1+sizeof(FrameHdr),
      pMG->lhdr2);
   rc = MFFM_OK;

ClearAndReturn:                  /* Clear semaphore and return */
   if (sem_post(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;
   return rc;

   } /* End mffm_get_hdr2() */


/*=====================================================================*
*                            mffm_get_data                             *
*                                                                      *
*  Note:  Because most data will be stored contiguously, and because   *
*  the caller will not know what length to expect without significant  *
*  work, this routine is designed to pass the location and length of   *
*  the data back to the caller for access as desired.  However, when   *
*  the data have been divided across two or more blocks, the caller    *
*  should not need to know this fact.  In this case, the data are      *
*  collected in a temporary area called the drawing command assembly   *
*  area (dcaa) and its address is returned.  This area is allocated    *
*  by a "high-water-mark" algorithm as needed.                         *
*=====================================================================*/

int mffm_get_data(FrameNum frame, byte **ppd, long *pld) {

   long ldat;                    /* Length of data */
   int rc;                       /* Return code */

   /* Return NULL data pointer if something goes wrong */
   *ppd = NULL;

   /* Argument checking */
   if (!ppd || !pld) return MFFM_ERR_BADARG;

   /* If the frame does not exist, the lock test will perforce fail,
   *  so a separate existence test is not needed, but one is done
   *  anyway just to give the user a more informative error code.  */
   if (frame != pMG->lockr)
      return (frame <= 0 || frame > pMG->lindx || !pMG->pI[frame]) ?
         MFFM_ERR_BADFRAME : MFFM_ERR_NORLOCK;

   if (!pMG->pcrb) return MFFM_NODATA;

   /* Retrieve the count -- Assignment intended in next if */
   rc = mfp_get_data((byte *)pld, sizeof(long));
   if (rc) return rc;
   /* End-of-file encountered? */
   if ((ldat = *pld) == EndSignal) return MFFM_NODATA;

   /* If the data are not all in the current block,
   *  expand the dcaa if necessary and collect the data there */
   if ((byte *)pMG->pcrb + pMG->ldblk - pMG->pcrb->pnfd < ldat) {
      if (pMG->ldcaa < (size_t)ldat) {
         /* Allow some extra space to minimize reallocs */
         size_t bigldat = ldat + MfdDCAAExtra;
         pMG->pdcaa = (byte *)realloc(pMG->pdcaa, bigldat);
         if (!pMG->pdcaa) {
            pMG->ldcaa = 0; return MFFM_ERR_NOMEM; }
         pMG->ldcaa = bigldat;
         }
      /* Assignment intended in next if */
      rc = mfp_get_data(pMG->pdcaa, ldat);
      if (rc) return rc;
      *ppd = pMG->pdcaa;
      }
   /* Data are contiguous, return location, update internals */
   else {
      *ppd = pMG->pcrb->pnfd;
      pMG->pcrb->pnfd += ldat;
      }

   return MFFM_OK;

   } /* End mffm_get_data() */


/*=====================================================================*
*                            mffm_end_get                              *
*=====================================================================*/

int mffm_end_get(FrameNum frame) {

   /* If the frame does not exist, the lock test will perforce fail,
   *  so a separate existence test is not needed, but one is done
   *  anyway just to give the user a more informative error code.  */
   if (frame != pMG->lockr)
      return (frame <= 0 || frame > pMG->lindx || !pMG->pI[frame]) ?
         MFFM_ERR_BADFRAME : MFFM_ERR_NORLOCK;

   /* Remove read lock */
   pMG->pcrb = NULL;             /* JIC */
   pMG->lockr = 0;

   return MFFM_OK;

   } /* End mffm_end_get() */


/*=====================================================================*
*                          mffm_last_viewed                            *
*=====================================================================*/

FrameNum mffm_last_viewed(void) {

   return *pMG->pvhv;

   } /* End mffm_last_viewed() */


/*=====================================================================*
*                             mffm_expand                              *
*=====================================================================*/

int mffm_expand(size_t newmem) {

   pMG->nbata += (newmem + pMG->ldblk - 1)/pMG->ldblk;

   return MFFM_OK;

   } /* End mffm_expand() */


/*=====================================================================*
*                            mffm_cleanup                              *
*=====================================================================*/

int mffm_cleanup(long stats[MFFM_NSTATS]) {

   BlkPack *pack,*pnxt;

   if (!pMG) return MFFM_ERR_NOMEM;

   /* Acquire exclusive access to data structure */
   if (sem_wait(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   /* Calculate and return statistics (if wanted) */
   if (stats) {
      MfdBlock *pblk;
      FrameHdr *phdr,*phub;
      long ntotblks  = 0;
      long nhdrblks  = 0;
      long ndatblks  = 0;
      long nhdrwaste = 0;
      long ndatwaste = 0;

      /* Count data blocks and data waste.  Because there is no data
      *  block chain, it is necessary first to traverse the header
      *  block chain to find all the data blocks.  */
      phub = &pMG->OldNew;
      for (phdr=pMG->OldNew.pnfa; phdr != phub; phdr=phdr->pnfa) {
         for (pblk=phdr->pfdb; pblk; pblk=pblk->pndb) {
            ndatblks += 1;
            ndatwaste += (byte *)pblk + pMG->ldblk - pblk->pnfd;
            }
         }
      /* Count header blocks and header waste */
      for (pblk=pMG->phbc; pblk; pblk=pblk->pndb) {
         nhdrblks += 1;
         nhdrwaste += (byte *)pblk + pMG->ldblk - pblk->pnfd;
         }
      /* Count free blocks */
      for (pblk=pMG->pfbc; pblk; pblk=pblk->pndb) {
         ntotblks += 1;
         }
      stats[MFSTAT_FIXMEM] = sizeof(struct MFMMGlob) + pMG->ldcaa +
         pMG->lindx*sizeof(FrameHdr *) + pMG->lhist*sizeof(FrameNum);
      ntotblks += nhdrblks + ndatblks;
      stats[MFSTAT_BLKMEM] = pMG->ldblk * ntotblks;
      stats[MFSTAT_HDRMEM] = pMG->ldblk * nhdrblks;
      stats[MFSTAT_DATMEM] = pMG->ldblk * ndatblks;
      stats[MFSTAT_HDRWASTE] = nhdrwaste;
      stats[MFSTAT_DATWASTE] = ndatwaste;
      }

   /* Free the block packs */
   for (pack=pMG->pfbp; pack; ) {
      pnxt = *(BlkPack **)pack; free(pack); pack = pnxt; }
   free(pMG->pI);                /* Free index */
   free(pMG->pvhl);              /* Free view history */
   if (pMG->pdcaa) free(pMG->pdcaa);

   /* Free semaphore and remove from system */
   if (sem_post(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;
   if (sem_destroy(&pMG->mfsem) < 0) return MFFM_ERR_SEMOP;

   /* Finally, get rid of global data */
   free(pMG);
   pMG = NULL;

   return MFFM_OK;

   } /* End mffm_cleanup() */

