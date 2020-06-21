% (c) Copyright 2011-2012, The Rockefeller University *11302* */
% Test program for citmex package - WORKS OK, 11/28/11
% $Id: citmextest.m 2 2012-01-19 21:31:23Z  $
% V1A, 11/28/11, GNR - New script, based on norbmextest.m
% V2A, 12/20/11, GNR - Add tests of clipping and masking
% ==>, 12/22/11, GNR - Last mod before committing to svn repository
% V2B, 01/19/12, GNR - Test error case '/C(1-10)*I(31-$),N(30)'

% Set 'Dodbx' to the number of the test where dbmex testing
% should begin (no figures) or 0 for no dbmex, figures on.
Dodbx = 0;
% Set Kcol = true to allow color images, false to average them
% to grayscale levels
Kcol = true;
% Set Kops to sum of options described in citmexset.c, noting
% that the low order bit must match Kcol.  Add 2 for clipping,
% 4 for masking.
Kops = uint32(Kcol+4);

CITPath = '/home/cdr/Sentinel/CIT101Images/';
if Dodbx == 0
   hRaw = figure;
   colormap('gray');
   end
pause on;

% First a straightforward test with deterministic selectors
fprintf('Test 1 -- deterministic selectors\n')
udata1 = '/C(1-4),I(3,4,7,8)'
if Dodbx == 1, dbmex on, end
CitMexComm = citmexset(CITPath, 600, 600, udata1, Kops);
for i=1:20  % Check for wraparound after 16
   [img, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('Image C=%hu,I=%hu', jsid(1),jsid(2));
   disp(sid)
   if Dodbx == 0
      figure(hRaw);
      if Kcol
         image(img);
      else
         imagesc(img,[0 max(max(img))]);
         end
      axis image;
      title(sid);
      end
   if Dodbx < 2, pause, end
   end

% Now test with named categories
% (This incidentally tests the "revise-sequence" call,
%  sequential category with random image selection,
%  batch number, and 'faces' tests case matching)
fprintf('Test 2 -- named categories\n')
udata2 = '/C(airplanes,faces,watch,wild_cat)+I(1-$),N(10)'
if Dodbx == 2, dbmex on, end
citmexset(CitMexComm, udata2);
for i=1:40
   [img, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('Image C=%hu,I=%hu', jsid(1),jsid(2));
   disp(sid)
   if Dodbx == 0
      figure(hRaw);
      if Kcol
         image(img);
      else
         imagesc(img,[0 max(max(img))]);
         end
      axis image;
      title(sid);
      end
   if Dodbx < 3, pause, end
   end

% Now test with permutations
fprintf('Test 3 -- permuted selectors\n')
udata3 = '/C(airplanes,faces,watch,wild_cat)*I(1-$),N(10)'
if Dodbx == 3, dbmex on, end
citmexset(CitMexComm, udata3);
for i=1:80
   [img, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('Image C=%hu,I=%hu', jsid(1),jsid(2));
   disp(sid)
   if Dodbx == 0
      figure(hRaw);
      if Kcol
         image(img);
      else
         imagesc(img,[0 max(max(img))]);
         end
      axis image;
      title(sid);
      end
   if Dodbx < 4, pause, end
   end

% Now a fancier test with random selection from 2 selectors
fprintf('Test 4 -- multiple probabilistic selectors\n')
udata4 = 'C(21-40)+I(1-30),N(10),P(0.25);C(81-100)*I(1-10)'
if Dodbx == 4, dbmex on, end
citmexset(CitMexComm, udata4);
for i=1:200
   [img, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('Image C=%hu,I=%hu', jsid(1),jsid(2));
   disp(sid)
   if Dodbx == 0
      figure(hRaw);
      if Kcol
         image(img);
      else
         imagesc(img,[0 max(max(img))]);
         end
      axis image;
      title(sid);
      end
   if Dodbx < 5, pause, end;
   end

% Now test with permutations where number of available
%   images is less than batch size
fprintf('Test 5 -- batch larger than number of images\n')
udata5 = '/C(1-10)*I(31-$),N(30)'
if Dodbx == 5, dbmex on, end
citmexset(CitMexComm, udata5);
for i=1:80
   [img, isid] = citmexget(CitMexComm);
   jsid = uint16(isid);
   sid = sprintf('Image C=%hu,I=%hu', jsid(1),jsid(2));
   disp(sid)
   if Dodbx == 0
      figure(hRaw);
      if Kcol
         image(img);
      else
         imagesc(img,[0 max(max(img))]);
         end
      axis image;
      title(sid);
      end
   if Dodbx < 6, pause, end
   end

% All done, test shutdown call
citmexset(CitMexComm, 'close');
pause off;

