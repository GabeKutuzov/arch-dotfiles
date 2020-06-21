% Multidimensional Kolmogorov-Smirnov (MDKS) statistic test
% Variant to test alternative depths of brick division
% (c) The Rockefeller University, 2013-2015
% $Id: mdkststd.m 9 2016-01-04 19:30:24Z  $

function [] = mdkststd()

%  This software was developed in the Laboratory of Biological
% Modelling at The Rockefeller University by George N. Reeke

%  This file tests various versions of the MDKS code by running them
% alongside a brute-force C-coded mex-file version of the test on any
% combinations of the following data sets:
% (1) Dimensions given by the k loop.
% (2) Numbers of data points given by the n loop.
% (3) Uniform, normal, and Gamma distributed data.
% (4) Two normal distributions with different sigma.
% (5) Depths from one less to two more than the default.
% (6) Force one- or two-quad sums versions of the code.
% Only double precision and only equal sizes of the two data sets
%   are tested.  However, alternatives to these options were tested
%   by the author.
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

% Select which methods to test (1 to perform, 0 to skip)
dorv6 = 0;        % Released version 6 (still has neg data bug)
% Each variable here should be the 'g2' value for that method
%  to run it (see comments) or 0 to skip it.
domdks =  1;      %  1 = Binary-tree Fasano & Franceschini
dormks =  0;      %  2 = Ribbon approximation method
doroks =  0;      %  4 = Ribbon method with approx double overlaps
doepks =  0;      %  8 = Equi-partition overlap bricks
docgks = 16;      % 16 = Center-of-gravity overlap bricks
doupks =  0;      % 32 = Unequal-partition
docbf  =  0;      % 64 = C brute force

% Select which data dimensions to analyze
%  ddims = 2:5
ddims = 2:5;

% Select which data set sizes to analyze
%  Note: Specify ndata out of order to test ability of mdskallo
%  to reallocate to higher or lower work area sizes, e.g.
%  ndata = 50;   % ndata = [50 200];
%  ndata = [200 25000 5000 1000];
  ndata = [320 1000 3200 10000 32000 100000];

% Select which depths to analyze (0 = default, always first)
drng = [0 -1 1 2];
%drng = 0;

% Select which data sets to analyze
%   (1 = uniform, 2 = normal, 3 = gamma, 4 = two unlike normals)
rdist = [1 4];
%rdist = 1;

% Select which summation algorithms to analyze (V7 and later)
%   (0 = use defaults, 1 = one quad array, 2 = two quad arrays)
qsums = [1 2];
%qsums = 2;

%% User-settable parameters defining the random distributions
Mxreps = 100;           % Maximum repeats to do for any case
Mnreps = 5;             % Minumum repeats to do for any case
Ulow = 0;               % Low end of uniform distribution
Uhigh = 1;              % High end of uniform distribution
Nmean = 10;             % Mean of normal distribution
Nsigma = 5;             % Sigma of normal distribution
Nsigma2 = 6;            % Sigma of second normal distribution
Gk = 1.5;               % Shape of gamma distribution
Gscl = 10;              % Scale of gamma distribution

% Set random number seeds to generate same data each time
rng('default');

%% Do the runs
ndr = numel(drng);
nqs = numel(qsums);
nrr = numel(rdist);
MDKSCC = cell(ndr,1);
V6KSCC = cell(ndr,1);
RNames = { 'uniform'; 'normal'; 'gamma'; 'unlike' };
g2 = domdks + dormks + doroks + doepks + docgks + docbf + ...
   1024*ismember(2,qsums); 
for k = ddims
   for n = ndata
      MDKSCC{1} = mdksallod(MDKSCC{1}, [], g2, n, n, k);
      [DivsX1, DivsX2] = mdksdivs(MDKSCC{1});
      for d = 2:ndr
         MDKSCC{d} = mdksallod(MDKSCC{d}, DivsX1+drng(d), g2, n, n, k);
         end
      if dorv6, for d = 1:ndr
         V6KSCC{d} = oldmdksallod(V6KSCC{d}, DivsX1+drng(d), 1, n, n, k);
         end, end
      nreps = floor(1000000/(k*n));
      if nreps < Mnreps, nreps = Mnreps; end
      if nreps > Mxreps, nreps = Mxreps; end
      % Generate data for reuse later with new depth
      nr = n*nreps;
      X1RR = cell(1,4);
      X2RR = cell(1,4);

