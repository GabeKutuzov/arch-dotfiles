% (c) Copyright 2013, The Rockefeller University *11303* 
% Test program for shencas package - Make gallery of images of figs
%  w/Canny edge tracing and Shen-Castan with several b values
% $Id: shentst2.m 11 2013-03-15 17:24:30Z  $
%
% V1A, 02/26/13, GNR - New script based on cit5x5.m
% V2A, 03/05/13, GNR - Show smoothed & gradient images
% V2B, 03/12/13, GNR - Make panels with Canny vs ShenCas

% Set DoDebug to turn off all image displays
DoDebug = false;

% Set DoPause to stop after each image is processed
DoPause = false;

% Number of images and image selection
NImgs = 1;
udata1 = '*C(40),I(14)'

% Set ShowRaw = true to draw raw images.
ShowRaw = true & ~DoDebug;

% Set ShowCanny true to draw Canny edges
ShowCanny = true & ~DoDebug;

% Set ShowShen true to draw ShenCastan trace
ShowShen = true & ~DoDebug;

% Set ShowHistos index number of histogram to draw (0 to omit)
ShowHistos = 0;
nhist = 200;

% Set ShowDbgis to index number of debug image to draw (0 to omit)
ShowDbgis = 0;

% Parameters for Shen-Castan
SCB = 0.575;
% hgtf, olb, ohb
SChf = [0.275 0.025 0];

% Miscellaneous setup parameters
CITPath = '/home/cdr/Sentinel/CIT101Images/';

Kops = uint32(0);          % Optional image pruning
MxEdge = 600;              % Arbitrary image edge limit
NSCB = numel(SCB);
NHT = size(SChf,1);
%NHT = 1;

% Initialize mex packages
CitMexComm = citmexset(CITPath, MxEdge, MxEdge, udata1, Kops);
SCComm = smexinit(SCB);
smexopt(SCComm, 4, ShowDbgis, ShowHistos, nhist);

if DoPause, pause on; end
for ii=1:NImgs
   [rawimg, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('C=%hu,I=%hu', jsid(1),jsid(2));
   fprintf('Image %s\n', sid);
   figure
   if ShowRaw
      subplot(1,3,1);
      imagesc(rawimg,[0 max(max(rawimg))]);
      colormap(gray)
      title(sid);
      axis off
      axis image
      end
   if ShowCanny
% N.B.  By test conducted 04/30/12, medfilt2(img) and
%     medfilt2(img, [3 3]) produce identical results.
      img = medfilt2(rawimg);
      edgimg = 120*edge(img, 'canny', []);
      subplot(1,3,2);
      colormap(gray)
      imagesc(edgimg);
      axis off
      axis image
      title('Canny');
      drawnow
      end
   for ib=1:NSCB
      bb = SCB(ib);
      smexsetb(SCComm, bb);
      if ShowDbgis, for idb = ShowDbgis
         smexhfhl(SCComm, SChf(1,1), SChf(1,2));
         smexhgtf(SCComm, SChf(1,3));
         smexopt(SCComm, 16, idb, ShowHistos, 256);
         img = smexedge(SCComm, rawimg');
         dbgi = smexdbgi(SCComm);
         figure
         imagesc(dbgi');
         colormap(gray);
         axis off
         axis image
         title(sprintf('%s, b = %.3f, db = %.0f', sid, bb, idb));
         end, end % idb loop
      for j=1:NHT
         smexhgtf(SCComm, SChf(j,1), SChf(j,2), SChf(j,3), nhist);
         img = smexedge(SCComm, rawimg');
         if ShowShen
            subplot(1,3,3);
            imagesc(img');
            colormap(gray);
            axis off
            axis image
            title(sprintf('tt=%.2f,%.2f,%.2f', ...
               SChf(j,1), SChf(j,2), SChf(j,3)));
            end
         if ShowHistos
            histo = smexhist(SCComm);
            fprintf('The %s B = %.2f histo range is %.4f to %.4f\n', ...
               sid, bb, histo(257), histo(258));
            fprintf('The gradient range is %.4f to %.4f\n', ...
               histo(259), histo(260));
            figure
            plot(histo(1:256));
            title(sprintf('%s, b = %.3f', sid, bb));
            end
         end % Threshold loop
      end % bb loop
   if DoPause, pause, end
   end % Image loop

% All done, test shutdown call
citmexset(CitMexComm, 'close');
smexclse(SCComm);
