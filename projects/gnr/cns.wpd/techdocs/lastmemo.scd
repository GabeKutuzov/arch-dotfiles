
                   Parting memo from S.C. Dolan
                             9/3/89
                    

                     CURRENT STATUS OF DARWIN3

   1) The SPEED run still does not produce the same results on the
      NCUBE as those obtained on the IBM. There is a 30-line input deck
      called 'scdbug.input' that was arrived at by reducing the SPEED
      deck. 'scdbug.input' has two repertoires. One of those repertoires
      (RU) serves as input to nothing, and has errors in its statistics.
      'scdbug.input' is on ACTEON in /home/scd/darwin.
      NOTE:Value seems OK: the values printed out with DP=V on the
           CYCLE card agree on the IBM and the NCUBE.

   2) There seems to be problems with the error detecting logic of
      the DETAIL card: I had a case in which I listed all cell numbers
      plus one out of range and the program hung.

   3) In d3echo() it is sometimes the case that sconvrt will cause a
      line of output to have a string-terminator character that will
      prematurely cut off printing of the line.  This seems to happen
      when two sconvrt calls are used to prepare a line of output such
      that one sconvrt call picks up beyond the point written by the
      other call.

   4) Probes do not work.

   5) Cross-response statistics do not work.

   6) Whether or not MIJ plot works is uncertain.


------------------------------------------------------------------------------

                    Important Files and Directories

   * * * * * * * * * Files on ACTEON * * * * * * * * * * * 

   --> /home/scd/darwin
       The most recent version of all Darwin3 source code is in
       ACTEON:~scd/darwin

   --> /home/scd/darwin  and /home/scd/darwin/overlay
       The files prepared by SCD relating to D3 in parallel are all 
       in either ACTEON:~scd/darwin or ACTEON:~scd/darwin/overlay

   --> /usr/lib/ncube/usr/src/cmd/nrun/nrun.c
       Source code for the standard NCube supplied Sun host 
       program.

   --> /usr/lib/ncube/usr/src/sample
       Directory containing subdirectories with sample programs.

   * * * * * * * Files on the MRED account SCD * * * * * * * 

   --> NCUBE NOTES A
       The file NCUBE NOTES A contains various phone numbers and
       addresses and notes having to do with NCUBE.

   --> JUL18 C A
       The file JUL18 C A contains the source code for the last
       version of the Darwin3 benchmark on the BBN Butterfly.

   --> JFIT FORTRAN A
       Routine used for calculating expected Darwin3 benchmark
       performance on a given number of nodes on a parallel machine 
       by extrapolating from known timings on other numbers of
       nodes.
       

   * * * * * * * Files on the BIG NCUBE * * * * * * * * * * * 

   --> /usr/test/bin
       The directory /usr/test/bin contains a number of shell
       scripts and programs (source and executable) written at
       NSI including:

         nexec -- Simple host program to open cube and load program.
                  User is prompted for cube dimension and name
                  of node executable. 'nexec' kicks off routine
                  that runs on host board handling printf calls
                  on the cube.

         chk_cecc, chk_uecc, chk_messerr
                  -- Shell scripts for searching for errors
                     on the nodes. See the file for details.

         tlog --  Routine used for temperature logging.

   --> /usr/scd/mandel
       The source code for SCD's mandelbrot/julia set maker is on
       the big NCUBE in /usr/scd/mandel.  There is a makefile
       in that directory.  There is a problem with the executable
       that results from the compilations in the makefile. It
       seems to be a problem with the libraries (/lib/vrtxc.lib, etc).
       We need to check which versions we have of the libraries on
       the NCUBE. (There may be a bug in the 'putem' script in
       /usr/lib/ncube/lib: it seems it should copy over vertex
       libraries but it doesn't).

   --> /usr/nsi/demo
       Copies of demos written at NSI are kept in /usr/nsi/demo.
       Neither the Darwin3 benchmark nor the mandelbrot set
       maker work now.

   --> /usr/test/bench
       This directory contains Matthew Halls NCUBE version of the
       Darwin3 benchmark. There are also some versions of Matthew code
       that were modified at NSI. There is a file called 'readme'
       that documents the contents of the directory.

------------------------------------------------------------------------------

