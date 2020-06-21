function [length_tree] = LR_la(tree, pin)

% $Id: LR_la.m 3 2014-02-27 21:23:39Z  $

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

% Return entropy and other information about the tree in the Lin & Reeke
    % method

% INPUTS:
    % tree: the context tree
    % pin: the level of the context tree to be analyzed
% OUTPUTS:
    % length_tree: information about the tree, including entropy
% PROGRAMS CALLED:
    % LR_la (recursively), gamfit


if tree{pin, 3} == -1 % leaf
    layer = tree{pin, 4};
    code = tree{pin, 1};
    N = size(code, 2);
    length_tree = cell(1, 5);

    % Calculate the entropy using the formula, using an MLE
    % approximation for the parameters of the gamma function that best
    % approximates leaf data

    if size(code(1,:), 2) == 1
        length_tree{1, 1} = 0;
        return;
    end;

    SD = code(1,:);
    % Fit a gamma distribution:
    phat = gamfit(SD);
    if phat(1, 2) == 0
        length_tree{1, 1} = 0;
        return;
    end;
    % Calculate the entropy for the fitted gamma distribution:
    phattemp = phat';
    a = phattemp(1,:);
    b = phattemp(2,:);
    length_tree{1, 1} = (a+log(b)+log(gamma(a))+(1-a).*psi(a))./log(2);
    if length_tree{1, 1} == Inf
        %fprintf(1, 'Data degraded too much for fitting, entropy set to 0\n')
        length_tree{1, 1} = 0;
    end;
    if isnan(length_tree{1, 1}) == 1
        error('Invalid entropy!')
    end;

    if length_tree{1, 1} == Inf
        code(1,:)
        error('Invalid entropy');
    end;

    length_tree{1, 2} = N;
    length_tree{1, 3} = 0;
    length_tree{1, 4} = -1;
    length_tree{1, 5} = layer;
else % not a leaf
    layer = tree{pin, 4};
    children_book = tree{pin, 3};
    length_tree = cell(1, 5);
    length_tree{1, 3} = 0;
    length_tree{1, 4} = children_book;
    length_tree{1, 5} = layer;
    num_children = size(children_book, 2);
    H = zeros(num_children, 1);
    N = zeros(num_children, 1);
    
    for child_no=1:num_children
        child_offset = children_book(1, child_no);
        child_length_tree = LR_la(tree, pin+child_offset);
        H(child_no, 1) = child_length_tree{1, 1};
        N(child_no, 1) = child_length_tree{1, 2};
        booklength = size(length_tree, 1);
        child_length_tree{1, 3} = -booklength;
        tmp_children_book = length_tree{1, 4};
        tmp_children_book = [tmp_children_book booklength];
        length_tree{1, 4} = tmp_children_book;
        length_tree = [length_tree; child_length_tree];
    end;
    HN = H .* N;
    length_tree{1, 1} = sum(HN)/sum(N);
    length_tree{1, 2} = sum(N);
end
