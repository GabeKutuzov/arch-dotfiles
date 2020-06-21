
gcc -o canny canny.c lib.c -lm
coff2exe -s c:\gnu\bin\go32.exe canny
gcc -o eval1 eval1.c lib.c -lm
coff2exe -s c:\gnu\bin\go32.exe eval1
gcc -o eval2 eval2.c lib.c -lm
coff2exe -s c:\gnu\bin\go32.exe  eval2
gcc -o gnoise gnoise.c lib.c -lm
coff2exe -s c:\gnu\bin\go32.exe gnoise
gcc -o grad1 grad1.c lib.c -lm       
coff2exe -s c:\gnu\bin\go32.exe grad1
gcc -o grad2 grad2.c lib.c -lm        
coff2exe -s c:\gnu\bin\go32.exe  grad2
gcc -o kirsch kirsch.c lib.c -lm
coff2exe -s c:\gnu\bin\go32.exe kirsch
gcc -o maketmpl maketmpl.c 
coff2exe -s c:\gnu\bin\go32.exe maketmpl
gcc -o marr marr.c lib.c -lm
coff2exe -s c:\gnu\bin\go32.exe marr
gcc -o measure measure.c lib.c -lm
coff2exe -s c:\gnu\bin\go32.exe measure
gcc -o shen shen.c lib.c -lm
coff2exe -s c:\gnu\bin\go32.exe shen
gcc -o sobel sobel.c lib.c -lm
coff2exe -s c:\gnu\bin\go32.exe sobel
