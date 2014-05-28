function ballplot(data)
%params=3,4,5
%load=6
%7... "Blocking probability";"Bandwidth blocking probability";"Sharability";"Fragmentation";"Spectrum Utilization"
c_cut=data(:,3);
c_algn=data(:,4);
c_sep=data(:,5);
minsz=4;
maxsz=14;
szfac=maxsz-minsz;

figure(1);
d=data(:,7);
scatter3(c_algn,c_cut,c_sep,minsz+szfac*(1-((d-min(d))./(max(d)-min(d))).^2),d,"filled");
colorbar;
title("Blocking Probability");
xlabel("Misalignment");
ylabel("Spectrum cuts");
zlabel("Separation");

figure(2);
d=data(:,9);
scatter3(c_algn,c_cut,c_sep,minsz+szfac*((d-min(d))./(max(d)-min(d))).^2,d,"filled");
colorbar;
title("Sharability");
xlabel("Misalignment");
ylabel("Spectrum cuts");
zlabel("Separation");

figure(3);
d=data(:,10);
bs=64;
scatter3(c_algn,c_cut,c_sep,minsz+szfac*(1-((d-min(d))./(max(d)-min(d))).^2),d,"filled");
colorbar;
title("Fragmentation");
xlabel("Misalignment");
ylabel("Spectrum cuts");
zlabel("Separation");

endfunction
