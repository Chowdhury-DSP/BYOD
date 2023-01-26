import numpy as np
from scipy.io import wavfile
import scipy.signal as signal
import matplotlib.pyplot as plt
import audio_dspy as adsp

def freqSmooth(x, sm=1.0/24.0):
    s = sm if sm > 1.0 else np.sqrt(2.0**sm)
    N = len(x)
    y = np.zeros_like(x)
    for i in range(N):
        i1 = max(int(np.floor(i/s)), 0)
        i2 = min(int(np.floor(i*s)+1), N-1)
        if i2 > i1:
            y[i] = np.mean(x[i1:i2])
    return y

def plot_fft(sig, fs, gain=1):
    b,a = adsp.design_highshelf(1000, 0.7071, gain, fs)
    xx = signal.lfilter(b, a, sig)

    freqs = np.fft.rfftfreq(len(xx), 1.0 / fs)
    fft = freqSmooth(np.fft.rfft(xx), 1.0)
    plt.semilogx(freqs, 20 * np.log10(np.abs(fft)))

fs, x = wavfile.read("noise_48k.wav")
print(fs)
plot_fft(x, fs)

fs, x = wavfile.read("noise_96k.wav")
print(fs)
plot_fft(x, fs) #, 1 / 2)

fs, x = wavfile.read("noise_192k.wav")
print(fs)
plot_fft(x, fs) #, 1 / 4)

plt.xlim(20, 28000)
plt.show()
