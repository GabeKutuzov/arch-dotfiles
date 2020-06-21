% (c) Copyright 2012, The Rockefeller University *11303* 
% Script to test idea of running Canny edge trace separately on red,
%  green, blue channels of colored CIT images.
% $Id: cannyrgb.m 11 2013-03-15 17:24:30Z  $
% V1A, 12/04/12, GNR - New script
% ==>, 12/04/12, GNR - Last mod before committing to svn repository

% Number of images to run
NImgs = 20;

% Type diagram = 1 show all 3 colors, 2 show sum, AND
Type = 2;

% Set DoMedian = true to perform median filter before Canny trace
DoMedian = true;

% Set DoTrace = true to trace each image.
% Trace parameters are given below.
DoTrace = false;

% Parameters for "turtle" edge tracer
Wdthfac = single(1.32);    % Width as a multiple of delta
Thangle = single(50.0);    % Trace half fwd search angle (deg)
Mnwdth  = single(1.5);     % Min turtle base width (pixels)
FBRatio = single(1.8);     % Turtle front to back width ratio
GapDist = single(2.3);     % Gap distance -- knight's move
GapAng  = single(30.0);    % Gap acceptance angle
Stepfac = single(3.0);     % Multiples of width to step
Mncurv  = single(0.15);    % Min frac off-line for curved/wavy
Mxbend  = single(170.0);   % Max bend before line is split (degs)
Jcrad   = single(2.9);     % Junction search radius (pixels)
Jpsrad  = single(3.9);     % Parallel line search radius (pixels)
Jpsdr   = single(8.1);     % Parallel line distance range^2
Cutoff  = uint32(100);     % Cutoff density
MxGaps  = uint32(2);       % Max gaps accepted in one step
Nlmn0   = uint32(2);       % Min points to start a line
Nlmin   = uint32(5);       % Min points to end a line
GfVCode = uint32(hex2dec('0037ffff')); % Feature vectors

% Parameters for "harrow" image scans (currently not used)
HFwdBox = single(5.0);     % Forward box or min gap resolution
HNumAng = int32(6);        % Number of scan angles
HNumDiv = int32(18);       % Divisions at angle w/largest 's'
HCutoff = int32(100);      % Cutoff density
HMinNac = int32(2);        % Min points to terminate a gap
Hdkp    = uint32(hex2dec('00000300')); % Plot outputs
HfVCode = uint32(hex2dec('ffffffff')); % Feature vectors
%HfVCode = uint32(hex2dec('fffdb6df')); % Feature vectors

% Set Kops to sum of option values described in citmexset.c for options
% desired here.  Add 1 for keep color, 2 for clipping, 4 for immediate
% masking, 8 for deferred masking.
Kops = uint32(3);
DefMask = bitand(Kops, uint32(8));

MxEdge = 600;              % Arbitrary image edge limit
CropBord = 10;             % Crop border extension
if DefMask                 % Parameters for deferred masking
   EdgeLimsX = [MxEdge CropBord 3.1];
else
   EdgeLimsX = [MxEdge CropBord];
   end
EdgeLimsY = [MxEdge CropBord];

% Make colormap for RGB Cannys
% N.B.  Apparently, matlab only allows one colormap per figure,
% not per subplot.  So we need a global colormap.
mymap = zeros(121,3);
mymap(2:8,:) = [1.0 0.0 0.0; ...
                0.0 1.0 0.0; ...
                1.0 1.0 0.0; ...
                0.0 0.0 1.0; ...
                1.0 0.0 1.0; ...
                0.0 1.0 1.0; ...
                1.0 1.0 1.0];
mymap(121,:) = [1.0 1.0 1.0];

if DoTrace
   Kprt = uint32(hex2dec('06400000'));
   Umfnm = 'NONE';
   TurtMexComm = tmexinit(Wdthfac, Thangle, Mnwdth, FBRatio, ...
      GapDist, GapAng, Stepfac, Mncurv, Mxbend, Jcrad, Jpsrad, ...
      Jpsdr, Cutoff, MxGaps, uint32(MxEdge), uint32(MxEdge), ...
      Nlmn0, Nlmin, Kprt, Umfnm);
   end

