function [ st ] = refrac_2gamma_fitter( params )

% $Id: refrac_2gamma_fitter.m 2 2013-11-07 20:45:56Z  $

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

% Fits sum of two gammas.  params is string of 7 with last being
%  the weighting of the first gamms (between 0 and 1)

global smalldata fitter_datsize

datsize = ceil(fitter_datsize);
resample = 1;
trials = 100;
sst = zeros(1,trials);
a1 = params(1);
b1 = params(2);
c1 = params(3);
a2 = params(4);
b2 = params(5);
c2 = params(6);
wts1 = params(7);
if wts1>1
   wts1 = 1;
elseif wts1<0
   wts1 = 0;
end

for i = 1:trials

    x1 = c1+gamrnd(a1,b1,1,floor(datsize*wts1));
    x2 = c2+gamrnd(a2,b2,1,ceil(datsize*(1-wts1)));
    x = [x1,x2];
    switch resample
        case 0
            y = smalldata(1:datsize);
        case 1
            indices = randi(numel(smalldata),1,datsize);
            y = smalldata(indices);
    end
    try
    [~,ssst] = kstest2(x,y);
    catch
        error('Error in fitting data with sum of two gammas')
    end
    sst(i) = 1-(1000*ssst);
end
st = mean(sst);

end

