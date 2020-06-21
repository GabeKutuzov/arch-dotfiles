#!/usr/bin/tclsh

#----------------------------------------------------------------------#
#                              UPDZ.TCL                                #
#----------------------------------------------------------------------#
#
#  Purpose:  Synchronize a set of directories with zip files
#            contained in those directories and on a removable
#            disk.  Also synchronizes itemized ordinary files.
#            Written in tcl to work on Windows or Linux.
#
#  Synopsis: updz [-P] {d:|/d} [r] [/P]
#            where d specifies the removable disk (Windows disk letter
#            and colon or Linux mount point) where the control file and
#            files to be synchronized may be found.
#            r is an optional root point that is prepended to the
#            names in the control file.  If r is not entered, the
#            program looks for an environment variable UPDZ_HOME,
#            and if that is not found, it looks for the environment
#            variable HOME, and if that is not found, it uses the
#            current working directory as the base location.
#            "-P" or "/P" (Linux or Windows) ("P" stands for
#            "preserve") is an optional parameter which indicates
#            that the program should preserve the most recent version
#            of each file contained within a zip file based on the
#            individual file dates in the zip files.  Without "P",
#            updates are preserved only from the more recent zip file
#            of each pair.  "P" is ignored for files that are not
#            contained within zip archives.  [Note that "/P" at the
#            end of the command line can be used with Linux only if
#            the r option is also present, but with Windows the
#            forward slash indicates that P is an option switch.]
#
#  Input:    Target disk d is expected to contain a control file,
#            updz.dat, which must contain a list of path names, one
#            to a line.  These may represent directories, ordinary
#            files, or zip files.  Path names are relative to the
#            root directory of the file system where the program is
#            run.  UNIX-style slashes should be used as directory
#            separators.  A directory name followed by "-R" or "/R"
#            is copied recursively.  Lines beginning with a "#" or
#            "*" are comments.  Lines beginning with a ">" give a
#            path where the files mentioned on following lines up
#            to the next ">" line should be placed on the target
#            removable drive.
#
#  Action:   Each path name contained in the updz.dat file is
#            processed as follows:
#
#            If the path name refers to a directory, then the time
#            stamps of all files in that directory (and recursively
#            in all subdirectories if "R" is specified) are compared
#            with those in the current removable drive target.  If a
#            subdirectory in a recursive copy does not exist on the
#            removable directory, it is created.  If a file on the
#            removable drive does not exist or is older than the
#            corresponding file on the hard drive, the file on the 
#            hard drive is copied to the removable drive.  If the
#            file on the hard drive is older or does not exist, it
#            is replaced by the corresponding file from the removable
#            drive.
#
#            If the path name refers to a file that is not a zip
#            file, then the time stamps of the copies of the file
#            on the hard disk and the floppy are compared, and
#            whichever is more recent is copied over the other.
#            If the file does not exist on either drive, it is
#            copied from the other drive.
#
#            Otherwise, if the path name refers to a zip file,
#            and if "-P" is active, the program updates the con-
#            tents of the specified directory from both zip files.
#            It then freshens the zip file on the hard disk from
#            the contents of the directory.  It compares the date
#            of that zip file with the corresponding zip on the
#            removable disk.  The more recent file is copied over
#            the other, then the files in the hard disk directory
#            are updated from the more recent zip file.
#
#            If either zip file does not exist, it is created and
#            contains all the files in the directory.  Otherwise,
#            only files already in the zip archive are synched,
#            and the user must add new files manually to the zip.
#            If the directory contains no files other than the
#            named zip file, then the newer zip file replaces
#            the older one, but no files are extracted.  A log
#            of actions taken is left in %TMPDIR\updz.log
#            (Windows) or $TMPDIR/updz.log (Linux).
#
#  Warnings: This program cannot merge updates made to the same
#            file from two different sources.  It just preserves
#            the most recent version of the file it can find.
#            If "-P" is not specified, the program assumes that
#            the zip files on the hard disk do not contain any
#            files more recent than those in the directory in
#            which it is contained.
#
#  Notes:    Variables associated with the removable disk are:
#            rdm        Removable disk mount point or drive
#            rdi        Removable disk fileid returned from "open"
#            rdc        Removable disk commands read from rdi
#            rdf        Removable disk destination folder
#            rpath      Removable disk destination path
#
#  V1A, 04/01/94, GNR - New program (4DOS Batch language)
#  Rev, 12/02/96, GNR - If hard disk is a network drive, refresh
#                       local zip after extraction to freeze in
#                       current timestamp.  Handle non-zip files.
#  V2A, 03/15/06, GNR - Rewrite in tcl.  Work under Windows or Linux.
#                       Remove special mtime updating for Sun PC files.
#                       Add destination folders, globbing, recursion.

