% (c) Copyright 2011, The Rockefeller University *11302*
% Plot an image in a new figure for debugging
% $Id: dbgimg.m 1 2012-01-13 20:05:02Z  $
% ==>, 12/21/11, GNR - Last mod before committing to svn repository
function dbgimg(img)
display(whos('img'));

figure
axis image
if ndims(img) == 3
   image(img);
   drawnow;
else
   imagesc(img,[0 max(max(img))]);
   drawnow;
end
