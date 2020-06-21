% Multidimensional Kolmogorov-Smirnov (MDKS) statistic test
% Variant to test ank version of mdks
% (c) The Rockefeller University, 2015
% $Id: mdkststank.m 7 2015-08-20 20:11:59Z  $

function [] = mdkststank()

%  This software was developed in the Laboratory of Biological
% Modelling at The Rockefeller University by George N. Reeke

%  This file tests the MDKS code by running it alongside a brute-
% force mex-file version of the test on specified data
% Only double precision and only equal sizes of the two data sets
%   are tested.
% Test results and execution times are reported.  The number of
% repetitions of each test may be set to obtain reasonable run times.

% LICENSE:
%  This software is distributed under GPL, version 2.
% This program is free software; you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation; either version 2 of the License, or (at
% your option) any later version.  Accordingly, this program is distri-
% buted in the hope that it will be useful, but WITHOUT ANY WARRANTY;
% without even the implied warranty of MERCHANTABILITY or FITNESS FOR
% A PARTICULAR PURPOSE.  See the GNU General Public License for more
% details.  You should have received a copy of the GNU General Public
% License along with this program.
% If not, see <http://www.gnu.org/licenses/>.

% Which methods to test (1 to perform, 0 to skip)
docbf = 0;        % C brute force
dooldks = 1;
doanks = 1;       % ank ribbon version
% Each variable here should be the 'g2' value for that method
%  to run it (see comments) or 0 to skip it.
domdks =  0;      %  1 = Binary-tree Fasano & Franceschini
dormks =  0;      %  2 = Ribbon approximation method
doroks =  0;      %  4 = Ribbon method with approx double overlaps
doepks =  0;      %  8 = Equi-partition overlap bricks
docgks =  0;      % 16 = Center-of-gravity overlap bricks

%% User-settable parameters defining the random distributions
% Mxreps = 500;           % Maximum repeats to do for any case
%Mxreps = 100;           % Maximum repeats to do for any case
Mxreps = 2;           % Maximum repeats to do for any case
Ulow = 0;               % Low end of uniform distribution
Uhigh = 1;              % High end of uniform distribution
Nmean = 20;             % Mean of normal distribution
Nsigma = 5;             % Sigma of normal distribution
Gk = 1.5;               % Shape of gamma distribution
Gscl = 10;              % Scale of gamma distribution

%% Do the runs
MDKSCOM = [];
MDKSDP1 = [];
BFKSCOM = [];
MDKSOLD = [];
MDKSOLDDP1 = [];
MDKSANK = [];
MDKSANKDP1 = [];
RNames = { 'uniform'; 'normal'; 'gamma' };
g2 = domdks + dormks + doroks + doepks + docgks;
for k = 2
   % Running n in this order tests ability of mdskallo to
   % reallocate to higher or lower work area sizes.
