function [ Zsubn ] = nelder (telements)

% $Id: nelder.m 6 2014-05-12 20:21:24Z  $

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

% Program: This is the Nelder-Mead cost function.  It determines the 
%  difference between experimental data and artificial data using the
%  multi-dimensional Kolmogorov Smirnov statistic
%  (Fasano & Franceschini, 1987).

% INPUT:
    % telements: this vector contains the parameters for the artificial
    %  data generation.

% OUTPUT:
    % Zsubn is a single multi-dimensional kolmogorov-smirnov statistic

global lnelseq tree_depth mdkscom
global numgams skeleton1 maxdatlength entiredata

%% Make the T array
% NOTE: numgams must be 1 or 2
if numgams == 1
    T = skeleton1;
    for i = 1:size(T,1)
        for j = 1:size(T,2)
            for k = 1:size(T{i,j},1)
                for kk = 1:size(T{i,j},2)
                    if T{i,j}(k,kk)~=0 && T{i,j}(k,kk)~=Inf
                        T{i,j}(k,kk) = telements(T{i,j}(k,kk));
                    end
                end
            end
        end
    end
elseif numgams == 2
    T = skeleton1(1:end-1,:);
    for i = 1:size(T,1)-1
        for j = 1:size(T,2)
            for k = 1:size(T{i,j},1)
                for kk = 1:size(T{i,j},2)
                    if T{i,j}(k,kk)~=0 && T{i,j}(k,kk)~=Inf
                        T{i,j}(k,kk) = telements(T{i,j}(k,kk));
                    end
                end
            end
        end
    end
    T_probs = cell2mat(skeleton1(4,:));
    for i = 1:numel(T_probs)
        T_probs(i) = telements(T_probs(i));
    end
end

%% Generate sequence
if numgams == 2
    if (size(T,1) ~= 3 || lnelseq < 2)
       error('nelder called with bad desciptor');
    end
    seq = zeros(lnelseq,1);
    I = zeros(lnelseq,1);

    % Generate the remaining values picking region based on history
    % As a protection against nonsense bounds, and to handle the initial
    %     runin, if a match is not found the previous region is reused.

    lhist = size(T{1,1},1);
    for i = 1:lnelseq+tree_depth+lhist
       if (i <= lhist)
          continue
       end;
       prior = seq(i-lhist:i-1);

       % Find which point is the center this history is closest to:
       ND = zeros(1,size(T,2));
       for j = 1:size(T,2)
           ND(j) = sum((T{1,j}-prior).^2);
       end
       [~,ir] = min(ND);
       % Choose which gamma on a weighted coin flip:
       coin_flip = rand(1,1);
       if coin_flip < T_probs(ir)
           spec = T{2,ir};
       else
           spec = T{3,ir};
       end
       a = abs(spec(1));
       b = abs(spec(2));
       c = abs(spec(3));
       if (a == 0)
          % This is a Gaussian region
          seq(i) = b + c*randn(1);
       else
          % This is a gamma region
          seq(i) = c + gamrnd(a,b);
       end
       I(i) = ir;
    end;
    seq = seq(lhist+tree_depth+1:end);
    XS = seq';
else % If there don't exist T1 and T2, only T
    if (size(T,1) ~= 2 || lnelseq < 2)
       error('nelder called with bad desciptor');
    end
    seq = zeros(lnelseq,1);
    I = zeros(lnelseq,1);
    lhist = size(T{1,1},1);
    for i = 1:lnelseq+tree_depth+lhist
       if (i <= lhist)
          continue
       end;

       prior = seq(i-lhist:i-1);

       % Find which point is the center this history is closest to:
       ND = zeros(1,size(T,2));
       for j = 1:size(T,2)
           ND(j) = sum((T{1,j}-prior).^2);
       end
       [~,ir] = min(ND);

       % Choose specifications from T
       spec = T{2,ir};
       a = abs(spec(1));
       b = abs(spec(2));
       c = abs(spec(3));
       if (a == 0)
          % This is a Gaussian region
          seq(i) = b + c*randn(1);
       else
          % This is a gamma region
          seq(i) = c + gamrnd(a,b);
       end
       I(i) = ir;
    end;
    seq = seq(lhist+tree_depth+1:end);
    XS = seq';
end

%% Choose experimental data
startseqpoint = randi(maxdatlength-lnelseq-1);
X = entiredata(startseqpoint:startseqpoint+lnelseq+1);

% X1 from XS is synthetic, X2 from X is experimental
X1 = zeros(tree_depth,size(XS,2)-tree_depth);
X2 = zeros(tree_depth,size(XS,2)-tree_depth);
for i=1:tree_depth
    X1(i,:) = XS(i:size(X1,2)+i-1);
    X2(i,:) = X(i:size(X1,2)+i-1);
end

%% Calculate the MDKS using the mdks mex library
%   according to the Fasano/Franceschini paper
mdksn = size(X2,2);
mdkscom = mdksallod(mdkscom, [], [], mdksn, mdksn, tree_depth);
Zsubn = mdks1sd(mdkscom, X2, X1);

end
