function [ koz_leo_e ] = koz_leo(hist_X, KL_noise_test)

% $Id: koz_leo.m 4 2014-03-17 21:12:51Z  $

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

% Compute spike-train entropy by the Kozachenko & Leonenko method

% INPUTS:
   % hist_X: a history matrix of ISIs
   % KL_noise_test: fraction of smallest ISI difference used
   %  to set noise range
% OUTPUTS:
   % kox_leo_e: the entropy estimation


% Find noise multiplication factor:
temp_X = hist_X(1,:);
unique_X = unique(temp_X);
sort_X = sort(unique_X);
diff_X = sort_X(2:end)-sort_X(1:end-1);
sort_diff_X = sort(diff_X);
badness_tracker = [1];
if KL_noise_test > 0
   for i=2:ceil(numel(sort_diff_X)/2)
      if sort_diff_X(i) > KL_noise_test*sort_diff_X(i-1)
         badness_tracker = [badness_tracker,i];
      end
   end
end
minv = sort_diff_X(badness_tracker(end));
noise_multfactor = minv;

dimension = size(hist_X,1);
Euler = .5772157;
ptnum = size(hist_X,2);
BS_hist_X = hist_X+noise_multfactor*(rand(size(hist_X))-.5);

neighbors  = zeros(1,ptnum);
neighbordists = zeros(1,ptnum);
for j=1:numel(neighbors)
   point = BS_hist_X(:,j);
   new_BS_hist_X = BS_hist_X;
   new_BS_hist_X(:,j) = [];
   temp_dists = zeros(size(new_BS_hist_X));
   for jj = 1:dimension
      temp_dists(jj,:) = (new_BS_hist_X(jj,:)-point(jj)).^2;
   end
   dists = sum(temp_dists,1);
   [md,index] = min(dists);
   if index < j
      neighbors(j) = index;
      neighbordists(j) = sqrt(md);
   else
      neighbors(j) = index+1;
      neighbordists(j) = sqrt(md);
   end
end

term1 = (dimension/ptnum)*sum(log2(neighbordists));
term2 = log2((pi^(dimension/2))/gamma(1+dimension/2));
term3 = Euler/log(2)+log2(ptnum-1);
koz_leo_e = term1+term2+term3;

end