%   for n = [200 25000 5000 1000]
%   for n = [200 1000 5000]
   for n = 200
      % Even if MDKSCOM not needed, this gives us the Divs
      MDKSCOM = mdksallod(MDKSCOM, [], g2, n, n, k);
      [DivsX1, DivsX2] = mdksdivs(MDKSCOM);
      MDKSDP1 = mdksallod(MDKSDP1, DivsX1+1, g2, n, n, k);
      [NewDX1, NewDX2] = mdksdivs(MDKSDP1);
      if docbf, BFKSCOM = bfksallod(BFKSCOM, n, n, k); end
      if dooldks
         MDKSOLD = oldmdksallod(MDKSOLD, DivsX1, [], n, n, k);
         MDKSOLDDP1 = oldmdksallod(MDKSOLDDP1, DivsX1+1, [], n, n, k);
         end
      if doanks
         MDKSANK = ank_mdksallod(MDKSANK, DivsX1, [], n, n, k);
         MDKSANKDP1 = ank_mdksallod(MDKSANKDP1, DivsX1+1, [], n, n, k);
         end
      nreps = floor(1000000/(k*n));
      if nreps < 1, nreps = 1; end
      if nreps > Mxreps, nreps = Mxreps; end
      % Generate data for reuse later with new depth
      nr = n*nreps;
      X1R3 = cell(1,3);
      X2R3 = cell(1,3);
      X1R3{1} = random('unif',Ulow,Uhigh,k,nr);
      X2R3{1} = random('unif',Ulow,Uhigh,k,nr);
      X1R3{2} = random('norm',Nmean,Nsigma,k,nr);
      X2R3{2} = random('norm',Nmean,Nsigma,k,nr);
      X1R3{3} = random('gam',Gk,Gscl,k,nr);
      X2R3{3} = random('gam',Gk,Gscl,k,nr);

      for r = 1:3
         if docbf
            bfks1 = zeros(nreps,1); bfks2 = zeros(nreps,1);
            tbf1s = 0; tbf2s = 0;
            end
         if dooldks
            oldks1 = zeros(nreps,1); oldks2 = zeros(nreps,1);
            told1s = 0; told2s = 0;
            end
         if doanks
            anks1 = zeros(nreps,1); anks2 = zeros(nreps,1);
            tank1s = 0; tank2s = 0;
            end
         if domdks
            mdks1 = zeros(nreps,1); mdks2 = zeros(nreps,1);
            tmd1s = 0; tmd2s = 0;
            end
         if dormks
            rmks1 = zeros(nreps,1); rmks2 = zeros(nreps,1);
            trm1s = 0; trm2s = 0;
            end
         if doroks
            roks1 = zeros(nreps,1); roks2 = zeros(nreps,1);
            tro1s = 0; tro2s = 0;
            end
         if doepks
            epks1 = zeros(nreps,1); epks2 = zeros(nreps,1);
            tep1s = 0; tep2s = 0;
            end
         if docgks
            cgks1 = zeros(nreps,1); cgks2 = zeros(nreps,1);
            tcg1s = 0; tcg2s = 0;
            end

%% Single-sided, default depth

         for rep = 1:nreps
            nr2 = n*rep; nr1 = nr2 - n + 1;
            X1 = X1R3{r}(:,nr1:nr2);
            X2 = X2R3{r}(:,nr1:nr2);

            % brute-force mex-function
            if docbf
               tic
               bfks1(rep,1) = bfks1sd(BFKSCOM, X1, X2);
               tbf1s = tbf1s + toc;
               end

            % Old brick mex-function (two sets of quads)
            if dooldks
               tic
               oldks1(rep,1) = oldmdks1sd(MDKSOLD, X1, X2);
               told1s = told1s + toc;
               end

            % Old ank ribbon mex-function
            if doanks
               tic
               anks1(rep,1) = ank_mdks1sd(MDKSANK, X1, X2);
               tank1s = tank1s + toc;
               end

            % brick mex-function method
            if domdks
               tic
               mdks1(rep,1) = mdks1sd(MDKSCOM, X1, X2);
               tmd1s = tmd1s + toc;
               end

            % Ribbon method mex function
            if dormks && k == 2
               tic
               rmks1(rep,1) = rmks1sd(MDKSCOM, X1, X2);
               trm1s = trm1s + toc;
               end

            % Ribbon method with approx double overlaps
            if doroks && k == 2
               tic    
               roks1(rep,1) = roks1sd(MDKSCOM, X1, X2);
               tro1s = tro1s + toc;
               end

            % Equi-partition approx method
            if doepks
               tic
               epks1(rep,1) = epks1sd(MDKSCOM, X1, X2);
               tep1s = tep1s + toc;
               end

            % Center-of-gravity approx method
            if docgks
               tic
               cgks1(rep,1) = cgks1sd(MDKSCOM, X1, X2);
               tcg1s = tcg1s + toc;
               end

            end % Loop over reps

