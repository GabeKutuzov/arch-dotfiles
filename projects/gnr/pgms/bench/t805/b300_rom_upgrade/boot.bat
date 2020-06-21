set iboardsize=#C000
iserver /sb iskip.bax %1 /a 
if "%3" == "" goto nolink

iserver /ss /sc romprog.b2h /r rom /e %2 /w /l %3

goto end

:nolink

iserver /ss /sc romprog.b2h /r rom /e %2 /w 

:end


