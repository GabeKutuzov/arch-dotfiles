function [entropy,tree_nodenumber,clusters,tree] = lin_reeke(hist_X,X_std,max_branch,sub_tree_minsize)

% $Id: lin_reeke.m 3 2014-02-27 21:23:39Z  $

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

% Compute spike-train entropy by the Lin & Reeke method

% INPUTS:
    % hist_X: a history matrix of ISIs
    % X_std: standard deviation cutoff for clusters
    % max_branch: maximum number of branches at any node of context tree
    % sub_tree_minsize: minimum number of points below any node in tree
% OUTPUTS:
    % entropy: the entropy estimation
    % tree_nodenumber: number of nodes in tree
    % clusters: the final clusters
    % tree: the context tree
% PROGRAMS CALLED:
    % k_means, LR_la

tree_depth = size(hist_X,1);
X = hist_X;

%% Build context tree

depth = tree_depth;
tree = cell(1, 4);
tree{1, 1} = X;
tree(1, 2:4) = {[0] [-1] [1]};
pin = 1;  % pin keeps track of row of tree
while 1
    if pin > size(tree, 1)
        break;
    end;
    sub_X = tree{pin, 1}; % The first column of tree is the data column
    layer = tree{pin, 4};
    if layer == depth
        pin = pin + 1;
        continue; 
    end
    sub_X_focus = sub_X(layer+1, :);

    if size(sub_X, 2) < sub_tree_minsize
        % fprintf(1,'there is not enough data to separate any more!\n');
        sub_tree = cell(1, 1);
        sub_tree{1, 1} = sub_X;
        break;
    end;
    if sum(std(sub_X(1,:))) < 1e-6 % Test for ridiculous clustering
        sub_tree = cell(1, 1);
        sub_tree{1, 1} = sub_X;
        break;
    end;
    sub_data = sub_X(1, :);
    [phat, ci] = gamfit(sub_data);

    % Do the clustering:
    for k=2:max_branch
        % Make tentative cluster points
        Z= [];
        X_prob = ones(1, size(sub_X_focus, 2));
        X_prob = X_prob ./ sum(X_prob);
        for ii=1:k
            ind=randi(numel(X_prob));
            Z = [Z, sub_X_focus(:, ind)];
                D = zeros(size(Z, 2), size(sub_X, 2));
                centroid = repmat(Z', 1, length(sub_X_focus));
                XX = repmat(sub_X_focus, length(Z), 1);
                D = (XX - centroid).^2;
                [D_sq, I] = min(D, [], 1);
                % D_sq is the distance between each element of X and the
                % closest element of Z
            X_prob = D_sq ./ sum(D_sq);
        end;
        ZZ = repmat(Z',1,size(sub_X,1));
        [I] = k_means(sub_X_focus',k,Z');

        sub_tree = cell(k,1);
        sub_tree_size=zeros(k,1);
        for ii=1:k
            sub_tree_size(ii) = sum(I==ii);
            sub_tree{ii,1} = sub_X(:,I==ii);
        end;

        if min(sub_tree_size) < sub_tree_minsize % Test for ridiculous clustering
                if k==2
                    sub_tree = cell(1, 1);
                    sub_tree{1, 1} = sub_X;
                    break;
                else
                    sub_tree = sub_tree_before;
                    sub_tree_size = sub_tree_size_before;
                    break;
                end
        end;

        sub_tree_before = sub_tree;
        sub_tree_size_before = sub_tree_size;
        % Fit gammas to sub_tree:
            dim = size(sub_tree, 1);
            sub_phat = zeros(dim, 2);
            for ii=1:dim
                M = sub_tree{ii, 1}(1, :);

                new_phat = gamfit(M);


                sub_phat(ii, :) = new_phat';
            end;

        a_min = ci(1, 1);
        a_max = ci(2, 1);
        sub_a = sub_phat(:, 1);
            C_std = zeros(k,1);
            for ii=1:k
                V_std = mean(std(sub_tree{ii, 1}, 0, 2));
                C_std(ii, 1) = mean(V_std);
            end;
        if max(C_std) < X_std
            break;
            % If there isn't too much deviation in a cluster (defined by the
            % input X_std), then keep going
        end
    end;

    % Now we have the sub-tree
    if size(sub_tree, 1) > 1
        m = size(tree, 1);
        n = size(sub_tree, 1);
        tree{pin, 3} = [m+1-pin:m+n-pin];
            % tree{pin, 3} goes from 1 to the number of clusters; it
            % bookkeeps the number of children of that node
        tmp_mat = [pin-(m+1):-1:pin-(m+n); ones(1, n)* -1;ones(1, n) * (layer+1)]';
        tmp_size = ones(1, n);
        sub_table = mat2cell(tmp_mat, tmp_size, [1 1 1]);
        tree = [tree; [sub_tree, sub_table]];  % Adding the next layer of nodes onto the tree
    end;
    pin = pin + 1;

end

%% Context tree built
pin = 1;
length_tree = LR_la(tree, pin);  % Calculate entropy from pin level down
entropy = length_tree{1, 1};  % This is the entropy

%% Now get some information about the tree
tree_nodenumber = numel(horzcat(length_tree{:,end}));
% Get the clusters:
nodelevels = [];
for i=1:size(tree,1)
    if tree{i,3} == -1;
    nodelevels = [nodelevels,i];
    end
end;
clusters = cell(1,numel(nodelevels));
for i=1:numel(nodelevels)
    clusters{1,i} = tree{nodelevels(i),1};
end

end
