% (c) Copyright 2011, The Rockefeller University *11302*
% Test program for norbmex package - WORK OK, 03/18/11
% $Id: norbmextest.m 1 2012-01-13 20:05:02Z  $
% V1A, 03/16/11, GNR - New script
% V2A, 12/01/11, GNR - Move image transpose to norbmexget()
% ==>, 12/01/11, GNR - Last mod before committing to svn repository

NORBPath = '/home/cdr/Sentinel/NORBImages/xxx';
figure
colormap('gray');
pause on;

% First a straightforward test with deterministic selectors
udata1 = '/C(0-3),I(3,4,7,8),A(0,6),E(0,3,7),L(3),V(1)'
%dbmex on
NorbMexComm = norbmexset(NORBPath, 64, 64, 16, 16, udata1);
for i=1:96
   [img, isid] = norbmexget(NorbMexComm);
   jsid = uint16(isid);
   imagesc(img,[0 max(max(img))]);
   sid = sprintf('C=%hu,I=%hu,A=%hu,E=%hu,L=%hu,V=%hu',...
      jsid(1),jsid(2),jsid(3),jsid(4),jsid(5),jsid(6));
   text(0, 70, sid);
   pause;
   end

% Now a fancier test with random selection from 2 selectors
% (This incidentally tests the "revise-sequence" call
udata2 = ['C(0,1),I(0-9)/A(3,12),E(3),L(0,2),V(0),P(0.25);',...
          'C(3,4),I(0-9),E(0-8),V(0,1)/A(0,9),L(1,3)']
%dbmex on
norbmexset(NorbMexComm, udata2);
for i=1:200
   [img, isid] = norbmexget(NorbMexComm);
   jsid = uint16(isid);
   imagesc(img,[0 max(max(img))]);
   sid = sprintf('C=%hu,I=%hu,A=%hu,E=%hu,L=%hu,V=%hu',...
      jsid(1),jsid(2),jsid(3),jsid(4),jsid(5),jsid(6));
   text(0, 70, sid);
%   display(sid);   % No-grahics version
   pause;
   end

% All done, test shutdown call
norbmexset(NorbMexComm);
pause off;

