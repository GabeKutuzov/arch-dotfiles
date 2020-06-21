% Multidimensional Kolmogorov-Smirnov (MDKS) statistic test
% (c) The Rockefeller University, 2013
% $Id: mdkstest.m 9 2016-01-04 19:30:24Z  $

function [] = mdkstest()

%  This software was developed in the Laboratory of Biological
% Modelling at The Rockefeller University by George N. Reeke

%  This file tests the MDKS code by running it alongside a brute-
% force MATLAB-coded version of the test and a brute-force mex-file
% version of the test on all combinations of the following data sets:
% (1) Dimensions given by the k loop.
% (2) Numbers of data points given by the n loop.
% (3) Uniform, normal, and Gamma distributed data.
% (4) Single-sided and double-sided tests.
% Only double precision, only equal sizes of the two data sets, and
%   only default axial divisions are tested.  However, alternatives
%   to these options were tested by the author.
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
do2side = 0;      % Do 2-sided tests
domlbf = 0;       % Matlab brute force
% Each variable here should be the 'g2' value for that method
%  to run it (see comments) or 0 to skip it.
domdks =  1;      %  1 = Binary-tree Fasano & Franceschini
dormks =  0;      %  2 = Ribbon approximation method
doroks =  0;      %  4 = Ribbon method with approx double overlaps
doepks =  8;      %  8 = Equi-partition overlap bricks
docgks = 16;      % 16 = Center-of-gravity overlap bricks
doupks =  0;      % 32 = Partition by ribbon fractions
docbf = 0;        % 64 = C brute force

%% User-settable parameters defining the random distributions
% Mxreps = 500;           % Maximum repeats to do for any case
Mxreps = 100;           % Maximum repeats to do for any case
Mnreps = 5;             % Minimum repeats to do for any case
Ulow = 0;               % Low end of uniform distribution
Uhigh = 1;              % High end of uniform distribution
Nmean = 20;             % Mean of normal distribution
Nsigma = 5;             % Sigma of normal distribution
Gk = 1.5;               % Shape of gamma distribution
Gscl = 10;              % Scale of gamma distribution

%% Do the runs
MDKSCOM = [];
RNames = { 'uniform'; 'normal'; 'gamma' };
g2 = domdks + dormks + doroks + doepks + docgks + doupks + docbf;
for k = 2:5
   % Running n in this order tests ability of mdskallo to
   % reallocate to higher or lower work area sizes.
%   for n = [200 25000 5000 1000]
   for n = [200 1000 5000]
%   for n = [10000 32000 100000]
      MDKSCOM = mdksallod(MDKSCOM, [], g2, n, n, k);
      nreps = floor(1000000/(k*n));
      if nreps < Mnreps, nreps = Mnreps; end
      if nreps > Mxreps, nreps = Mxreps; end
      for r = 1:3
         if domlbf
            mlks1 = zeros(nreps,1); tml1s = 0;
            if do2side, mlks2 = zeros(nreps,1); tml2s = 0; end
            end
         if docbf
            bfks1 = zeros(nreps,1); tbf2s = 0;
            if do2side, bfks2 = zeros(nreps,1); tbf1s = 0; end
            end
         if domdks
            mdks1 = zeros(nreps,1); tmd1s = 0;
            if do2side, mdks2 = zeros(nreps,1); tmd2s = 0; end
            end
         if dormks
            rmks1 = zeros(nreps,1); trm1s = 0;
            if do2side, rmks2 = zeros(nreps,1); trm2s = 0; end
            end
         if doroks
            roks1 = zeros(nreps,1); tro1s = 0;
            if do2side, roks2 = zeros(nreps,1); tro2s = 0; end
            end
         if doepks
            epks1 = zeros(nreps,1); tep1s = 0;
            if do2side, epks2 = zeros(nreps,1); tep2s = 0; end
            end
         if docgks
            cgks1 = zeros(nreps,1); tcg1s = 0;
            if do2side, cgks2 = zeros(nreps,1); tcg2s = 0; end
            end
         if doupks
            upks1 = zeros(nreps,1); tup1s = 0;
            if do2side, upks2 = zeros(nreps,1); tup2s = 0; end
            end
         for rep = 1:nreps
            switch r
            case 1
               % Uniform data
               X1 = random('unif',Ulow,Uhigh,k,n);
               X2 = random('unif',Ulow,Uhigh,k,n);
            case 2
               X1 = random('norm',Nmean,Nsigma,k,n);
               X2 = random('norm',Nmean,Nsigma,k,n);
            case 3
               X1 = random('gam',Gk,Gscl,k,n);
               X2 = random('gam',Gk,Gscl,k,n);
            end % Random type switch

