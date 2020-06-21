/* hexd.c */
/* quick hex to decimal converter */
/* command line: hexd xx       xx is hex number */
/* w.e.g. 5/90 */

#include <stdio.h>

main(argc, argv)
int argc;
char *argv[];
{
int num;

while (--argc > 0) {
	sscanf(*++argv, "%x", &num);
	printf("%#x %d\n", num, num);  }
}