#----------------------------------------------------------------------#
#                        HandlePath procedure                          #
#                                                                      #
#  This is used to process one path specification.  It can call itself #
#  recursively if user has specified recursion.                        #
#----------------------------------------------------------------------#

   global log pflag qwin rflag unzip zip

   proc HandlePath { fspec rpath } {

      global log pflag qwin rflag unzip zip

      set dflag [file isdirectory $fspec]
      if {$dflag} {
         set fpath $fspec
         set ftail "*"
      } else {
         set fpath [file dirname $fspec]
         set ftail [file tail $fspec]
         }

#  If output is not a directory, try to create it

      if {![file isdirectory $rpath]} {
         if {[catch {file mkdir $rpath} ec]} {
            puts "Error $ec creating directory $rpath"
            exit 1
            }
         }

      cd $fpath

#  The "file" might actually be a glob pattern.  Now is the
#  time to loop over all file names implicit in the pattern.

      set glist [glob -nocomplain $ftail]
      if {"$glist" == {}} {
         puts "No files match pattern $ftail, skipping."
         puts $log "No files match pattern $ftail, skipping."
         }
      foreach fnext $glist {

#  If file is a directory, recurse if recursion set, otherwise skip.

         if {[file isdirectory $fnext]} {
            if {$rflag} { HandlePath $fpath/$fnext $rpath/$fnext
            } else      { continue }
            }

#  Check existence and modification time of both files

         set rnext $rpath/$fnext
         set xhd [file exists $fnext]
         if {$xhd} { set xhd [file mtime $fnext] }
         set xrm [file exists $rnext]
         if {$xrm} { set xrm [file mtime $rnext] }

#  Handle ordinary (non-zip) files

         if {[string toupper [file extension $fnext]] != "ZIP"} {
            if {$xhd > $xrm} {
               puts "Copying local file $fnext to $rpath"
               puts $log "Copying local file $fnext to $rpath"
               if {[catch {file copy $fnext $rpath} ec]} {
                  puts "Error $ec copying $fnext to $rpath"
                  exit 4
                  }
            } elseif {$xrm > $xhd} {
               puts "Copying file $fnext from $rpath"
               puts $log "Copying file $fnext from $rpath"
               if {[catch {file copy $rnext .} ec]} {
                  puts "Error $ec copying $rnext to $fpath"
                  exit 5
                  }
            } elseif {$xhd == 0 && $xrm == 0} {
               puts "Files $fnext do not exist--skipping"
               puts $log "Files $fnext do not exist--skipping"
            } else {
               puts "Files $fnext are same age, no copy"
               puts $log "Files $fnext are same age, no copy"
               }
            continue;
            } ;# End handling non-zip files

#  If directory has no files other than zip files, just replace
#  the older with the newer, creating either zip file if missing.

         if {$qwin} {
            set fl [exec DIR /b /a-d-r-s-h . | FIND /v ".zip"]
         } else {
            set fl [exec ls . | grep -v zip]
            }

         if {$fl == ""} {
            puts "No data files, updating zips only"
            puts $log "No data files, updating zips only"
            if {$xhd > $xrm} {
               puts "Copying local zip $fnext to $rpath"
               puts $log "Copying local zip $fnext to $rpath"
               if {[catch {file copy $fnext $rpath} ec]} {
                  puts "Error $ec copying $fnext to $rpath"
                  exit 6
                  }
            } elseif {$xrm > $xhd} {
               puts "Copying zip $fnext from $rpath"
               puts $log "Copying zip $fnext from $rpath"
               if {[catch {file copy $rnext .} ec]} {
                  puts "Error $ec copying $rnext to $fpath"
                  exit 7
                  }
            } elseif {$xhd == 0 && $xrm == 0} {
               puts "Zip files $fnext do not exist--skipping"
               puts $log "Zip files $fnext do not exist--skipping"
            } else {
               puts "Zip files $fnext are same age, no copy"
               puts $log "Zip files $fnext are same age, no copy"
               }
            continue;
            } ;# End handling zip files w/o unpacked contents

#  Do extra checking for "Preserve" protocol
#  (PKZIP code 12 and PKUNZIP code 11 indicate nothing updated)

         if {$pflag} {
            if {$xhd > 0} {
               puts "Updating $fnext from local zip"               
               puts $log "Updating $fnext from local zip"
               set rc [catch {set op [exec $unzip $fnext]} ec]
               puts "$op"
               puts $log "$op"
               if {$rc > 0 && $rc != 11} {
                  puts "Unzip error $ec, code $rc, in $fnext"
                  exit 8
                  }
               }
            if {$xrm > 0} {
               puts "Updating from zip on $rpath"
               puts $log "Updating from zip on $rpath"
               set rc [catch {set op [exec $unzip $rnext]} ec]
               puts "$op"
               puts $log "$op"
               if {$rc > 0 && $rc != 11} {
                  puts "Unzip error $ec, code $rc, in $rnext"
                  exit 9
                  }
               }
            }  ;# End P switch ("preserve") protocol

#  Perform normal protocol as documented above
#  First freshen or create the local ZIP file

         if {$xhd > 0} {
            puts "Freshening local zip $fnext"
            puts $log "Freshening local zip $fnext"
            set rc [catch {set op [exec $zip -f $fnext]} ec]
            puts "$op"
            puts $log "$op"
            if {$rc > 0 && $rc != 12} {
               puts "Zip error $ec, code $rc, in $fnext"
               exit 10
               }
         } else {
            puts "Creating local zip $fnext"
            puts $log "Creating local zip $fnext"
            set rc [catch {set op [exec $zip $fnext]} ec]
            puts "$op"
            puts $log "$op"
            if {$rc > 0} {
               puts "Zip error $ec, code $rc, creating $fnext"
               exit 11
               }
            set xhd [file mtime $fnext]
            }

#  If removable disk ZIP file exists, copy newer ZIP over older

         if {$xrm > 0} {
            if {$xhd > $xrm} {
               puts "Copying local zip $fnext to $rpath"
               puts $log "Copying local zip $fnext to $rpath"
               if {[catch {file copy $fnext $rpath} ec]} {
                  puts "Error $ec copying $fnext to $rpath"
                  exit 12
                  }
            } elseif {$xrm > $xhd} {
               puts "Updating local files from $rpath"
               puts $log "Updating local files from $rpath"
               if {[catch {file copy $rnext .} ec]} {
                  puts "Error $ec copying $rnext to $fpath"
                  exit 13
                  }
               set rc [catch {set op [exec $unzip $fnext]} ec]
               puts "$op"
               puts $log "$op"
               if {$rc > 0 && $rc != 11} {
                  puts "Unzip error $ec, code $rc, in $fnext"
                  exit 14
                  }
            } else {
               puts "Zip files are same age, no copy"
               puts $log "Zip files are same age, no copy"
               }
               
#  Otherwise, if floppy disk ZIP file does not exist, create it

         } else {
            puts "Creating zip $rpath"
            puts $log "Creating zip $rpath"
            if {[catch {file copy $fnext $rpath} ec]} {
               puts "Error $ec copying $fnext to $rpath"
               exit 15
               }
            }
         }  ;# End foreach fnext
      }  ;# End HandlePath procedure

