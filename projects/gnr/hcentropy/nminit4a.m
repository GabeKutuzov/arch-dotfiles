function [x,yinfo] = nminit4a(ufn,x,kavoid,varargin)

% $Id: nminit4a.m 2 2013-11-07 20:45:56Z  $

%  This software was developed in the Laboratory of Biological
% Modelling at The Rockefeller University by George N. Reeke

% (c) The Rockefeller University, 2013
%
% This routine is provided as a component part of a set of routines
% for computing the entropy of a neuronal spike train by the history
% clustering method of Watters & Reeke (Neural Computation, submitted),
% but it may also be used in other applications having no connection
% with history clustering.
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

% Initialize Nelder-Mead simplex optimizer with simulated annealing

%  Calculates and stores the function values at the vertices of the
%  simplex.  Any vertex that is in a singularity may be moved to a
%  safe nearby position (kavoid > 0).  Stores some useful constants
%  and parameters in global variables.  Should be followed by one
%  or more calls to nmasiter to carry out the actual optimization.
%  In deference to custom, the ufn function value is minimized--ufn
%  should return the complementary value if maximization is desired.
%
%  Arguments:
%     ufn      MATLAB handle to a user-supplied function,
%              ufn(xv,varargin), that is to be minimized, where xv
%              is a vector of length N, typically containing one
%              row of x.
%     x        Array (N,N+1) defining the starting simplex of N+1
%              points in N-dimensional space.  This exists in the
%              caller's workspace so it can be used to restart an
%              optimization that was halted prematurely, usually
%              to change the annealing temperature.
%     kavoid   Kind of avoidance when init or shrink hits negln:
%                 0     No special action.
%                 1     Try to move vertex away from singularity.
%                 2     Move C's away from 0 plus do action 1.
%     varargin Optional additional parameters to be passed to ufn.
%              These are not included in the optimization.
%
%  Globals (really used to remember local variables between calls):
%     kreport  Kind of reporting:
%                 0     No reports.
%                 1     Report current and best y value if
%                       convergence test passes.
%                 2     Same as 1, plus at each iteration, report
%                       method used, new low vertex and y value.
%                 3     Additional info on shrink and neglns.
%     nmasy    Vector (N+1,1) holding the function values at the
%              vertices of x.
%     nmasxb   Value of x at best y found in the series of calls.
%     nmasyb   Best value of y found in the series of calls.
%     nmstats  Statistics (counts of various special actions)
%              (1) Number of negln avoidances in nminit
%              (2) Number of negln avoidance moves in nminit
%              (3...) See nmiter
%
%  The following values are normally never touched, but can be
%  varied by the user between the nmasinit and nmasiter calls
%  if desired:
%     nmasrr   Reflection ratio (normally 1.0)
%     nmasxr   Expansion ratio (normally 2.0)
%     nmascr   Contraction ratio (normally 0.5)
%     nmassr   Shrinkage ratio (normally 0.5)
%     nmasri   X sums recalculation interval (normally 250)
%     nmasks   =0 to shrink around thermalized best vertex,
%              >0 to shrink around unthermalized best vertex.
%     nmasar   Avoidance ratio (used to avoid placing a simplex vertex
%              in a singularity (should be negative and a little less
%              than -1.0, normally -1.083333).  0 if kavoid = 0.
%
%  Return value:
%     x        Array (N,N+1) containing the (maybe updated) simplex.
%     yinfo    Vector (4,1) containing same 4 quantities as yhist
%              reported at last iteration, not summed over subnets.
%              1) Current replacement (or shrink origin) y value
%              2) Best ever y value
%              3) Best y value in current simplex
%              4) Worst y value in current simplex
%
%  Notes:
%     This function uses the Nelder-Mead simplex algorithm, with the
%  addition of simulated annealing as suggested in the book "Numerical
%  Recipes in C" (2nd Ed.) by WH Press, SA Teukolsky, WT Vetterling,
%  and BP Flannery, Cambridge University Press, 1992 with some ideas
%  for program organization borrowed from the fminsearch() program
%  (Nelder-Mead without simulated annealing) in MATLAB.
%     The division into separate initialization and iteration routines
%  allows an optimization to be continued with minimal startup cost
%  after a temperature change or other interruption.
%     When a new simplex is generated at each thermal step, it is
%  built around the best vertex in the previous simplex.  Therefore,
%  the "best-ever" y will be maintained automatically across steps.
%
%  Reference: Jeffrey C. Lagarias, James A. Reeds, Margaret H. Wright,
%  Paul E. Wright, "Convergence Properties of the Nelder-Mead Simplex
%  Method in Low Dimensions", SIAM Journal of Optimization, 9(1):
%  p.112-147, 1998.
%
%  V1A, 05/03/02, G.N. Reeke - New implementation
%  V3I, 10/08/02, GNR - Add nmasks
%  Rev, 10/17/02, GNR - Add kreport, nmasar and singularity fixup
%  V4A, 11/29/02, GNR - Remove multiple shrinks, ybest-->yinfo
%  Rev, 01/29/03, GNR - Handle different subnet termination times
%  Rev, 02/27/03, GNR - Add kavoid argument

