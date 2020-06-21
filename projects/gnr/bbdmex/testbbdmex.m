% (c) Copyright 2010-2015, The Rockefeller University *11304* 
% Simple script to test features of bbdmex that will be used with
% mixed MATLAB-CNS novelty detector
% $Id: testbbdmex.m 16 2017-01-13 20:36:40Z  $
%
% V1A, 11/01/10, GNR - New Program
% ==>, 11/04/10, GNR - Last mod before committing to svn repository
% V1B, 11/10/10, GNR - Add test for sense-id mechanism
% V1C, 11/19/15, GNR - Add test for passing vardefs to CNS

% Set ktest = 0 to test normal end after 10 cycles
% Set ktest = 1 to test that CNS termination is received
ktest = 0

disp('Begin bbdmex test')
disp('==>Be sure to run ''clear mex'' after test completes<==')

sdim1 = 2;     % 1st dim of sense array
sdim2 = 10;    % 2nd dim of sense array
edim = 2;      % Size of results array

% Startup CNS
testexec.def1 = '1';
testexec.def2 = '2.0678459';
BBDComm = bbdcinit('/home/cdr/src/bbdmex/testbbdmex.cns', ...
   '/home/cdr/src/bbdmex/testbbdmex.log', int32(1), testexec);
disp('   bbdcinit returned')

% Register two senses and an effector
bbdcregs(BBDComm, 'FakeSense', int32(sdim1), int32(sdim2), ...
   int32(1024));
disp('   bbdcregs returned')
bbdcrsid(BBDComm, 'IdSense', int32(2*sdim1), int32(sdim2), ...
   int32(1024));
disp('   bbdcrsid returned')
bbdcrege(BBDComm, 'Results', int32(edim));
disp('   bbdcrege returned')

% Check registration and print returned version
bbdcchk(BBDComm);
disp('   bbdcchk returned')
vtext = bbdcgetm(BBDComm);
disp(['   bbdcgetm returned >>' vtext])

% Loop over 10 or 12 cycles, expecting CNS to quit after 10
csns = cell(3);
fsns = zeros(sdim1, sdim2, 'single');
fids = zeros(2*sdim1, sdim2, 'single');
sids = zeros(2,1, 'int16');
if ktest == 0, ntest = 10, else, ntest = 12, end;
for icyc=1:ntest

% Create fake sense data and send to CNS
   fsns(1,:) = 0.0:0.025:(sdim2-1)*0.025;
   fsns(2,:) = 0.5:0.025:0.5+(sdim2-1)*0.025;
   fsns = fsns + 0.01*icyc;
   csns{1} = fsns;
   fids(1,:) = 0.0: 0.025:     (sdim2-1)*0.025;
   fids(2,:) = 0.25:0.025:0.25+(sdim2-1)*0.025;
   fids(3,:) = 0.50:0.025:0.5 +(sdim2-1)*0.025;
   fids(4,:) = 0.75:0.025:0.75+(sdim2-1)*0.025;
   fids = fids + 0.01*icyc;
   csns{2} = fids;
   sids = [int16(20-icyc), int16(icyc)];
   csns{3} = sids;
   bbdcputs(BBDComm, csns);

% Retrieve effector data and print
   [testout retcode] = bbdcgete(BBDComm);
   disp(['   For cycle ' num2str(icyc) ', CNS returned:'])
   disp(testout)

% Test for END message from CNS
   if retcode ~= 0, break, end
   end
% Done
disp(['Exited cycle loop with icyc = ' num2str(icyc)])
if (ktest ~= 0), disp('(Should be 11)'), end
disp('Completed bbdmex test')
