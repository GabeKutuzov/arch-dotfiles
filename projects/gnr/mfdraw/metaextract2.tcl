#!/usr/bin/tclsh8.4
#!/home/smarx/tcl8.4/bin/tclsh
# usage: metaextract.tcl filename frommframe toframe
#        metaextract.tcl filename fromframe
#        when only fromframe is given then toframe = fromframe
if {$argc < 2 || $argc > 3} {
    puts "Error: improper usage: supply as args metafile name followed by from frame and optionally a to frame."
    exit
}
set filename [lindex $argv 0]
set getframenumlow [lindex $argv 1]
if { $argc == 3} {
    set getframenumhi  [lindex $argv 2]
} else {
    set getframenumhi $getframenumlow
}
puts "input file = $filename with low frame = $getframenumlow and high frame = $getframenumhi"
set infile [open $filename r]
set number_commands 0
set number_frames   0
set outfile [open temp_$filename w];
# first get meta file header, that is, everything up to first command
set headercount 0;
while { [gets $infile line] >= 0 } {
    incr headercount
    puts $outfile $line;   # output header here
    if { $headercount == 3 } {
	break
    } 
}
if { $headercount < 3 } {
        puts "error - input has less than 3 header lines"
	exit
}
# get all frames and their commands but output only getframenum commands
while { [gets $infile line] >= 0 } {
    if { [regexp {^\]} $line] } {
        puts $outfile "]"
	break; #found end of meta file
    }
    if { [regexp {^\[} $line] } {
	incr number_frames
    } 
    incr number_commands; # note a frame is also a command
    if { ($number_frames <= $getframenumhi) && ($number_frames >= $getframenumlow) } {
	puts $outfile $line
    }  
}
close $infile
close $outfile
exec ~/mfdraw/mfdraw temp_$filename
#puts "input file $filename has $number_commands commands and $number_frames frames"
#!/home/smarx/tcl8.4/bin/tclsh