global nmasy nmasxb nmasyb
global nmasrr nmasxr nmascr nmassr nmasri nmasks nmasar nmstats
global dfid h kreport nnegln                 %%% DEBUG %%%

nmasrr = 1.0;
nmasxr = 2.0;
nmascr = 0.5;
nmassr = 0.5;
nmasri = 250;
nmasks = 0;
if (kavoid > 0)
   nmasar = -1.083333;
   ar1 = (nmasar + 1.0)/nmasar;  % To get first dx = |nmasar| - 1.0
else
   nmasar = 0;
   ar1 = 0;
   end

if (ndims(x) ~= 2)
   error('x must be a 2-D matrix.')
   end
[N,V] = size(x);
if (N <= 2)
   error('x must have at least 2 variables (columns).')
   end
if (V ~= N + 1)
   error('x must have one more vertices than dimensions.')
   end

%  When ufn returns a d<=0 situation, if kavoid was TRUE, we move
%  the point in an expanding see-saw along the line to another
%  point picked at random until a valid point is found (hoping
%  this never gets us in an infinite loop).  Moving along lines
%  between points (without letting the points coalesce) maintains
%  the rank of the simplex.

nmasy = zeros(V,1);
onegln = nnegln;
for v=1:V
   nmasy(v) = feval(ufn,x(:,v),varargin{:});
   if (nnegln > onegln && nmasar ~= 0)
      % Pick an arbitrary vertex to move toward or away from
      v0 = fix((V-1.0001)*rand(1)) + 1;
      if (v0 == v) v0 = v0 + 1; end
      xv = x(:,v);
      dx = ar1 * (x(:,v0) - xv);
      nmoves = 0;
      while (nnegln > onegln)
         dx = nmasar * dx;
         onegln = nnegln;
         nmasy(v) = feval(ufn,xv+dx,varargin{:});
         nmoves = nmoves + 1;
         end % vertex movement loop
      x(:,v) = xv + dx;
      nmstats(1) = nmstats(1) + 1;
      nmstats(2) = nmstats(2) + nmoves;
      if (kreport > 2)
         fprintf(dfid, ['Iter %6d, CN(d<=0) for v = %5d set ', ...
            'to %8.5g after %d moves at nminit\n'], ...
            h, v, nmasy(v), nmoves);
         end
      end % singularity removal
   end % v loop

[ylo,vlo] = min(nmasy);
nmasxb = x(:,vlo);
nmasyb = ylo;
yinfo(1) = ylo;
yinfo(2) = ylo;
yinfo(3) = ylo;
yinfo(4) = max(nmasy);