#----------------------------------------------------------------------#
#                          Main UPDZ routine                           #
#----------------------------------------------------------------------#

#  Give a quick syntax check

   if {$argc < 1} { error {Usage: updz [-P] drive [source] [/P]} }

#  Display the current date and time and prompt the user to
#  correct it and reboot the machine if it is not correct.

   set ts [clock seconds]
   puts [clock format $ts -format "This machine thinks it is %A %D at %T"]
   puts "Reboot now to correct CMOS calendar."
   puts "Otherwise, hit Enter to continue."
   read stdin 1

#  Make a logical platform test variable

   set qwin [expr {"$tcl_platform(platform)" == "Windows"}]

#  Extract cwd

   set cwd [pwd]

#  Exit if there is no TMPDIR environment variable.
#  If it is OK, open a log file there.

   if {![info exists env(TMPDIR)]} {
      error "TMPDIR environment variable must be set!" }
   if {![file isdirectory $env(TMPDIR)]} {
      error "TMPDIR environment variable must be a directory!" }
   set log [open $env(TMPDIR)/updz.log w]

#  Parse the command-line arguments

   set pflag 0       ;# Preservation flag
   set hdr ""        ;# Hard disk root
   set rdf ""        ;# Removable disk destination folder

   set ia 0          ;# Argument list index
   set alist [split $argv " "]

   # Check for "-P"
   set a [lindex $alist $ia]
   if {[string index $a 0] == "-"} {
      if {[string toupper [string range $a 1 end]] == "P"} {
         set pflag 1
         set a [lindex $alist [incr ia]]
      } else {
         error "Unrecognized switch option $a"
         }
      }

   # Read name of removable disk
   if {[string length $a] == 0} {
      error "Removable disk designator is required" }
   if {$qwin} {
      # Removable disk designator -- Windows
      set zip   "pkzip"
      set unzip "pkunzip -n -o"
      set c1 [string toupper [string index $a 0]]
      set c2 [string toupper [string index $a 1]]
      set l1 [string length $a]
      if {$l1 == 2 && "$c1" >= "A" && "$c1" <= "Z" && "$c2" == ":"} {
         set rdm $a }
   } else {
      # Removable disk designator -- Linux
      set zip   "zip"
      set unzip "unzip -oL"
      set c1 [string toupper [string index $a 0]]
      if {"$c1" == "/"} {
         set rdm $a }
      } ;# End if Windows else
   if {![info exists rdm]} {
      error "Valid removable drive specification not found" }

   # Read optional source root designator
   set a [lindex $argv [incr ia]]
   if {"$a" != {}} {
      if {!$qwin || [string index $a 0] != "/"} {
         regsub -all {\\} $a "/" qhdr
         if {[file isdirectory $qhdr]} {
             set hdr $qhdr
             set a [lindex $argv [incr ia]]
         } else { error "Source root spec is not a directory" }
      }

   # Read optional "/P" designator
   # (N.B.  Still inside above if -- one indent level suppressed)
   if {"$a" != {}} {
      set c1 [string toupper $a]
      if {"$c1" == "/P"} {
         set pflag 1
      } else { error "Unrecognized switch option $a" }
      set a [lindex $argv [incr ia]]

   # Final error check -- now two indents suppressed
   if {"$a" != {}} {
      error {Usage: updz [-P] drive [source] [/P]}
   }}}

   # If hdr not entered, look up environment variables as documented
   if {"$hdr" == {}} {
      if {[info exists env(UPDZ_HOME)]} {
         set hdr $env(UPDZ_HOME)
      } elseif {[info exists env(HOME)]} {
         set hdr $env(HOME)
      } else {
         set hdr $cwd }
      }
   set hdr [string trimright $hdr "/"]

