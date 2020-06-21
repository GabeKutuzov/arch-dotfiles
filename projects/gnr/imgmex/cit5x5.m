% (c) Copyright 2011-2013, The Rockefeller University *11303* 
% Test program for citmex package - Make gallery of images of figs
%    before and after painting background (originally always 5x5)
% $Id: cit5x5.m 11 2013-03-15 17:24:30Z  $
% V1A, 12/22/11, GNR - New script--I seem to have lost an earlier one.
% V2A, 01/03/12, GNR - Add turtle trace graphs
% ==>, 01/04/12, GNR - Last mod before committing to svn repository
% V3A, 01/30/12, GNR - Add opening-closing mods before canny
% V3B, 02/13/12, GNR - Add median filter options
% V3C, 03/06/12, GNR - Add harrow scan options
% V3D, 03/08/12, GNR - Add ability to display feature vectors
% Rev, 11/09/12, GNR - Add arbitrary 10 pix border around crop
% Rev, 11/21/12, GNR - Add deferred background painting,
%                      remove Median55, better Show controls
% Rev, 11/28/12, GNR - Add ability to save features in a log file
% V3E, 01/02/13, GNR - Add test of Edge-Likelihood-Index
% Rev, 01/04/13, GNR - Add printing of individual eli components
% V4A, 03/19/13, GNR - Add Shen-Castan edge detection

% Set DoDebug to turn off all image displays
DoDebug = false;

% Set diary file to make a log file
DoDiary = false;
if DoDiary
   delete('ELI-Medianp75qhr75.log');
   diary('ELI-Medianp75qhr75.log');
   end
diary off

% Set ShowRaw = true to draw raw images.
ShowRaw = false & ~DoDebug;

% Set kEdge = EdgeCanny or EdgeShen to perform Canny or ShenCastan
%  edge detection.  One of these is required if DoTrace or DoHarr
%  is requested.
EdgeNone   = 0;
EdgeCanny  = 1;
EdgeShen   = 2;
kEdge = EdgeShen;
ShowSCGrad = kEdge == EdgeShen & ~DoDebug & true;

% Set DoTrace = true to trace each image.
% Trace parameters are given below.
DoTrace = true;
ShowGfV = false & DoTrace;
SaveGfV = false & DoTrace;

% Set DoELI = UseRaw, UseMedian, or UseShen to compute & display ELI
%  for each image using specified data for gradient calculation
NoELI     = 0;
UseRaw    = 1;
UseMedian = 2;
UseShen   = 3;
DoELI = UseShen;

% Set DoHarr = true to harrow-scan each image.
% Harrow scan parameters are given below.
DoHarr  = false;
ShowHfV = false & DoHarr;
SaveHfV = false & DoHarr;

% Set ListTGraph = true to call tmexlist (print lines & junctions)
ListTGraph = false;

% Set MakeMF = true to save metafile and Tmfnm to assign name
MakeMF  = true;
Tmfnm = 'ShenTrace.mf'

% Set DoPause to stop after each figure is drawn
DoPause = false;

% Set DoSub to make a gallery of NImgs images
NImgs = 32;
DoSub = false;

% Parameters for "turtle" edge tracer
Wdthfac = single(1.32);    % Width as a multiple of delta
Thangle = single(50.0);    % Trace half fwd search angle (deg)
Mnwdth  = single(1.5);     % Min turtle base width (pixels)
Mxwdth  = 3.5;             % Max width (a Matlab double)
FBRatio = single(1.8);     % Turtle front to back width ratio
GapDist = single(2.3);     % Gap distance -- knight's move
GapAng  = single(30.0);    % Gap acceptance angle
Stepfac = single(3.0);     % Multiples of width to step
Mncurv  = single(0.15);    % Min frac off-line for curved/wavy
Mxbend  = single(170.0);   % Max bend before line is split (degs)
%Jcrad   = single(2.9);     % Junction search radius (pixels)
Jcrad   = single(2.5);     % Junction search radius (pixels)
Jpsrad  = single(3.9);     % Parallel line search radius (pixels)
Jpsdr   = single(8.1);     % Parallel line distance range^2
Cutoff  = uint32(100);     % Cutoff density
MxGaps  = uint32(2);       % Max gaps accepted in one step
Nlmn0   = uint32(2);       % Min points to start a line
Nlmin   = uint32(5);       % Min points to end a line
GfVCode = uint32(hex2dec('0037ffff')); % Feature vectors
%GfVCode = uint32(hex2dec('0037fecf')); % Feature vectors

