function [x,xbest,yinfo,niter] = nmiter4a(ufn,x,ftol,xtol, ...
   mxiter,T,varargin)

% $Id: nmiter4a.m 2 2013-11-07 20:45:56Z  $

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

% Perform Nelder-Mead simplex optimization with simulated annealing

%  Minimizes the function ufn beginning with the starting guess x.
%  nminit4a() must be called first to initialize relevant globals.
%  (Most of the globals have names starting with nmas to minimize
%  confusion with application globals.)
%
%  Arguments:
%     ufn      MATLAB handle to a user-supplied function,
%              ufn(xv,varargin), that is to be minimized, where xv
%              is a vector of length N, typically containing one
%              row of x.
%     x        Array (N,N+1) defining the starting simplex of N+1
%              points in N-dimensional space.  The updated matrix
%              is returned so it can be used to restart an optimi-
%              zation that was halted prematurely, usually to
%              change the annealing temperature.
%     ftol     Fractional tolerance in y value. When the magnitude
%              of the difference between the largest and smallest
%              values of y at vertices of the simplex becomes less
%              than ftol times the mean of the magnitudes of those
%              values of y, the optimization terminates. (Caution:
%              This is not necessarily a very good test.)
%     xtol     Absolute tolerance in radius of simplex.  After a
%              shrink only (to save time), when difference of x
%              coords at lowest vs highest y is less than xtol,
%              the optimization terminates.  The integer part of
%              this variable encodes whether yhist values should
%              be saved (1) or not (0).
%     mxiter   Maximum number of iterations.  When this number of
%              iterations is reached, the optimization terminates.
%              (MATLAB fminsearch() default is 200*N.)
%     T        Annealing temperature for this series of iterations.
%              N.B.  Original article adds -T*log(rand) to each y
%              value.  This program multiples y by (1-T*log(rand)).
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
%              (3) Number of negln on reflection point
%              (4) Number of negln on expansion point
%              (5) Times expansion was best ever
%              (6) Times expansion was accepted
%              (7) Times reflection was best ever
%              (8) Times reflection was accepted
%              (9) Number of negln on outside contraction
%              (10) Times outside contraction was best ever
%              (11) Times outside contraction was accepted
%              (12) Number of nnegln on inside contraction
%              (13) Times inside contraction was best ever
%              (14) Times inside contraction was accepted
%              (15) Times shrinkage was attempted
%              (16) Number of negln avoidances in shrink
%              (17) Number of negln avoidance moves in shrink
%              (18) Times shrinkage was best ever
%              (19) Times quit due to shrinkage to a point
%              (20) Number of times convergence was reached.
%     yhist    Matrix (4,global max iterations) to contain:
%              1) Current replacement (or shrink origin) y value
%              2) Best ever y value
%              3) Best y value in current simplex
%              4) Worst y value in current simplex
%              This array should be initialized to zeros by the
%              caller to allow accumulation over multiple subnets.
%  The following values are normally never touched, but can be
%  varied by the user between the nminit4a and nmiter4a calls
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
%  Return values:
%     x        Array (N,N+1) containing the updated simplex.
%              Due to the action of thermalization, this may
%              not necessarily contain the best solution ever.
%              (Swapping the best ever with the best in x can
%              in principal produce a singularity, so don't.)
%     xbest    Vector (N,1) containing the x vector at the
%              lowest function value obtained in the search.
%     yinfo    Vector (4,1) containing same 4 quantities as
%              yhist, relevant to last iteration performed.
%     niter    Exit information:
%                 >0    Solution converged after fix(niter)
%                       iterations.  The reason is encoded in
%                       the fraction:  0 if tolerance condition
%                       met, 0.1 if simplex shrank to a point.
%                 0     Solution did not converge, mxiter count
%                       was reached.
%                 <0    Some other error occurred.
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
%  after a temperature change or other interruption.  The sums of the
%  vertex coords are recalculated at 250 iteration intervals to avoid
%  accumulation of round-off errors.  This could probably be longer.
%  In general, code is repeated where necessary to avoid extra 'if's.
%     The idea of repeating a shrink operation until the worst point
%  stops improving or the best point moves to new vertex, the
%  occasional recalc of the running total xsums, and the application
%  of thermal noise as a multiplier rather than an addition are all
%  original with this GNR implementation.
%
%  Reference: Jeffrey C. Lagarias, James A. Reeds, Margaret H. Wright,
%  Paul E. Wright, "Convergence Properties of the Nelder-Mead Simplex
%  Method in Low Dimensions", SIAM Journal of Optimization, 9(1):
%  p.112-147, 1998.
%
%  V1A, 05/03/02, G.N. Reeke - New implementation
%  V3I, 10/08/02, GNR - Add nmasks; option to shrink around best
%                 unthermalized vertex and to iterate shrink until
%                 worst point stops improving or best point moves to
%                 a different vertex; record yhi,yb,ylo,yhi in yhist;
%                 catch one rare case where xb,yb were not saved;
%                 write kreport=2,3 output to debug file.
%  Rev, 10/19/02, GNR - Never allow a singular point in the simplex.
%  V4A, 11/29/02, GNR - Remove multiple shrinks, ybest-->yinfo,
%                 accumulate yhist over subproblems.
%  Rev, 01/29/03, GNR - Handle different subnet termination times.
%  Rev, 03/28/03, GNR - Add exit criterion code to niter.
%  Rev, 07/07/03, GNR - Define meaning of integer part of xtol.

global h nmasy nmasxb nmasyb yhist
global nmasrr nmasxr nmascr nmassr nmasri nmasks nmasar nmstats
global dfid kreport nnegln                   %%% DEBUG %%%

SaveYhist = fix(xtol);           % Isolate yhist control
xtol = xtol - SaveYhist;

%  A few local variables to optimize the math
[N,V] = size(x);
hftol = 0.5 * ftol;
xtol2 = xtol * xtol;
rr1 = nmasrr;     rr0 = (1 + rr1)/N;
xr1 = nmasxr*rr1; xr0 = (1 + xr1)/N;
oc1 = nmascr*rr1; oc0 = (1 + oc1)/N;
ic1 = -nmascr;    ic0 = (1 + ic1)/N;
sr1 = nmassr;
if (nmasar)
   ar1 = (nmasar + 1.0)/nmasar;
else
   ar1 = 0;
   end

%  Define codes for kind of move
km_shrink       = 1;
km_reflect      = 2;
km_expand       = 3;
km_contract_out = 4;
km_contract_in  = 5;
km_names = char('shrink', 'reflect', 'expand', ...
   'contract_out', 'contract_in');

htest = h + mxiter;
nxsri = 0;
[ybot,vbot] = min(nmasy);
ytop        = max(nmasy);
dostop = 0;
while (h < htest)

%  If it is time to recalculate the x-sums, do it now.

   if (nxsri == 0)
      xsums = sum(x,2);
      nxsri = nmasri;
   else
      nxsri = nxsri - 1;
      end

%  Determine which vertex is the highest (worst), next-
%  highest, and lowest (best) after thermal fluctuation.
%  The function values in nmasy are left uncontaminated.

   vlo = 1;
   vhi = 2;
   ylo = nmasy(1)*(1.0 - T*log(rand));
   ynhi = ylo;
   yhi = nmasy(2)*(1.0 - T*log(rand));
   if (ylo > yhi)
      vhi = 1; vlo = 2;
      ynhi = yhi; yhi = ylo; ylo = ynhi;
      end
   for v=3:V
      yt = nmasy(v)*(1.0 - T*log(rand));
      if (yt <= ylo)
         vlo = v;
         ylo = yt;
      elseif (yt > yhi)
         ynhi = yhi;
         vhi = v;
         yhi = yt;
      elseif (yt > ynhi)
         ynhi = yt;
         end
      end

%  In the following examination of various test points, keep in
%  mind that xsums holds the sums of all N+1 vertices, but the
%  worst one must always be subtracted to get the correct xbar.

   xhi = x(:,vhi);               % x at high vertex
   xsn = xsums - xhi;            % Sum of x at other vertices
   km = km_shrink;               % Default kind of move is shrink

   %  Compute the reflection point
   xr = rr0*xsn - rr1*xhi;
   onegln = nnegln;
   yr = feval(ufn,xr,varargin{:});
   yrbad = nnegln > onegln;
   if (yrbad)
      nmstats(3) = nmstats(3) + 1;
      if (kreport > 2)                                   % DEBUG %
         tracknegs(yr,vlo,vhi,ylo,ynhi,yhi,km_reflect);  % DEBUG %
         end                                             % DEBUG %
      end
   if (~yrbad & yr < ylo)

%  Range 1:  ytrial < ylo

      %  Compute the expansion point
      xe = xr0*xsn - xr1*xhi;
      onegln = nnegln;
      ye = feval(ufn,xe,varargin{:});
      yebad = nnegln > onegln;
      if (yebad)
         nmstats(4) = nmstats(4) + 1;
         if (kreport > 2)                                % DEBUG %
            tracknegs(ye,vlo,vhi,ylo,ynhi,yhi,km_expand);% DEBUG %
            end                                          % DEBUG %
         end
      if (~yebad & ye < yr)
         %  Accept the expansion point.
         thisy = ye;
         if (ye < nmasyb)        % Save the best ever
            nmstats(5) = nmstats(5) + 1;
            nmasyb = ye;
            nmasxb = xe;
            end
         if (ye*(1.0 + T*log(rand)) < yhi)
            nmstats(6) = nmstats(6) + 1;
            nmasy(vhi) = ye;
            xsums = xsn + xe;
            x(:,vhi) = xe;
            end
         km = km_expand;
      else
         %  Accept the reflection point
         thisy = yr;
         if (yr < nmasyb)        % Save the best ever
            nmstats(7) = nmstats(7) + 1;
            nmasyb = yr;
            nmasxb = xr;
            end
         if (yr*(1.0 + T*log(rand)) < yhi)
            nmstats(8) = nmstats(8) + 1;
            nmasy(vhi) = yr;
            xsums = xsn + xr;
            x(:,vhi) = xr;
            end
         km = km_reflect;
         end

   elseif (~yrbad & yr < ynhi)

%  Range 2:  ylo <= ytrial < ynhi

      %  Accept the reflection point
      thisy = yr;
      if (yr < nmasyb)           % Save the best ever
         nmstats(7) = nmstats(7) + 1;
         nmasyb = yr;
         nmasxb = xr;
         end
      if (yr*(1.0 + T*log(rand)) < yhi)
         nmstats(8) = nmstats(8) + 1;
         nmasy(vhi) = yr;
         xsums = xsn + xr;
         x(:,vhi) = xr;
         end
      km = km_reflect;

   else
      if (yr < yhi)

%  Range 3:  ynhi <= ytrial < yhi

         %  Calculate the outside contraction point
         xc = oc0*xsn - oc1*xhi;
         onegln = nnegln;
         yc = feval(ufn,xc,varargin{:});
         ycbad = nnegln > onegln;
         if (ycbad)
            nmstats(9) = nmstats(9) + 1;
            if (kreport > 2)                             % DEBUG %
               tracknegs(yc,vlo,vhi,ylo,ynhi,yhi,km_contract_out);
               end                                       % DEBUG %
            end
         if (~ycbad & yc <= yr)
            %  Accept the outside contraction point
            thisy = yc;
            if (yc < nmasyb)     % Save the best ever
               nmstats(10) = nmstats(10) + 1;
               nmasyb = yc;
               nmasxb = xc;
               end
            if (yc*(1.0 + T*log(rand)) < yhi)
               nmstats(11) = nmstats(11) + 1;
               nmasy(vhi) = yc;
               xsums = xsn + xc;
               x(:,vhi) = xc;
               end
            km = km_contract_out;
            end

      else

%  Range 4:  ytrial >= yhi

         % Calculate the inside contraction point
         xc = ic0*xsn - ic1*xhi;
         onegln = nnegln;
         yc = feval(ufn,xc,varargin{:});
         ycbad = nnegln > onegln;
         if (ycbad)
            nmstats(12) = nmstats(12) + 1;
            if (kreport > 2)                             % DEBUG %
               tracknegs(yc,vlo,vhi,ylo,ynhi,yhi,km_contract_in);
               end                                       % DEBUG %
            end
         if (~ycbad & yc < yhi)
            %  Accept the inside contraction point
            thisy = yc;
            if (yc < nmasyb)     % Save the best ever
               nmstats(13) = nmstats(13) + 1;
               nmasyb = yc;
               nmasxb = xc;
               end
            if (yc*(1.0 + T*log(rand)) < yhi)
               nmstats(14) = nmstats(14) + 1;
               nmasy(vhi) = yc;
               xsums = xsn + xc;
               x(:,vhi) = xc;
               end
            km = km_contract_in;
            end
         end
      end

   %  Shrink if no other move was accepted.  It is probably faster
   %  to swap the low point into the low position, then shrink all
   %  the rest, rather than have a test for v==vlo inside the loop.
   %  If nmasks > 0, shrink around the actual lowest point rather
   %  than the fluctuated lowest point.  The y value reported is
   %  the best value (the one not replaced).  This is consistent
   %  with the other cases in that it is the one unique point on
   %  the simplex that was treated differently from the rest.
   %  This value sticks out in any plot of yhist vs h, marking the
   %  shrinks.  Test code in V3I that shrank multiple times until
   %  certain criteria were met has been removed in V4A: Shrinking
   %  is very expensive and it seems best to repeat all the vertex
   %  tests each time rather than shrink with the simplified tests.

   if (km == km_shrink)
      nmstats(15) = nmstats(15) + 1;
      if (nmasks > 0) vlo = vbot; end
      xlo = x(:,vlo);
      x(:,vlo) = x(:,1);
      x(:,1) = xlo;
      thisy = nmasy(vlo);
      nmasy(1) = thisy;
      onegln = nnegln;        % For singularity test
      for v=2:V
         xc = xlo + sr1*(x(:,v) - xlo);
         nmasy(v) = feval(ufn,xc,varargin{:});
         if (nnegln > onegln && nmasar ~= 0)
            dyinfox = ar1 * (xlo - xc);
            nmoves = 0;
            while (nnegln > onegln)
               dx = nmasar * dx;
               onegln = nnegln;
               nmasy(v) = feval(ufn,xc+dx,varargin{:});
               nmoves = nmoves + 1;
               end % vertex movement loop
            xc = xc + dx;
            nmstats(16) = nmstats(16) + 1;
            nmstats(17) = nmstats(17) + nmoves;
            if (kreport > 2)
               fprintf(dfid, ['Iter %6d, CN(d<=0) for v = %5d ', ...
                  'set to %8.5g after %d moves at shrink\n'], ...
                  h, v, nmasy(v), nmoves);
               end
            end % singularity removal
         x(:,v) = xc;
         end % vertex loop
      %  Update the x sums
      xsums = sum(x,2);
      nxsri = nmasri;
      %  Update the current best and best ever solutions
      [ybot,vbot] = min(nmasy);
      ytop        = max(nmasy);
      if (ybot < nmasyb)
         nmstats(18) = nmstats(18) + 1;
         nmasxb = x(:,vbot);
         nmasyb = ybot;
         end
      %  Calculate simplex radius and set dostop if very small
      xrel = x - repmat(xsums/V,1,V);
      xr2 = max(sum(xrel .* xrel));
      if (xr2 < xtol2)
         nmstats(19) = nmstats(19) + 1;
         dostop = 1;
         end

      if (kreport > 2)                                % DEBUG %
         xrad = sqrt(xr2);                            % DEBUG %
         fprintf(dfid, ['At iter %d, simplex radius ', ...
            'is %8.5g after shrink\n'], h, xrad);     % DEBUG %
         end                                          % DEBUG %
   else
      ybot = min(nmasy);
      ytop = max(nmasy);
      end

   h = h + 1;
   yinfo = [thisy; nmasyb; ybot; ytop];
   if (SaveYhist)
      yhist(:,h) = single(double(yhist(:,h)) + yinfo);
      end

   if (kreport > 1)
      fprintf(dfid, ['Iter %6d, low vtx = %5d, ', ...
         'min, max, this y value = %8.5g, %8.5g, %8.5g, ', ...
         'method = %s\n'], h, vlo, ybot, ytop, thisy, ...
         km_names(km,:));
      end

%  Check for convergence.  Note that if the best and worst points
%  have opposite signs, the y test will never succeed.  Unlike
%  Press et al, I test on the unthermalized values of y, in order
%  to detect when the simplex shrinks close to a single point,
%  even early in the annealing process.  The test on the x values
%  (dostop set) is performed only after a shrink cycle, because
%  it is rather expensive.  I also always do at least one cycle
%  before testing in order to avoid infinite call loops.

   if (dostop | (abs(ytop - ybot) < hftol * (abs(ytop) + abs(ybot))))
      nmstats(20) = nmstats(20) + 1;
      if (kreport > 0)
         fprintf(dfid, 'Iter %6d, convergence test %d passed\n', ...
            h, dostop);
         end
      break;
      end

   end   % h loop

if (h == htest)
   niter = 0;
else
   niter = h + 0.1*dostop;
   end
xbest = nmasxb;

%----------------------------------------------------------------------%
%                           DEBUG FUNCTION                             %
%----------------------------------------------------------------------%

function [] = tracknegs(y,vlo,vhi,ylo,ynhi,yhi,km)
% Output current state when get a negative determinant

global h nnegln dfid

km_names = char('shrink', 'reflect', 'expand', ...
   'contract_out', 'contract_in');
fprintf(dfid, ['Iter %6d, CN(d<=0) = %8.5g, vlo = %5d, ', ...
   'vhi = %5d\n'], h, y, vlo, vhi);
fprintf(dfid, ['   ylo = %8.5g, ynhi = %8.5g, yhi = %8.5g, ', ...
   'method = %s\n'], ylo, ynhi, yhi, km_names(km,:));

