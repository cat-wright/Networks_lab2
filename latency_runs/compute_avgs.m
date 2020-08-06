formatSpec = '%f';

TCP_mean = [];
UDP_mean = [];

id = fopen('TCP100.txt','r');
A = fscanf(id,formatSpec);
TCP_mean(1) = mean(A);

id = fopen('TCP1K.txt','r');
A = fscanf(id,formatSpec);
TCP_mean(2) = mean(A);

id = fopen('TCP10K.txt','r');
A = fscanf(id,formatSpec);
TCP_mean(3) = mean(A);

id = fopen('UDP100.txt','r');
A = fscanf(id,formatSpec);
UDP_mean(1) = mean(A);

id = fopen('UDP1K.txt','r');
A = fscanf(id,formatSpec);
UDP_mean(2) = mean(A);

id = fopen('UDP10K.txt','r');
A = fscanf(id,formatSpec);
UDP_mean(3) = mean(A);

disp("TCP averages (100, 1k, 10k): ");
disp(TCP_mean);
disp("UDP averages (100, 1k, 10k): ");
disp(UDP_mean);


plot(TCP_mean);
set(gca,'XTick',1:3,'XTickLabel',{'100','1000','10000'});
hold on;
plot(UDP_mean);
% ylim([0 5]);
legend("TCP", "UDP");
title("Average latecy between requests");
xlabel("Number of clients");
ylabel("Time (seconds)");
print("latency-chart", "-dpng");

