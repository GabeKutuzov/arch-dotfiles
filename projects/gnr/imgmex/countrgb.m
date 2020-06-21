% (c) Copyright 2012, The Rockefeller University *11301*
% Read all the CIT images and count how many are color vs. monochrome
% V1A, 12/03/12, GNR - New script

Kops = uint32(1);
MxEdge = 600;              % Arbitrary image edge limit
NImg = 8677;

CITPath = '/home/cdr/Sentinel/CIT101Images/';

nColor = 0;
nMono  = 0;

% Acquire and graph a sequence or 5x5 array of random images
udata1 = '/C(1-101),I(1-$)'
CitMexComm = citmexset(CITPath, MxEdge, MxEdge, udata1, Kops);
for i=1:NImg
   [img, isid] = citmexget(CitMexComm);
   if ndims(img) == 3, nColor = nColor + 1;
   else,               nMono  = nMono  + 1; end
   end

% All done, report and shutdown
fprintf('Image color type count completed\n');
fprintf('   Found %d color, %d mono images\n', nColor, nMono);
citmexset(CitMexComm, 'close');
