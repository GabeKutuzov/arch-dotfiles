function [iter_H] = strong(bin_width, iter, ...
   S_Thresh, S_MxAlSz, STRONGPLOT, grand_X)

% $Id: strong.m 5 2014-03-18 20:18:14Z  $

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

% Calculate the entropy by Strong, Koberle, de Ruyter van Steveninck,
%     Bialek (1998)

% INPUTS:
   % bin_width:  Bin size for generating 'words'.
   % iter:  A row, the length of which is the number of times the entropy
   %  is to be estimated and the elements of which define how much of
   %  the data to use in each iteration.
   % S_Thresh:  Threshold for discarding long words from the fit.
   % S_MxAlSz:  When > 0, turns on an alternative method of
   %  generating data words in which fixed bins are not used and ISIs
   %  are instead digitized as integer multiples of the bin_width.
   %  The value sets a maximum alphabet size to help minimize under-
   %  sampling a large word set.
   % STRONGPLOT:  1 to plot entropy vs 1/word-length.
   % grand_X: The ISI data.

delta = bin_width;
iter_H = zeros(1, size(iter, 2));
for i=1:size(iter, 2);
   code_length = iter(1, i);
   old_b = [];

   %% Use strongmx mex code to perform literature or modified method
   % Code starts with word length 3 (lit method) or 1 (modified method)
   % and increases by one until the regression test stops it.
   Mxwl = 120;          % Max wl -- just a safety to stop runaway
   Hwl = zeros(1,Mxwl);
   xo  = ones(Mxwl,2);
   if S_MxAlSz == 0
      wli = 2; minreg = 3; Tunit = 1.0/delta;
   else
      wli = 0; minreg = 2; Tunit = 1.0;
   end
   thrsq = S_Thresh^2;
   save_this = 0;
   for wl = 1:Mxwl
%% DEBUG %%
%% fprintf('Calling strongmx with N = %d, wl = %d\n', code_length, wl);
%% END DEBUG %%
      Hwl(wl) = strongmx(grand_X, delta, code_length, wl+wli, ...
         S_MxAlSz);
      xo(wl,2) = Tunit/(wl+wli);
%% DEBUG %%
%% fprintf('Entropy for wl = %d = %f\n', wl+wli, Hwl(wl));  %% DEBUG %%
%% END DEBUG %%
      if wl <= minreg, continue, end
      % After minreg word lengths are done, start regressing
      % The regression now uses up to but not including the current wl
      wlm1 = wl - 1;
      y = Hwl(1:wlm1);
      x = xo(1:wlm1,:);
      b = regress(y', x)';
      yerr2 = (y - b * x').^2;
      mse = mean(yerr2);
      ye2new = (Hwl(wl) - b * xo(wl,:)').^2;
      if ye2new > thrsq*mse && save_this == 0
         save_this = 1;
         old_b = b;
      end
      if (save_this > 0)
            save_this = save_this + 1;
         if (save_this >= 10), break; end
      end
   end   % End loop over wl
   if save_this == 0, old_b = b; end
   iter_H(i) = old_b(1);

%% Perform plot of extrapolation data if requested
if STRONGPLOT
   figure
   y = Hwl(1:wl);
   x = xo(1:wl,:);
   scatter(x(:,2),y);
   hold on
   line([0 x(1,2)],[old_b(1) old_b*x(1,:)']);
   xlabel('1/T');
   ylabel('Entropy rate');
   if S_MxAlSz == 0
      lbl = sprintf('Strong et al. extrapolation for N = %d', ...
         code_length);
   else
      lbl = sprintf('Strong (mod.) extrapolation for N = %d', ...
         code_length);
   end
   title(lbl);
   end
end