%% Report averages and times

         fprintf('\nFor k = %d, n = %d, %s distribution, divs = (%d,%d)\n', ...
            k, n, RNames{r}, DivsX1(1), DivsX2(1))

         if docbf
            fprintf(['   Brute-force mex mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(bfks1), std(bfks1), tbf1s/nreps);
            end
         if dooldks
            fprintf(['   Old brick method mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(oldks1), std(oldks1), told1s/nreps);
            end
         if doanks
            fprintf(['   Ank ribbon method mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(anks1), std(anks1), tank1s/nreps);
            end
         if domdks
            fprintf(['   Brick method mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(mdks1), std(mdks1), tmd1s/nreps);
            end
         if dormks
            fprintf(['   Ribbon method mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(rmks1), std(rmks1), trm1s/nreps);
            end
         if doroks
            fprintf(['   Ribbon+Overlap method mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(roks1), std(roks1), tro1s/nreps);
            end
         if doepks
            fprintf(['   Equi-partition method mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(epks1), std(epks1), tep1s/nreps);
            end
         if docgks
            fprintf(['   Center-gravity method mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(cgks1), std(cgks1), tcg1s/nreps);
            end

%% Increase depth by one

         for rep = 1:nreps
            nr2 = n*rep; nr1 = nr2 - n + 1;
            X1 = X1R3{r}(:,nr1:nr2);
            X2 = X2R3{r}(:,nr1:nr2);

            % Old brick mex-function method
            if doanks
               tic
               anks2(rep,1) = ank_mdks1sd(MDKSANKDP1, X1, X2);
               tank2s = tank2s + toc;
               end

            % brick mex-function method
            if domdks
               tic
               mdks2(rep,1) = mdks1sd(MDKSDP1, X1, X2);
               tmd2s = tmd2s + toc;
               end

            % Ribbon method mex function
            if dormks && k == 2
               tic
               rmks2(rep,1) = rmks1sd(MDKSDP1, X1, X2);
               trm2s = trm2s + toc;
               end

            % Ribbon method with approx double overlaps
            if doroks && k == 2
               tic    
               roks2(rep,1) = roks1sd(MDKSDP1, X1, X2);
               tro2s = tro2s + toc;
               end

            % Equi-partition approx method
            if doepks
               tic
               epks2(rep,1) = epks1sd(MDKSDP1, X1, X2);
               tep2s = tep2s + toc;
               end

            % Center-of-gravity approx method
            if docgks
               tic
               cgks2(rep,1) = cgks1sd(MDKSDP1, X1, X2);
               tcg2s = tcg2s + toc;
               end

            end % Loop over reps

         fprintf('\nFor k = %d, n = %d, %s distribution, divs = (%d,%d)\n', ...
            k, n, RNames{r}, NewDX1(1), NewDX2(1))

         if dooldks
            fprintf(['   Old brick method mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(oldks2), std(oldks2), told2s/nreps);
            end
         if doanks
            fprintf(['   Ank ribbon method mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(anks2), std(anks2), tank2s/nreps);
            end
         if domdks
            fprintf(['   Brick method mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(mdks2), std(mdks2), tmd2s/nreps);
            end
         if dormks
            fprintf(['   Ribbon method mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(rmks2), std(rmks2), trm2s/nreps);
            end
         if doroks
            fprintf(['   Ribbon+Overlap method mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(roks2), std(roks2), tro2s/nreps);
            end
         if doepks
            fprintf(['   Equi-partition method mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(epks2), std(epks2), tep2s/nreps);
            end
         if docgks
            fprintf(['   Center-gravity method mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(cgks2), std(cgks2), tcg2s/nreps);
            end

         end % Loop over r
      end % Loop over n
   end % Loop over k

%% Finish up

   fprintf('***Work areas not freed--current bug when more than one allocated\n');
   fprintf('MDKS timing and accuracy test completed.');
   end

