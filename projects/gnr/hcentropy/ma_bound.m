function [e] = ma_bound(iter, grand_X, delta)

% $Id: ma_bound.m 2 2013-11-07 20:45:56Z  $

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

% Divide the data up into bins and calculates the Ma bound directly
%  from the formula

e = zeros(1, length(iter));
for i=1:length(iter)
    code_length = iter(i);
    X = grand_X(1, 1:code_length);
    n = histc(X, 0:delta:ceil(max(X)/delta)*delta);
    p = n ./sum(n);
    p_c = sum(p.^2);

    log_p_c = log2(p_c);
    log_p_c(log_p_c==-Inf) = 0;
    e(i) = -log_p_c + log2(delta);
end;