%% Single-sided

            % Matlab-coded brute force
            if domlbf && k < 4
               tic
               mlks1(rep,1) = bfdiff(X1, X2)/sqrt(n);
               tml1s = tml1s + toc;
               end

            % brute-force mex-function
            if docbf
               tic
               bfks1(rep,1) = bfks1sd(MDKSCOM, X1, X2);
               tbf1s = tbf1s + toc;
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

            % Partition-by-ribbon-fraction approx method
            if doupks
               tic
               upks1(rep,1) = upks1sd(MDKSCOM, X1, X2);
               tup1s = tup1s + toc;
               end

%% Two-sided

            if do2side

            % Matlab-coded brute force
            if domlbf && k < 4
               tic
               s1 = bfdiff(X1, X2);
               s2 = bfdiff(X2, X1);
               mlks2(rep,1) = (s1 + s2)*sqrt(0.125/n);
               tml2s = tml2s + toc;
            end

            % brute-force mex-function
            if docbf
               tic
               bfks2(rep,1) = bfks2sd(MDKSCOM, X1, X2);
               tbf2s = tbf2s + toc;
               end

            % brick mex-function method
            if domdks
               tic
               mdks2(rep,1) = mdks2sd(MDKSCOM, X1, X2);
               tmd2s = tmd2s + toc;
               end

            % Ribbon method mex function
            if dormks && k == 2
               tic
               rmks2(rep,1) = rmks2sd(MDKSCOM, X1, X2);
               trm2s = trm2s + toc;
               end

            % Ribbon method with approx double overlaps
            if doroks && k == 2
               tic    
               roks2(rep,1) = roks2sd(MDKSCOM, X1, X2);
               tro2s = tro2s + toc;
               end

            % Equi-partition approx method
            if doepks
               tic
               epks2(rep,1) = epks2sd(MDKSCOM, X1, X2);
               tep2s = tep2s + toc;
               end

            % Center-of-gravity approx method
            if docgks
               tic
               cgks2(rep,1) = cgks2sd(MDKSCOM, X1, X2);
               tcg2s = tcg2s + toc;
               end

            % Partition-by-ribbon-fraction approx method
            if doupks
               tic
               upks2(rep,1) = upks2sd(MDKSCOM, X1, X2);
               tup2s = tup2s + toc;
               end

            end % End if do2side

            end % Loop over rep

