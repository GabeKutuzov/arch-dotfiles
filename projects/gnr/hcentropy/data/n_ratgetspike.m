function [spikevec] = n_ratgetspike(X,framenum)

%X is the voltage level matrix

lengthframe = floor(numel(X)/framenum);
spikevec = [];
for i = 1:framenum
    sub_X = X(lengthframe*(i-1)+1:lengthframe*i-1);
    maxval = max(sub_X);
    cutoff = maxval*2/3;
    highpoints = find(sub_X>cutoff);
    highpointnum = numel(highpoints);
    spike_heights = zeros(size(sub_X));
    if highpointnum == 1 %if only one spike which is only one point wide 
        spike_heights(highpoints) = sub_X(highpoints);
    else %if more than one spike, or if more than one point width to one spike
        for k = 1:(highpointnum + 1)
            if k == 1 % for the first hight point
                current_max_point = highpoints(1);
                current_max = sub_X(highpoints(1));
            elseif k == highpointnum + 1  % no more high points to consider  
                spike_heights(current_max_point) = current_max;
            elseif highpoints(k) == (highpoints(k-1)+1) %if still looking at the same bump
                if sub_X(highpoints(k)) > current_max %update max point if we're higher than last max for this bump
                    current_max_point = highpoints(k);
                    current_max = sub_X(highpoints(k));   
                end
            else % if looking at a new bump  %MODIFIED THIS TO REMOVE BUG ON 06/29/01
                spike_heights(current_max_point) = current_max; %create spike height from last bump
                current_max = sub_X(highpoints(k));
                current_max_point = highpoints(k);
            end
        end
    end
    spikespots = find(spike_heights~=0);
    spikevec = [spikevec,spikespots'+(i-1)*lengthframe];
    
end
    





end

