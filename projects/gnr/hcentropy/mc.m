function [ Zsubn, compnum ] = mc(subseq_num, mclseq)

% $Id: mc.m 6 2014-05-12 20:21:24Z  $

%  This software was developed in the Laboratory of Biological
% Modelling at The Rockefeller University by Nicholas D. Watters
% and George N. Reeke

% (c) The Rockefeller University, 2013
%
% This routine is part of a set of routines for computing the entropy of a
% neuronal spike train by the history clustering method of Watters & Reeke
% (Neural Computation, submitted) and various comparison methods as
% documented in the main routine of this set, hcentropy.m.
%
% This software is distributed under GPL, version 2.
% This program is free software; you can redistribute it and/or
% modify it under the terms of the GNU General Public License as
% published by the Free Software Foundation; either version 2 of
% the License, or (at your option) any later version.
% Accordingly, this program is distributed in the hope that it will
% be useful, but WITHOUT ANY WARRANTY; without even the implied
% warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
% See the GNU General Public License for more details.  You should
% have received a copy of the GNU General Public License along with
% this program.  If not, see <http://www.gnu.org/licenses/>.

% See README file for revision history
% N.B.  Single-ended MDKS test changed to double-ended, 04/17/14

% PROGRAM: Determines internal variation within the experimental data
%  (entiredata) itself using multi-dimensional Kolmogorov Smirnov
%  statistic (Fasano & Franceschini, 1987)
% Generates subseq_num subsequences (if possible) from entiredata,
%  and conducts all pairwise comparisons

% INPUTS:
   % subseq_num:  Number of subsequences to generate
   % mclseq:  Number of ISIs in current iteration

% OUTPUTS:
    % Zsubn: a vector of all pairwise differences
    % compnum: the number differences computed

global entiredata tree_depth mdkscom

compnum = 0;
X = entiredata;

gX = zeros(tree_depth,size(X,2)-tree_depth); % History matrix
for i=1:tree_depth
    gX(i,:) = X(i:size(X,2)+i-tree_depth-1);
end

%% Generate subsequences
if subseq_num*mclseq <= size(gX,2)
    startpoints = floor(size(gX,2)/subseq_num)*[0:1:subseq_num]+1;
    subseqs = cell(1,subseq_num);
    for i = 1:subseq_num
        subseqs{1,i} = gX(:,startpoints(i):startpoints(i)+mclseq-1);
    end
else
    ssnum = floor(size(gX,2)/mclseq);
    if ssnum <= 2;
        Zsubn = 0;
        fprintf(['Cannot do Monte Carlo for sequence length %d\n' ...
            '   with %d subsequences.\n'], mclseq, subseq_num);
        return;
    end
    startpoints = floor(size(gX,2)/ssnum)*[0:1:ssnum-1]+1;
    subseqs = cell(1,ssnum);
    for i = 1:ssnum
        subseqs{1,i} = gX(:,startpoints(i):startpoints(i)+mclseq-1);
    end
end

%% Kolmogorov-Smirnov test performed by mex package
nseqs = numel(subseqs);
Zvals = zeros(1,nseqs*(nseqs-1)/2);
nZs = 0;
mdkscom = mdksallod(mdkscom, [], [], mclseq, mclseq, tree_depth);
for ii = 1:nseqs
    for jj = ii+1:nseqs
        X1 = subseqs{1,ii};
        X2 = subseqs{1,jj};
        nZs = nZs + 1;
        Zvals(nZs) = mdks2sd(mdkscom, X1, X2); 
    end
end
Zsubn = Zvals;
compnum = nZs;

end