CITPath = '/home/cdr/Sentinel/CIT101Images/';

% Acquire, process, and graph an array of random images
udata1 = '*C(1-100),I(1-$)'
%udata1 = '/C(3-100),I(3-$)'
CitMexComm = citmexset(CITPath, EdgeLimsX, EdgeLimsY, udata1, Kops);
for i=1:NImgs
   for j=1:20
      [img, isid] = citmexget(CitMexComm);
      if ndims(img) == 3, break; end
      if (j==20)
         error('More than 20 tries to get a colored image');
         end
      end
   jsid = uint16(isid);
   sid = sprintf('Image C=%hu,I=%hu', jsid(1),jsid(2));
   disp(sid);
   figure
   colormap(mymap);
   % Make separate R,G,B images
   imgm = mean(img,3);
   imgr = img(:,:,1);
   imgg = img(:,:,2);
   imgb = img(:,:,3);
   % If doing median filtering, do it now
   if DoMedian
      imgm = medfilt2(imgm);
      imgr = medfilt2(imgr);
      imgg = medfilt2(imgg);
      imgb = medfilt2(imgb);
      end
   % Do Canny filter 4 times
   edgm = 120*edge(imgm, 'canny', []);
   edgr = edge(imgr, 'canny', []);
   edgg = 2*edge(imgg, 'canny', []);
   edgb = 4*edge(imgb, 'canny', []);
   % If DefMask apply mask now
   if DefMask
      edgm = citmexpbg(CitMexComm, uint8(edgm));
      edgr = citmexpbg(CitMexComm, uint8(edgr));
      edgg = citmexpbg(CitMexComm, uint8(edgg));
      edgb = citmexpbg(CitMexComm, uint8(edgb));
      end
   % Display raw image
   subplot(2,3,1);
   image(img);
   title(sid);
   axis off
   axis image

   switch Type
   case 1                  % Original figure
      % Plot Canny traces
      subplot(2,3,2)
      image(uint8(edgm))
      title('Gray Canny')
      axis off
      axis image
      % Red
      subplot(2,3,4)
      image(uint8(edgr))
      title('Red Canny')
      axis off
      axis image
      % Green
      subplot(2,3,5)
      image(uint8(edgg))
      title('Green Canny')
      axis off
      axis image
      % Blue
      subplot(2,3,6)
      image(uint8(edgb))
      title('Blue Canny')
      axis off
      axis image
      % Red+Green+Blue
      subplot(2,3,3)
      image(uint8(edgb+edgg+edgr))
      title('Sum Canny')
      axis off
      axis image
   case 2                  % Show AND of 2,3 color traces
      % Plot Canny traces
      subplot(2,3,4)
      image(uint8(edgm))
      title('Gray Canny')
      axis off
      axis image
      % Red+Green+Blue
      sumedg = edgb+edgg+edgr;
      subplot(2,3,6)
      image(uint8(sumedg))
      title('Sum Canny')
      axis off
      axis image
      % Any two colors
      subplot(2,3,2)
      sumed2 = sumedg;
      ism = ismember(sumed2,[3 5 6 7]);
      sumed2( ism) = 7;
      sumed2(~ism) = 0;
      image(uint8(sumed2))
      title('Any 2 Canny')
      axis off
      axis image
      % Red & Green & Blue
      subplot(2,3,3)
      sumedg(sumedg ~= 7) = 0;
      image(uint8(sumedg))
      title('All 3 Canny')
      axis off
      axis image
   end % End type switch
   % Do turtle trace
   if DoTrace
      tmexnisz(TurtMexComm, uint32(size(edgm,2)), uint32(size(edgm,1)));
      TGraph = tmexturt(TurtMexComm, uint8(edgm'), sid);
      tmexfree(TurtMexComm, TGraph);
      end
   drawnow
   end

% All done, test shutdown call
citmexset(CitMexComm, 'close');
if DoTrace
   tmexclse(TurtMexComm);
   end