%% Report averages and times

         [DivsX1, DivsX2] = mdksdivs(MDKSCOM);
         fprintf('\nFor k = %d, n = %d, %s distribution, divs = (%d,%d)\n', ...
            k, n, RNames{r}, DivsX1(1), DivsX2(1))

         if domlbf && k < 4
            fprintf(['   Brute-force ML mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(mlks1), std(mlks1), tml1s/nreps);
            end
         if docbf
            fprintf(['   Brute-force mex mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(bfks1), std(bfks1), tbf1s/nreps);
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
         if doupks
            fprintf(['   Ribbon-fraction method mean MDKS1s = %f, sigma MDKS1s' ...
               ' = %f, time = %f\n'], mean(upks1), std(upks1), tup1s/nreps);
            end

         if do2side
         
         if domlbf && k < 4
            fprintf(['   Brute-force ML mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(mlks2), std(mlks2), tml2s/nreps);
            end
         if docbf
            fprintf(['   Brute-force mex mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(bfks2), std(bfks2), tbf2s/nreps);
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
         if doupks
            fprintf(['   Ribbon-fraction method mean MDKS2s = %f, sigma MDKS2s' ...
               ' = %f, time = %f\n'], mean(upks2), std(upks2), tup2s/nreps);
            end

         end % End if do2side

         end % Loop over r
      end % Loop over n
   end % Loop over k

%% Finish up

   mdksallod(MDKSCOM, 0);
   fprintf('MDKS timing and accuracy test completed.');
   end

%% Function to perform the brute-force generalized-quadrant sums--
%  based on code in original version of hcentropy

function bfd = bfdiff(X1, X2)

   k = size(X1,1);
   n = size(X1,2);
   bfd = 0;
   switch k
   case 1
      for i = 1:n
         X1low = numel(find(X1(1,:) <= X1(1,i)));
         X2low = numel(find(X2(1,:) <= X1(1,i)));
         bfd = max(bfd, abs(X1low-X2low));
         end
   case 2
      for i = 1:n
         X1LLHC = numel(find(X1(1,:) < X1(1,i) & X1(2,:) < X1(2,i)));
         X1LRHC = numel(find(X1(1,:) > X1(1,i) & X1(2,:) < X1(2,i)));
         X1ULHC = numel(find(X1(1,:) < X1(1,i) & X1(2,:) > X1(2,i)));
         X1URHC = numel(find(X1(1,:) > X1(1,i) & X1(2,:) > X1(2,i)));
         X2LLHC = numel(find(X2(1,:) < X1(1,i) & X2(2,:) < X1(2,i)));
         X2LRHC = numel(find(X2(1,:) > X1(1,i) & X2(2,:) < X1(2,i)));
         X2ULHC = numel(find(X2(1,:) < X1(1,i) & X2(2,:) > X1(2,i)));
         X2URHC = numel(find(X2(1,:) > X1(1,i) & X2(2,:) > X1(2,i)));
         quads = [ abs(X1LLHC - X2LLHC); ...
                     abs(X1LRHC - X2LRHC); ...
                     abs(X1ULHC - X2ULHC); ...
                     abs(X1URHC - X2URHC); ];
         mxquad = max(quads);
         bfd = max(bfd, mxquad);
         end
   case 3
      for i = 1:n
         X1LLFOC = numel(find(X1(1,:) < X1(1,i) & X1(2,:) < X1(2,i) & X1(3,:) < X1(3,i)));
         X1LRFOC = numel(find(X1(1,:) > X1(1,i) & X1(2,:) < X1(2,i) & X1(3,:) < X1(3,i)));
         X1ULFOC = numel(find(X1(1,:) < X1(1,i) & X1(2,:) > X1(2,i) & X1(3,:) < X1(3,i)));
         X1URFOC = numel(find(X1(1,:) > X1(1,i) & X1(2,:) > X1(2,i) & X1(3,:) < X1(3,i)));
         X2LLFOC = numel(find(X2(1,:) < X1(1,i) & X2(2,:) < X1(2,i) & X2(3,:) < X1(3,i)));
         X2LRFOC = numel(find(X2(1,:) > X1(1,i) & X2(2,:) < X1(2,i) & X2(3,:) < X1(3,i)));
         X2ULFOC = numel(find(X2(1,:) < X1(1,i) & X2(2,:) > X1(2,i) & X2(3,:) < X1(3,i)));
         X2URFOC = numel(find(X2(1,:) > X1(1,i) & X2(2,:) > X1(2,i) & X2(3,:) < X1(3,i)));
         X1LLBOC = numel(find(X1(1,:) < X1(1,i) & X1(2,:) < X1(2,i) & X1(3,:) > X1(3,i)));
         X1LRBOC = numel(find(X1(1,:) > X1(1,i) & X1(2,:) < X1(2,i) & X1(3,:) > X1(3,i)));
         X1ULBOC = numel(find(X1(1,:) < X1(1,i) & X1(2,:) > X1(2,i) & X1(3,:) > X1(3,i)));
         X1URBOC = numel(find(X1(1,:) > X1(1,i) & X1(2,:) > X1(2,i) & X1(3,:) > X1(3,i)));
         X2LLBOC = numel(find(X2(1,:) < X1(1,i) & X2(2,:) < X1(2,i) & X2(3,:) > X1(3,i)));
         X2LRBOC = numel(find(X2(1,:) > X1(1,i) & X2(2,:) < X1(2,i) & X2(3,:) > X1(3,i)));
         X2ULBOC = numel(find(X2(1,:) < X1(1,i) & X2(2,:) > X1(2,i) & X2(3,:) > X1(3,i)));
         X2URBOC = numel(find(X2(1,:) > X1(1,i) & X2(2,:) > X1(2,i) & X2(3,:) > X1(3,i)));
         octs = [ abs(X1LLFOC - X2LLFOC); ...
                  abs(X1LRFOC - X2LRFOC); ...
                  abs(X1ULFOC - X2ULFOC); ...
                  abs(X1URFOC - X2URFOC); ...
                  abs(X1LLBOC - X2LLBOC); ...
                  abs(X1LRBOC - X2LRBOC); ...
                  abs(X1ULBOC - X2ULBOC); ...
                  abs(X1URBOC - X2URBOC); ];
         mxoct = max(octs);
         bfd = max(bfd, mxoct);
         end
   end % k switch
   end % bfdiff function

