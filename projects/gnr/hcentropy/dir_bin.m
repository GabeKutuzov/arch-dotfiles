function [e] = dir_bin(grand_X, delta)

% $Id: dir_bin.m 3 2014-02-27 21:23:39Z  $

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

% Divides the data up into bins and calculates the entropy directly
%  from the formula

X = grand_X;
n = histc(X, 0:delta:ceil(max(X)/delta)*delta);
p = n ./sum(n);
log_p = log2(p);
log_p(log_p==-Inf) = 0;
e = -log_p * p' + log2(delta);
end

