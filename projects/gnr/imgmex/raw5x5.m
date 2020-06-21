% (c) Copyright 2012, The Rockefeller University *11304* 
% Test program for citmex package - Make 5x5 images of figs
%    before and after painting background
% Q&D Test version -- just make a gallery of unpainted images
%
% V1A, 02/07/12, GNR - New script
% Rev, 08/10/12, GNR - Revise for testing batch count

% Set DoPause to stop after each figure is drawn
DoPause = true;

% Set DoSub true to make a 5x5 gallery
DoSub = false; NImg = 60;
if (DoSub), NImg = 25; end

% Set Kops to sum of options described in citmexset.c, noting
% that the low order bit must match Kcol.  Add 2 for clipping,
% 4 for masking.
Kops = uint32(1);

MxEdge = 600;              % Arbitrary image edge limit

CITPath = '/home/cdr/Sentinel/CIT101Images/';

% Figure set up
hRaw = figure;
if DoPause, pause on; end

% Acquire and graph a sequence or 5x5 array of random images
udata1 = 'C(1-80)+I(1-30),N(5)'
CitMexComm = citmexset(CITPath, MxEdge, MxEdge, udata1, Kops);
for i=1:NImg
   [img, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('Image C=%hu,I=%hu', jsid(1),jsid(2));
   disp(sid)
   figure(hRaw);
   if DoSub, subplot(5,5,i); end
   if ndims(img) == 3
      image(img);
      colormap('default');
   else
      imagesc(img,[0 max(max(img))]);
      colormap(gray)
      end
   axis off
   axis image
   if DoPause, pause, end
   end

% All done, test shutdown call
citmexset(CitMexComm, 'close');
