function [entropy] = dir_gamma(grand_X)

% $Id: dir_gamma.m 3 2014-02-27 21:23:39Z  $

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

% Approximate the distribution by one gamma function, and calculate the
%  entropy directly.

% INPUTS:
    % grand_X: a sequence of ISIs
% OUTPUTS:
    % entropy: the entropy estimation

code_length = size(grand_X,2);
X = grand_X(1, 1:code_length);
temp_phat = gamfit(X);  % phat has the parameters of the gamma distribution
phat = temp_phat';
a = phat(1,:);
b = phat(2,:);
temp_entropy = (a + log(b) + log(gamma(a)) + (1-a).*psi(a))./log(2);
if temp_entropy == Inf
    % fprintf(1, 'data degraded too much for fitting, entropy set to 0\n')
    temp_entropy = 0;
end;
if temp_entropy == NaN
    error('Invalid entropy!')
end;
entropy = temp_entropy;
end