% Parameters for Shen-Castan edge detection (non-defaults)
ShenB   = 0.575;           % Smoothing exponent
ShenHgtf= 0.275;           % Hysteresis gradient threshold factor
ShenOlb = 0.025;           % Offset of low threshold bin
ShenOhb = 0;               % Offset of high threshold bin
ShenNhist = 200;           % Number of bins in grad histogram

% Parameters for "harrow" image scans
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
Kops = uint32(10);
DefMask = bitand(Kops, uint32(8));

% Set DoMedian, DoOpen, DoClose, SE to test filtering ideas
% See M. Roushdy, GVIP J. 6 (1006)
DoMedian = false;          % Apply median filter
DoOpen = false;
DoClose = false;
SE = strel('octagon', 6);
% Set ShowFilt = true to draw filtered (median, open, close) images.
ShowFilt = (DoMedian | DoOpen | DoClose) & ~DoDebug & false;

% Set ShowEdges true to draw Canny or Shen edges
ShowEdges = kEdge ~= EdgeNone & ~DoDebug & false;

MxEdge = 600;              % Arbitrary image edge limit
CropBord = 10;             % Crop border extension
EdgeLimsX = [MxEdge CropBord 3.1];
EdgeLimsY = [MxEdge CropBord];
%EdgeLimsX = MxEdge;        % For test of old cropping
%EdgeLimsY = MxEdge;

if MakeMF, DoTrace = true; end
if DoTrace | DoHarr
   if MakeMF
      Kprt = uint32(hex2dec('47000000'));
      KEpl = uint32(hex2dec('C0000000'));
      Umfnm = Tmfnm;
   else
      Kprt = uint32(hex2dec('06000000'));
      KEpl = uint32(hex2dec('80000000'));
      Umfnm = 'NONE';
      end
   TurtMexComm = tmexinit(Wdthfac, Thangle, Mnwdth, FBRatio, ...
      GapDist, GapAng, Stepfac, Mncurv, Mxbend, Jcrad, Jpsrad, ...
      Jpsdr, Cutoff, MxGaps, uint32(MxEdge), uint32(MxEdge), ...
      Nlmn0, Nlmin, Kprt, Umfnm);
   tmexnmxw(TurtMexComm, Mxwdth);
   end
if kEdge == EdgeShen
   SCComm = smexinit(ShenB);
   smexhgtf(SCComm, ShenHgtf, ShenOlb, ShenOhb, ShenNhist);
   smexopt(SCComm, 20, 6, 4, ShenNhist);
   end
if DoELI
   tmexelii(TurtMexComm, -0.02, 0.75, 0.5, 0.75);
   end
if DoHarr
   hmexinit(TurtMexComm, HFwdBox, HNumAng, HNumDiv, HCutoff, ...
      HMinNac, Hdkp);
   end

CITPath = '/home/cdr/Sentinel/CIT101Images/';

% Figure set up
if DoSub
   if ShowRaw, hRaw = figure; end
   if ShowFilt, hFilt = figure; end
   if ShowEdges, hEdge = figure; end
   if ShowSCGrad, hSCGrad = figure; end
   end
SubRows = floor(sqrt(NImgs));
SubCols = ceil(NImgs/SubRows);

if DoPause, pause on; end

