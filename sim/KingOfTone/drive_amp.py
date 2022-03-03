# %%
import numpy as np
import scipy.signal as signal
from scipy.special import comb
import matplotlib.pyplot as plt
import audio_dspy as adsp

# %%
C4 = 100e-12
C5 = 0.01e-6
C6 = 0.01e-6
C7 = 0.1e-6
R6 = 10e3
R7 = 33e3
R8 = 27e3
R9 = 10e3
R10 = 220e3
Rp = 100e3

# %%
def calc_coefs(t):
    Rd = R6 + Rp * np.power(t, 1.5)

    R1_b2 = C5 * C6 * R7 * R8
    R1_b1 = C5 * R7 + C6 * R8
    R1_b0 = 1
    R1_a2 = C5 * C6 * (R7 + R8)
    R1_a1 = 0
    R1_a0 = C5 + C6

    R2_b0 = Rd
    R2_a1 = C4 * Rd
    R2_a0 = 1

    a3 = R1_b2 * R2_a1
    a2 = R1_b1 * R2_a1 + R1_b2 * R2_a0
    a1 = R1_b0 * R2_a1 + R1_b1 * R2_a0
    a0 =                 R1_b0 * R2_a0

    b3 = a3
    b2 = a2 + R1_a2 * R2_b0
    b1 = a1 + R1_a1 * R2_b0
    b0 = a0 + R1_a0 * R2_b0

    return np.array([b3, b2, b1, b0]), np.array([a3, a2, a1, a0])

# %%
for t in [0, 0.25, 0.5, 0.75, 1.0]:
    b, a = calc_coefs(t)
    w, H = signal.freqs(b, a, 4 * np.pi * np.logspace(1, 4, 200))
    plt.semilogx(w / (2 * np.pi), 20 * np.log10(np.abs(H)))

plt.xlim(20, 20000)
plt.grid()

# %%
b, a = calc_coefs(0.5)

b, a = signal.bilinear(b, a, fs=96000)
print([b, a])

adsp.zplane(b, a)
print(np.roots(b))
print(np.roots(a))

# %%
def computeKValue(fc, fs):
    wc = 2 * np.pi * fc
    return wc / np.tanh(wc / (2 * fs))

def bilinear(b, a, K):
    N = len(b)
    M = N - 1
    bprime = np.zeros(N)
    aprime = np.zeros(N)

    for j in range(N):
        val_b = 0.0
        val_a = 0.0
        k_val = 1
        for i in range(N):
            n1_pow = 1
            for k in range(i + 1):
                comb_i_k = comb(i, k)
                k_pow = k_val * n1_pow
                for l in range(N - i):
                    if k + l == j:
                        comb_Ni_l = comb(M - i, l)
                        val_b += (comb_i_k * comb_Ni_l * b[M - i] * k_pow)
                        val_a += (comb_i_k * comb_Ni_l * a[M - i] * k_pow)
                n1_pow *= -1
            k_val *= K

        bprime[j] = np.real(val_b)
        aprime[j] = np.real(val_a)

    return signal.normalize(bprime, aprime)

# %%
FS = 48000
for t in [0, 0.25, 0.5, 0.75, 1.0]:
    b, a = calc_coefs(t)
    worN = np.logspace(1, 4.2, 200)

    w, H = signal.freqs(b, a, 2 * np.pi * worN)
    plt.semilogx(w / (2 * np.pi), 20 * np.log10(np.abs(H)))

    print(b)
    print(a)

    # b, a = signal.bilinear(b, a, fs=FS)
    b, a = bilinear(b, a, K=computeKValue(4000, FS))
    print(b)
    print(a)
    print()
    w, H = signal.freqz(b, a, worN, fs=FS)
    plt.semilogx(w, 20 * np.log10(np.abs(H)), '--')

plt.xlim(20, 20000)
plt.grid()

# %%
def calc_coefs2():
    return [C7 * (R9 + R10), 1], [C7 * R9, 1]

b, a = calc_coefs2()
w, H = signal.freqs(b, a, 4 * np.pi * np.logspace(1, 4, 200))
plt.semilogx(w / (2 * np.pi), 20 * np.log10(np.abs(H)))

# %%
