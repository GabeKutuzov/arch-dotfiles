% Plot phases for Molter, Yamaguchi paper
rtod = pi/180.0;
alpha = .2/rtod;
phiMEC = 0:10:720;
phiDG = zeros(73,1);
for i=2:60
phiDG(i) = phiDG(i-1) + 10 - alpha*cos(rtod*phiMEC(i))*sin(rtod*phiDG(i-1));
end
plot(phiMEC);
hold all
plot(phiDG);

