[x, Fs]=audioread("bin\Debug\sin.wav");

freq = 20;
width = 0.002;

fig_title = "vibrato width=" + width + "s, vibrato frequency=" + freq + "Hz";

y = vibrato(x, Fs, freq, width);

dt = 1/Fs;
r = min(length(y),50000);
t = 0:dt:(r*dt)-dt;
subplot(4,1,1);
plot(t,y(1:r,1));
xlabel('Seconds'); ylabel('Amplitude');
title(fig_title + ", matlab");

subplot(4,1,2);
[x_e, ~]=audioread("bin\Debug\out.wav");
plot(t, x_e(1:r,1));
xlabel('Seconds'); ylabel('Amplitude');
title(fig_title + ", c++");

diff = y(1:r,1) - x_e(1:r,1);
subplot(4,1,3);
plot(t, diff(1:r,1));
xlabel('Seconds'); ylabel('Amplitude');
title(fig_title + ", difference");

subplot(4,1,4);
% s = spectrogram(diff);
% imagesc(abs(s));
spectrogram(diff(1:r,1),hamming(1024),512,1024,Fs,'yaxis');
colormap hot;
ax = gca;
ax.YScale = "log";
title(fig_title + ", difference FFT");
