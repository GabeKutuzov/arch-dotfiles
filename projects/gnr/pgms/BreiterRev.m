% Test model for entropy sums in Breiter paper
N = 20;
pN = 1/N;
fprintf('    t-    H-    H+     r\n');
for t1=1:19 % 1 <= t1 <= N
t2 = t1+N;  % N < t2 <= 2*N
% Entropy on t1 side
i1 = floor(t1);
p1 = i1/N;
H1 = -(p1*log2(p1) + (N - i1)*pN*log2(pN));
% Entropy on t2 side
i2 = 2*N - ceil(t2);
p2 = i2/N;
H2 = -(p2*log2(p2) + (N - i2)*pN*log2(pN));
r = sqrt(H1*H1 + H2*H2);
fprintf('%6d%6.2f%6.2f%6.2f\n',t1, H1, H2, r);
end