% Acquire, process, and graph a 5x5 array of random images
udata1 = '*C(1-100)*I(1-$),N(4)'
%udata1 = '/C(3-100),I(3-$)'
CitMexComm = citmexset(CITPath, EdgeLimsX, EdgeLimsY, udata1, Kops);
for i=1:NImgs
   [rawimg, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('Image C=%hu,I=%hu', jsid(1),jsid(2));
   if DoDiary, diary on, end
   fprintf('%s\n', sid);
   diary off
   if ShowRaw
      if DoSub
         figure(hRaw);
         subplot(SubRows,SubCols,i);
      else
         figure
         end
      if ndims(rawimg) == 3
         image(rawimg);
      else
         imagesc(rawimg,[0 max(max(rawimg))]);
         colormap(gray)
         end
      title(sid);
      axis image
      end
   if ndims(rawimg) == 3
      rawimg = mean(rawimg,3);
      end
   smoimg = rawimg;
   if DoMedian
% N.B.  By test conducted 04/30/12, medfilt2(img) and
%     medfilt2(img, [3 3]) produce identical results.
      smoimg = medfilt2(img);
      end
   if DoOpen
      smoimg = imopen(img,SE);
      end
   if DoClose
      smoimg = imclose(img,SE);
      end
   tmexnisz(TurtMexComm, ...
      uint32(size(rawimg,2)), uint32(size(rawimg,1)));

   switch kEdge
   case EdgeCanny
      edgimg = 120*edge(smoimg, 'canny', []);
   case EdgeShen
      edgimg = smexedge(SCComm, rawimg')';
      smoimage = smexdbgi(SCComm, 2)';
      if ShowSCGrad
         dbgi = smexdbgi(SCComm, 4)';
         if DoSub
            figure(hSCGrad);
            subplot(SubRows,SubCols,i);
         else
            figure
            end
         colormap(gray);
         imagesc(dbgi);
         axis image
         drawnow
         end % ShowSCGrad
   end % kEdge switch

   if ShowFilt
      if DoSub
         figure(hFilt);
         subplot(SubRows,SubCols,i);
      else
         figure
         end
      colormap(gray)
      imagesc(smoimg);
      axis image
      drawnow
      end
   if kEdge ~= EdgeNone
      if DefMask
         edgimg = citmexpbg(CitMexComm, uint8(edgimg));
         end
      if ShowEdges
         if DoSub
            figure(hEdge);
            subplot(SubRows,SubCols,i);
         else
            figure
            end
         colormap(gray)
         imagesc(edgimg);
         axis image
         drawnow
         end
      end
   if DoPause, pause, end
   if DoTrace | DoHarr
      edgimg = uint8(edgimg');
      if DoTrace
         if DoDiary, diary on, end
         TGraph = tmexturt(TurtMexComm, edgimg, sid);
         if ListTGraph & DoDebug
            tmexlist(TGraph, sid, false);
            end
         diary off
      else
         TGraph = [];
         end
      if DoELI
         if DoELI == UseRaw
            [mxeli, pqr] = tmexelia(TurtMexComm, uint8(rawimg'));
         else
            [mxeli, pqr] = tmexelia(TurtMexComm, uint8(smoimg'));
            end
         tmexplot(TurtMexComm, KEpl, uint32(0));
         nlines = size(pqr,2);
         if DoDiary, diary on, end
         fprintf('The max ELI is %f\n', mxeli);
         fprintf('   Line   grad    leng    kurv    eli\n');
         for il = 1:nlines
            fprintf('%7d%7.3f%8.3f%8.4f%7.3f\n', pqr(:,il));
            end
         diary off
         end
      if DoHarr
         TGraph = hmexharr(TurtMexComm, TGraph, edgimg, sid);
         end
      if ListTGraph
         if DoDiary, diary on, end
         tmexlist(TGraph, sid, false);
         diary off
         end
      if ShowGfV | SaveGfV
         GfV = tmexfeav(TGraph, GfVCode);
         if DoDiary && SaveGfV, diary on, end
         fprintf('Graph trace features for %s\n', sid);
         fprintf(['  %7.3f%7.3f%7.3f%7.3f%7.3f%7.3f%7.3f' ...
            '%7.3f%7.3f%7.3f\n'], double(GfV));
         fprintf('\n');
         diary off
         end
      if ShowHfV | SaveHfV
         HfV = hmexfeav(TGraph, HfVCode);
         if DoDiary && SaveHfV, diary on, end
         fprintf('Harrow scan features for %s\n', sid);
         fprintf(['  %7.3f%7.3f%7.3f%7.3f%7.3f%7.3f%7.3f' ...
            '%7.3f%7.3f%7.3f\n'], double(HfV));
         fprintf('\n');
         diary off
         end
      tmexfree(TurtMexComm, TGraph);
      end
   if DoPause, pause, end
   end

% All done, test shutdown call
citmexset(CitMexComm, 'close');
if DoTrace | DoHarr
   tmexclse(TurtMexComm);
   end
if kEdge == EdgeShen, smexclse(SCComm); end
