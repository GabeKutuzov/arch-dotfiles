% $Id: demo.m 7 2014-06-12 21:51:55Z  $

%  This software was developed in the Laboratory of Biological
% Modelling at The Rockefeller University by Nicholas D. Watters
% and George N. Reeke

% (c) The Rockefeller University, 2013

% History Clustering Entropy Demo
%  This file runs the history clustering spike-train entropy package
% using the parameters in the distributed main program hcentropy.m
% and the publicly available cat visual cortex data set used in the
% Neural Computation paper [submitted] as documented below.

% LICENSE:
%  This software is distributed under GPL, version 2.
% This program is free software; you can redistribute it and/or modify it
% under the terms of the GNU General Public License as published by the
% Free Software Foundation; either version 2 of the License, or (at your
% option) any later version.  Accordingly, this program is distributed in
% the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
% the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
% PURPOSE.  See the GNU General Public License for more details.  You
% should have received a copy of the GNU General Public License along
% with this program.  If not, see <http://www.gnu.org/licenses/>.

% See README file for revision history.

% WHAT THIS DEMO DOES:
%  This script reads the data file 't64.spk' from the 'data' subdirectory
% of the history clustering distribution folder, converts it to a Matlab
% interspike interval array according to the instructions provided with
% the online data, and then runs hcentropy() on that data set.  A plot
% of the history clusters is generated.  Entropy is calculated and plot-
% ted as a function of data set size by the methods history clustering,
% Strong et al., direct bin (Shannon), Kozachenko & Leonenko, and Ma
% bound.  The run time is displayed.  The number of bootstraps, 10000
% in the Neural Computation paper, has here been reduced to 200 and the
% artificial data method has been turned off to allow the demo to run
% in a reasonable time (around 10 min. on an 0.8 GHz laptop.  If the
% artificial data method is turned on without changing any other
% parameters (set APPROXEXPER = 1 in hcentropy.m), the time on this
% machine is about 1.25 hrs.
%
%  The "printed" output you get should be similar to that in the file
% DemoOutput.txt except for small differences due to use of different
% random number seeds.
%
%  The plots you get should match the files DemoFig1.jpg and
% DemoFig2.jpg found in the distribution directory.  Some of the
% methods give negative discrete entropies for low N, which is not a
% physical result.  These values are artifacts of undersampling the
% data and do not appear with the history clustering method.

% IMPORTANT NOTES:
%  (1)  The bin resolution for computation of the discrete entropy has
% been set in the hcentropy.m main program to use the mean ISI, which
% is around 22 msec. for the cat visual cortex data used in the demo.
% Use of the mean ISI as the bin resolution provides one way to esti-
% mate the entropy in bits/spike.  However, it should be understood
% that this value is quite arbitrary and depends on what the user con-
% siders is the time resolution of whatever structure is receiving the
% experimental spike train; some might consider this to be equivalent
% to the minimum observed ISI in the data (about 0.59 msec after dele-
% ting 6 apparent outliers with ISI < 0.07 msec. for the data used in
% the demo).  With this latter choice, about 37 resolution intervals
% occur between average spikes and the entropy per spike would be 37
% times the entropy per bin.  The 0.59 msec minimum ISI in the data
% set is smaller than the usually expected neuronal refractory period,
% and so may be questionable; furthermore, the relevant variable
% 'bin_resolution' is the time resolution of the structure that
% receives the spike train, which may or may not have any any
% relation to the neuronal refractory period or the minimum ISI.
%
%  (2) In the hcentropy.m file as supplied, the computation of the
% synthetic data approximation to the experimental data has been
% turned off because the execution time would be excessive for
% purposes of the demo.  The parameters in this area (flseq,
% nelder_reps, etc.) were different for the different data sets
% used in the Neural Computation manuscript.  The values in the
% distributed version should be suitable for most data sets but
% the user is urged to experiment with these parameters.

% DATA SOURCE:
%  The cat visual cortex data were downloaded from the public data
% repository CRCNS.org.  Publications created through the use of
% these data should cite the data source in the following
% recommended format:
% Blanche, Tim (2009): Multi-neuron recordings in primary visual
% cortex.  CRCNS.org.  http://dx.doi.org/10.6080/K0MW2F2J
%
% The specific file was, after unzipping crcns_pvc3_cat_recordings.zip:
% crcns_pvc3_cat_recordings/spont_activity/spike_data_area18/t64.spk

% TERMS OF USE OF THE CAT VISUAL CORTEX DATA:
%  These data are provided free of charge and without warranty. There are no
% restrictions placed on its use, however if the data are used in a published
% academic work or for teaching purposes, both the Data Sharing Initiative and
% the relevant laboratory where the data were obtained must be cited in the
% Methods or Acknowledgement section. For the cat data, the appropriate
% attribution is:  Neural data were recorded by Tim Blanche in the laboratory
% of Nicholas Swindale, University of British Columbia, and downloaded from
% the NSF-funded CRCNS Data Sharing website.

% REFERENCES PROVIDED BY THE DATA DEPOSITORS:
%  A fuller description of the polytrodes and experimental methods can be
% found in these publications:

% Blanche TJ, Spacek MA, Hetke JF, Swindale NV (2005) Polytrodes: high density
% silicon electrode arrays for large scale multiunit recording. J. Neurophys.
% 93 (5): 2987-3000.

% Blanche TJ & Swindale NV (2006) Nyquist interpolation improves neuron yield
% in multiunit recordings. J. Neuro. Methods 155 (1): 81-91.

% Blanche TJ, Godfrey K, Douglas RM & Swindale NV (2008) Spike sorting for
% polytrodes. J. Neurophys. submitted.

% Mitzdorf U, Singer W. (1978) Prominent excitatory pathways in the cat visual
% cortex (A 17 and A 18): a current source density analysis of electrically
% evoked potentials. Exp Brain Res.; 33: 371-94.

% Nicholson C, Freeman JA. (1975) Theory of current source-density analysis
% and determination of conductivity tensor for anuran cerebellum.
% J. Neurophys.; 38: 356-68.

% Wegener D, Freiwald WA, Kreiter AK (2004) The influence of sustained
% selective attention on stimulus selectivity in macaque area MT.
% J Neurosci 24: 6106-6114

%% Read the spike times from the data file
filename = './data/t64.spk';
fh = fopen(filename, 'r');
st = fread(fh, inf, 'int64');
fclose(fh);

% Scale microseconds to milliseconds
st = 1E-3*st;

% Compute interspike intervals
X = st(2:end) - st(1:end-1);

fprintf('Spike times read: %d\n', numel(st));
fprintf('The minimum ISI is %f\n', min(X));
fprintf('The mean ISI is %f\n', mean(X));
fprintf('The maximum ISI is %f\n', max(X));

% Perform (and time) the analysis
tic;
[~, ~, ~, ~, ~, ~, ~] = hcentropy(X');
toc
