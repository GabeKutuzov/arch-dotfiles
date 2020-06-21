function [ st ] = refrac_gamma_fitter( params )

% $Id: refrac_gamma_fitter.m 2 2013-11-07 20:45:56Z  $

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

% Determines how good a three-parameter gamma distribution fits smalldata

global smalldata

datsize = 1000;
trials = 10;
sst = zeros(1,trials);
a = params(1);
b = params(2);
c = params(3);

for i = 1:trials

x = c+gamrnd(a,b,1,datsize);
indices = randi(numel(smalldata),1,datsize);
y = smalldata(indices);

[~,ssst] = kstest2(x,y);
sst(i) = 1-ssst;
end
st = mean(sst);

end

