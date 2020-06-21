function [I,fincenters] = k_means(X, k, Z, kp)

% $Id: k_means.m 2 2013-11-07 20:45:56Z  $

%  This software was developed in the Laboratory of Biological
% Modelling at The Rockefeller University by George N. Reeke

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

% V1A, 03/29/10, G. N. Reeke

% Perform K-means clustering on X to find k clusters.

% Arguments:
%  X(n,m) is the data matrix, containing n items of m dimensions.
%  Z(k,m) is a set of starting cluster centers -- this is passed
%   in as an argument to allow manual startup if needed.
%  kp = 1 to turn on printing & plotting of each cycle
%  Matrix I gives the cluster number for each observation in X.

global ctab nctb SPSZ

if (nargin < 4), kp = 0; end
if (kp)
   figure
   title('K-means progress')
   hold;
end

max_trial = 1000;
nobs = size(X,1);
ndim = size(X,2);
I_old = randi([1, k], nobs, 1);
Zwk = Z;
dsq = zeros(nobs,k);

for trial=1:max_trial
   % Assignment phase
   for i=1:k
      delxz = X - repmat(Zwk(i,:),nobs,1);
      dsq(:,i) = sum(delxz .* delxz, 2);
   end
   [~,I] = min(dsq,[],2);
   if (all(I_old == I))
       break
   end;

   % Graphics (debug information)
   if (kp)
      yy = [X(:,2); Zwk(:,2)];
      zz = [X(:,1); Zwk(:,1)];
      sz = [SPSZ*ones(nobs,1); 4*SPSZ*ones(k,1)];
      Imod = [rem(I-1,nctb) + 1; (1:i)'];
      cc = ctab(Imod,:);
      if (ndim >= 3)
         xx = [X(:,3); Zwk(:,3)];
         scatter3(xx,yy,zz,sz,cc,'filled');
      else
         scatter(yy,zz,sz,cc,'filled');
      end
      Ihist = histc(I,[1:k])
      qq = input('Continue? ');
      if (isempty(qq) == 0)
          return
      end
   end

   % Update phase
   % (If no points in the cluster, leave center unchanged)
   for i=1:k
      Ik = find(I==i);
      if isempty(Ik) == 0
         Zwk(i,:) = mean(X(Ik,:));
      end
   end;
   I_old = I;

   % A little more debug info
   if (kp)
       Zwk
   end

end;
% disp(['K-means converged in ', num2str(trial), ' trials'])
fincenters = Zwk;
end
