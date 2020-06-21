/*
 *	uul.c
 *
 *	by:	Johannes Sayre
 *		Neurosciences Institute
 *
 *	use:	look for lines containing a carriage return; print
 *		the part of the line after the return, then the
 *		part before, separated by a newline.  Made especially
 *		to massage Fortran output files with underlines.
 *
 *	revisions:	11/21/89 - initial version
 */

#include <stdio.h>
#include <string.h>

#define MAXLINE	1024

main(argc, argv)
    int     argc;
    char    *argv[];
{
	char	buf[MAXLINE + 1];
	register  char	*cr_pos,
			*ep;

	register  FILE	*infile;

	if (argc == 1)
	    infile = stdin;
	else
	    if ((infile = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "%s: failure to open file %s\n", argv[0],
			argv[1]);
		exit(1);
	    }

	while (fgets(buf, MAXLINE, infile) != NULL) {
	    if ((cr_pos = strchr(buf, '\r')) != NULL) {
		if (*((ep = cr_pos + strlen(cr_pos)) - 1) != '\n') {
		    *ep++ = '\n';
		    *ep = '\0';
		}
		fputs(cr_pos + 1, stdout);
		*cr_pos = '\n';
		*(cr_pos + 1) = '\0';
	    }
	    fputs(buf, stdout);
	}

	if ( ! feof(infile)) {
	    fprintf(stderr, "%s: failure on read of file %s\n", argv[0],
		    infile == stdin ? "stdin" : argv[1]);
	    exit(1);
	}

	if (infile != stdin)
	    fclose(infile);

	return(0);
}