#  Read the UPDZ.DAT file into a command list

   if {![file exists "$rdm/updz.dat"]} {
      error "UPDZ.DAT file does not exist on $rdm" }
   set rdi [open "$rdm/updz.dat"]
   set rdc [split [read $rdi] \n]
   close $rdi

#  Begin the log file with a title.
#  Errors will not generally be copied to the log, on
#  the assumption user will immediately correct and rerun.

   set logts [clock format $ts -format "on %D at %T"]
   puts $log "UPDZ log $logts for $rdm /P=$pflag"

#  Process lines from the UPDZ.DAT file

   foreach datline $rdc {

      if {$datline == {}} {
         set c1 #
      } else {
         set datlist [split $datline " \t"]
         regsub -all {\\} [lindex $datlist 0] "/" fspec
         set c1 [string index $datline 0]
         }
      # For unknown reasons, switch gives error here
      if {$c1 == "#" || $c1 == "*"} {
         # Got a comment
         puts $log $datline
      } elseif {$c1 == ">"} {
         # Got a destination folder spec
         set rdf [string range $fspec 1 end]
         set rdf [string trim $rdf "/"]
      } else {

#  Got a regular or globbed file spec.
#  Parse directory and filespec and construct full path.
#  The directory has to exist, on hdr, but the file can be created.

         set rpath $rdm/$rdf

         if {$qwin} {
            if {[string index $fspec 1] != ":"} {
               set fspec $hdr$fspec }
         } elseif {"$c1" != "/"} { set fspec $hdr/$fspec }

         set dflag [file isdirectory $fspec]
         set fpath [expr {$dflag ? $fspec : [file dirname $fspec]}]
         if {![file isdirectory $fpath]} {
            puts "\n==>$fpath not found--skipping"
            puts $log "\n==>$fpath not found--skipping"
            continue
            }

#  Parse recursion flag

         set c2 [string toupper [lindex $datlist 1]]
         set rflag [expr {"$c2" == "-R" || "$c2" == "/R"}]
         if {$rflag && !$dflag} {
            error "Recursion specified but $fspec is not a directory" }

#  Calling HandlePath as a procedure allows recursion
   
         set pmsg "\n==>Processing $datline"
         puts $pmsg
         puts $log $pmsg
         HandlePath $fspec $rpath

         }  ;# End default case

      }  ;# End filespec foreach

#  Finish up

   cd $cwd
   puts "\nUpdate complete"
   puts $log "\nUpdate complete"
   close $log
   exit 0