% Loop over nrr types of data

      for r = rdist

         switch r
         case 1
            X1RR{1} = random('unif',Ulow,Uhigh,k,nr);
            X2RR{1} = random('unif',Ulow,Uhigh,k,nr);
         case 2
            X1RR{2} = random('norm',Nmean,Nsigma,k,nr);
            X2RR{2} = random('norm',Nmean,Nsigma,k,nr);
         case 3
            X1RR{3} = random('gam',Gk,Gscl,k,nr);
            X2RR{3} = random('gam',Gk,Gscl,k,nr);
         case 4
            X1RR{4} = random('norm',Nmean,Nsigma,k,nr);
            X2RR{4} = random('norm',Nmean,Nsigma2,k,nr);
         otherwise
            error('requested data set type not programmed yet')
         end %switch

         if docbf,  bfks1 = zeros(nreps,1); tbf1s = 0; end
         if dorv6,  v6ks1 = zeros(nreps,nqs,ndr);
            tv61s = zeros(nqs,ndr); end
         if domdks, mdks1 = zeros(nreps,nqs,ndr);
            tmd1s = zeros(nqs,ndr); end
         if dormks, rmks1 = zeros(nreps,nqs,ndr);
            trm1s = zeros(nqs,ndr); end
         if doroks, roks1 = zeros(nreps,nqs,ndr);
            tro1s = zeros(nqs,ndr); end
         if doepks, epks1 = zeros(nreps,nqs,ndr);
            tep1s = zeros(nqs,ndr); end
         if docgks, cgks1 = zeros(nreps,nqs,ndr);
            tcg1s = zeros(nqs,ndr); end

%% Loop over repeats and depth ranges and quad sum methods

         for rep = 1:nreps
            nr2 = n*rep; nr1 = nr2 - n + 1;
            X1 = X1RR{r}(:,nr1:nr2);
            X2 = X2RR{r}(:,nr1:nr2);

            % brute-force mex-function
            if docbf
               tic
               bfks1(rep) = bfks1sd(MDKSCC{1}, X1, X2);
               tbf1s = tbf1s + toc;
               end

            for d = 1:ndr
               for q = 1:nqs
                  mdksqset(MDKSCC{d},qsums(q));

                  % brick mex-function method
                  if domdks
                     tic
                     mdks1(rep,q,d) = mdks1sd(MDKSCC{d}, X1, X2);
                     tmd1s(q,d) = tmd1s(q,d) + toc;
                     end

                  % R6 brick mex-function method
                  if dorv6
                     tic
                     v6ks1(rep,q,d) = oldmdks1sd(V6KSCC{d}, X1, X2);
                     tv61s(q,d) = tv61s(q,d) + toc;
                     end

                  % Ribbon method mex function
                  if dormks && k == 2
                     tic
                     rmks1(rep,q,d) = rmks1sd(MDKSCC{d}, X1, X2);
                     trm1s(q,d) = trm1s(q,d) + toc;
                     end

                  % Ribbon method with approx double overlaps
                  if doroks && k == 2
                     tic    
                     roks1(rep,q,d) = roks1sd(MDKSCC{d}, X1, X2);
                     tro1s(q,d) = tro1s(q,d) + toc;
                     end

                  % Equi-partition approx method
                  if doepks
                     tic
                     epks1(rep,q,d) = epks1sd(MDKSCC{d}, X1, X2);
                     tep1s(q,d) = tep1s(q,d) + toc;
                     end

                  % Center-of-gravity approx method
                  if docgks
                     tic
                     cgks1(rep,q,d) = cgks1sd(MDKSCC{d}, X1, X2);
                     tcg1s(q,d) = tcg1s(q,d) + toc;
                     end

                  end % Loop over qsum methods
               end % Loop over depths

            end % Loop over reps

