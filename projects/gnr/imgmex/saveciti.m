% (c) Copyright 2013, The Rockefeller University *11302* 
% Program to read in a CIT image and save as an ordinary binary file
% $Id: saveciti.m 11 2013-03-15 17:24:30Z  $
%
% V1A, 03/08/13, GNR - New script

% Number of images and image selection
NImgs = 1;
udata1 = '*C(40),I(14)'

% Name of output file
OFnm = 'ShenImg1'

% ShowRaw = true to display image before saving
ShowRaw = true;

% Miscellaneous setup parameters
CITPath = '/home/cdr/Sentinel/CIT101Images/';

Kops = uint32(0);          % Optional image pruning
MxEdge = 600;              % Arbitrary image edge limit

% Initialize citmex package
CitMexComm = citmexset(CITPath, MxEdge, MxEdge, udata1, Kops);

% Open output file
[OFid,msg] = fopen(OFnm, 'w');
if OFid < 0, error(msg), end

for ii=1:NImgs
   [rawimg, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('Image C=%hu,I=%hu', jsid(1),jsid(2));
   fprintf('%s\n', sid);
   if ShowRaw
      figure
      imagesc(rawimg,[0 max(max(rawimg))]);
      colormap(gray)
      title(sid);
      axis off
      axis image
      end
   fwrite(OFid, jsid(1), 'uint16');
   fwrite(OFid, jsid(2), 'uint16');
   fwrite(OFid, size(rawimg), 'uint32');
   fwrite(OFid, rawimg, 'uint8');
   end % Image loop

% All done, test shutdown call
citmexset(CitMexComm, 'close');
fclose(OFid);
