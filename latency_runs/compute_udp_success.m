formatSpec = '%f';

id = fopen('UDP100.txt','r');
udp100 = fscanf(id,formatSpec);

id = fopen('UDP1K.txt','r');
udp1k = fscanf(id,formatSpec);

id = fopen('UDP10K.txt','r');
udp10k = fscanf(id,formatSpec);

percent100 = (length(udp100)/100)*100;
percent1k = (length(udp1k)/1000)*100;
percent10k = (length(udp10k)/10000)*100;

bar([percent100, percent1k, percent10k]);
set(gca,'XTick',1:3,'XTickLabel',{'100','1000','10000'});
title("Percent of UDP requests that recieved successful response");
xlabel("Number of clients");
ylabel("Percent success rate");
print("udp-success-chart", "-dpng");