%% Report averages and times

         fprintf('\nFor k = %d, n = %d, %s data\n', k, n, RNames{r})

         if docbf
            fprintf(['   Brute-force mex mean MDKS1s = %f,' ...
               ' sigma MDKS1s = %f, time = %f\n'], ...
               mean(bfks1), std(bfks1), tbf1s/nreps);
            end

         for d = 1:ndr
            for q = 1:nqs
                         
               fprintf('-->Depth = %d, Sum method = %d\n', ...
                  DivsX1+drng(d),qsums(q));

               if domdks      % Have standard to show errors
                  corr = mean(mdks1(:,q,d));
                  fprintf(['   Brick method mean MDKS1s = %f,' ...
                     ' sigma MDKS1s = %f, time = %f\n'], ...
                     corr, std(mdks1(:,q,d)), tmd1s(q,d)/nreps);
                  if dorv6
                     tval = mean(v6ks1(:,q,d));
                     fprintf(['   Old brick method mean = %f,' ...
                        ' sigma = %f, error = %f, time = %f\n'], ...
                        tval, std(v6ks1(:,q,d)), (tval-corr)/corr, ...
                        tv61s(q,d)/nreps);
                     end
                  if dormks
                     tval = mean(rmks1(:,q,d));
                     fprintf(['   Ribbon method mean = %f,' ...
                        ' sigma = %f, error = %f, time = %f\n'], ...
                        tval, std(rmks1(:,q,d)), (tval-corr)/corr, ...
                        trm1s(q,d)/nreps);
                     end
                  if doroks
                     tval = mean(roks1(:,q,d));
                     fprintf(['   Ribbon+Overlap method mean = %f,' ...
                        ' sigma = %f, error = %f, time = %f\n'], ...
                        tval, std(roks1(:,q,d)), (tval-corr)/corr, ...
                        tro1s(q,d)/nreps);
                     end
                  if doepks
                     tval = mean(epks1(:,q,d));
                     fprintf(['   Equi-partition method mean = %f,' ...
                        ' sigma = %f, error = %f, time = %f\n'], ...
                        tval, std(epks1(:,q,d)), (tval-corr)/corr, ...
                        tep1s(q,d)/nreps);
                     end
                  if docgks
                     tval = mean(cgks1(:,q,d));
                     fprintf(['   Center-gravity method mean = %f,' ...
                        ' sigma = %f, error = %f, time = %f\n'], ...
                        tval, std(cgks1(:,q,d)), (tval-corr)/corr, ...
                        tcg1s(q,d)/nreps);
                     end
               else           % No standard, no errors
                  if dorv6
                     fprintf(['   Old brick method mean MDKS1 = %f,' ...
                        ' sigma MDKS1 = %f, time = %f\n'], ...
                        mean(v6ks1(:,q,d)), std(v6ks1(:,q,d)), ...
                        tv61s(q,d)/nreps);
                     end
                  if dormks
                     fprintf(['   Ribbon method mean MDKS1 = %f,' ...
                        ' sigma MDKS1 = %f, time = %f\n'], ...
                        mean(rmks1(:,q,d)), std(rmks1(:,q,d)), ...
                        trm1s(q,d)/nreps);
                     end
                  if doroks
                     fprintf(['   Ribbon+Overlap method mean MDKS' ...
                        '1s = %f, sigma MDKS1s = %f, time = %f\n'], ...
                        mean(roks1(:,q,d)), std(roks1(:,q,d)), ...
                        tro1s(q,d)/nreps);
                     end
                  if doepks
                     fprintf(['   Equi-partition method mean MDKS' ...
                        '1s = %f, sigma MDKS1s = %f, time = %f\n'], ...
                        mean(epks1(:,q,d)), std(epks1(:,q,d)), ...
                        tep1s(q,d)/nreps);
                     end
                  if docgks
                     fprintf(['   Center-gravity method mean MDKS'...
                        '1s = %f, sigma MDKS1s = %f, time = %f\n'], ...
                        mean(cgks1(:,q,d)), std(cgks1(:,q,d)), ...
                        tcg1s(q,d)/nreps);
                     end
                  end % Not domdks

               end % Loop over summation methods
            end % Loop over depths

% DEBUG
%            [BDif, ibd] = max(abs(mdks1(:,1,1) - bfks1));
%            fprintf('==>The largest difference was at rep %d\n',ibd);
%            fprintf('   mdks = %f, bfks = %f\n', mdks1(ibd,1,1), bfks1(ibd));
% ENDDEBUG

         end % Loop over r
      end % Loop over n
   end % Loop over k

%% Finish up
   
   for d=1:ndr
      mdksallod(MDKSCC{d},0);
      end
   fprintf('MDKS timing and accuracy test completed.');
   end

