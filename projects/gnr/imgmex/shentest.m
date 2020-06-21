% (c) Copyright 2013, The Rockefeller University *11303* 
% Test program for shencas package - Make gallery of images of figs
%  w/Canny edge tracing and Shen-Castan with several b values
% $Id: shentest.m 11 2013-03-15 17:24:30Z  $
%
% V1A, 02/26/13, GNR - New script based on cit5x5.m
% V2A, 03/11/13, GNR - Remove smexhfhl scan, add smexhgtf scan

% Set DoDebug to turn off all image displays
DoDebug = false;

% Set diary file to make a log file
DoDiary = false;
if DoDiary
   delete('ShenCasBase.log');
   diary('ShenCasBase.log');
   end
diary off

% Set DoPause to stop after each image is processed
DoPause = false;

% Number of images and image selection
NImgs = 20;
udata1 = '*C(1-100),I(1-$)'

% Set ShowRaw = true to draw raw images.
ShowRaw = true & ~DoDebug;

% Set ShowCanny true to draw Canny edges
ShowCanny = true & ~DoDebug;

% Set ShowShen true to draw ShenCastan trace
ShowShen = true & ~DoDebug;

% Set ShowHistos index number of histogram to draw (0 to omit)
ShowHistos = 4;
nhist = 200;               % Number of bins

% Set ShowDbgis to index number of debug image to draw (0 to omit)
ShowDbgis = 4;

% Parameters for Shen-Castan
SCB = 0.575;
SCW = 5;
% Parameter sets for smexhgtf calls
SChf = [0.275 0.025 0; 0.275 0.05 0; 0.25 0.025 0; 0.25 0.05 0];

% Miscellaneous setup parameters
CITPath = '/home/cdr/Sentinel/CIT101Images/';

Kops = uint32(0);          % Optional image pruning
MxEdge = 600;              % Arbitrary image edge limit
NSCB = numel(SCB);
NWin = numel(SCW);
NHT = size(SChf,1);

% Initialize mex packages
CitMexComm = citmexset(CITPath, MxEdge, MxEdge, udata1, Kops);
SCComm = smexinit(SCB);
smexopt(SCComm, 4, ShowDbgis, ShowHistos, nhist);

if DoPause, pause on; end
for ii=1:NImgs
   [rawimg, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('C=%hu,I=%hu', jsid(1),jsid(2));
   if DoDiary, diary on, end
   fprintf('Image %s\n', sid);
   diary off
   figure
   if ShowRaw
      subplot(2,4,1);
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
      subplot(2,4,2);
      colormap(gray)
      imagesc(edgimg);
      axis off
      axis image
      title('Canny');
      end
   for ib=1:NSCB
      bb = SCB(ib);
      smexsetb(SCComm, bb);
      for iw=1:NWin
         ww = SCW(iw);
         smexwin(SCComm, uint32(ww));
         for j=1:NHT
            smexhgtf(SCComm, SChf(j,1), SChf(j,2), SChf(j,3), nhist);
            img = smexedge(SCComm, rawimg');
            if ShowShen
               subplot(2,4,4+j);
               imagesc(img');
               colormap(gray);
               axis off
               axis image
               title(sprintf('tt=%.2f,%.2f,%.2f', ...
                  SChf(j,1), SChf(j,2), SChf(j,3)));
               end
            end % Threshold loop
         if ShowDbgis
            dbgi = smexdbgi(SCComm);
            if ~DoDebug
               subplot(2,4,3);
               imagesc(dbgi');
               colormap(gray);
               axis off
               axis image
               title ('Gradient');
%               title(sprintf('%s, b = %.2f, w = %.0f', sid, bb, ww));
               end
            end
         if ShowHistos
            histo = smexhist(SCComm);
            hmin = histo(nhist+1);
            hmax = histo(nhist+2);
            delh = hmax - hmin;
            if delh ~= 0, hscl = (nhist - 0.01)/delh;
            else,         hscl = 1; end
            htlo = (histo(nhist+3) - hmin)*hscl;
            hthi = (histo(nhist+4) - hmin)*hscl;
            httf = (SChf(1,3)*hmax - hmin)*hscl;
            htht = max(histo(1:nhist));
            if DoDiary, diary on, end
            fprintf('The %s B = %.3f histo range is %.4f to %.4f\n', ...
               sid, bb, hmin, hmax);
            fprintf('The gradient thresholds are %.4f and %.4f\n', ...
               histo(nhist+3), histo(nhist+4));
            diary off
            if ~DoDebug
               subplot(2,4,4);
               bar(histo(1:nhist));
               yloc = [0.8*htht htht];
               line([htlo htlo],yloc,'Color','y');
               line([hthi hthi],yloc,'Color',[1 0.5 0]);
               title(sprintf('hgtf=%.2f,olb=%.2f', ...
                  SChf(4,1), SChf(4,2)));
               end
            end
         end % iw loop
      end % bb loop
   if DoPause, pause, end
   end % Image loop

% All done, test shutdown call
citmexset(CitMexComm, 'close');
smexclse(SCComm);
