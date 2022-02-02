# %%
import numpy as np
import matplotlib.pyplot as plt
import scipy.signal as signal

# %%
FS = 48000

C1 = 100e-9 # 5e-9
C2 = 10e-9
C3 = 47e-6
R1 = 470e3
R2 = 68e3
R3 = 3.9e3
RV = 10e3

V_plus = 9.0
R_bias = R1 / (R1 + R2)

Z_T = 12.5e3

Vt = 0.02585
I_s = 10.0e-15
Beta_F = 200
Beta_R = 2

# %%
R1pZt = Z_T * R1 / (Z_T + R1)

b_s = [C1 * R1pZt * R2, 0]
a_s = [C1 * R1pZt * R2, R1pZt + R2]

worN = 2 * np.logspace(1, 4)
w, H = signal.freqs(b_s, a_s, worN = 2 * np.pi * worN)
plt.semilogx(w / (2 * np.pi), 20 * np.log10(np.abs(H)))

b1, a1 = signal.bilinear(b_s, a_s, fs=FS)
w, H = signal.freqz(b1, a1, fs=FS, worN = worN)
plt.semilogx(w, 20 * np.log10(np.abs(H)), '--')

plt.grid()

# %%
T = 1 / FS
b_s = [C3 * R3, 1]
a_s = [0, R3]

worN = 2 * np.logspace(1, 4)
w, H = signal.freqs(b_s, a_s, worN = 2 * np.pi * worN)
plt.semilogx(w / (2 * np.pi), 20 * np.log10(np.abs(H)))

b3, a3 = signal.bilinear(b_s, a_s, fs=FS)
w, H = signal.freqz(b3, a3, fs=FS, worN = worN)
plt.semilogx(w, 20 * np.log10(np.abs(H)), '--')

b3 = [C3 / T + 1 / R3, - C3 / T]
a3 = [1]
w, H = signal.freqz(b3, a3, fs=FS, worN = worN)
plt.semilogx(w, 20 * np.log10(np.abs(H)), '--')

# %%
class Rangemaster:
    def __init__(self):
        self.v_e = 8.4

    def process_block(self, block):
        v_in = np.copy(block)
        v_b = signal.lfilter(b1, a1, v_in) + R_bias * V_plus
        i_b = v_b / Z_T

        for n in range(len(v_b)):
            i_e = signal.lfilter(b3, a3, [V_plus - self.v_e])[0]
            # i_e = (V_plus - self.v_e) / R3
            v_be = v_b[n] - self.v_e
            exp_v_be = np.exp(v_be / Vt)
            v_bc = Vt * np.log(((i_b[n] / I_s) - (1.0 / Beta_F) * (exp_v_be - 1)) * Beta_R + 1)

            for _ in range(5):
                exp_v_bc = np.exp(v_bc / Vt)
                i_c = I_s * ((exp_v_be - exp_v_bc) - (1.0 / Beta_R) * (exp_v_bc - 1))
                F_y = i_b[n] + i_e - i_c
                dF_y = -I_s * ((-1/Vt) * exp_v_bc - (1 / Vt / Beta_R) * exp_v_bc)

                v_bc -= F_y / (dF_y + 1.0e-24)

            self.v_e = v_b[n] - Vt * np.log((i_c / I_s) + (1 / Beta_R) * (np.exp(v_bc / Vt) - 1) + np.exp(v_bc / Vt))
            v_b[n] = i_c * RV

        return v_b

        # v_be = v_b - 8.2
        # yy = (i_b / I_s) - (1 / Beta_F) * (np.exp(v_be / Vt) - 1)
        # v_bc = np.log(yy * Beta_R + 1) * Vt
        # return v_b - v_bc

        # return 1 * (np.tanh(20 * (v_b - 8)) + 0.5)

# %%
N = 1600
freq = 100
x = 0.2 * np.sin(2 * np.pi * freq * np.arange(N) / FS)

rm = Rangemaster()
v_b = rm.process_block(x)

# plt.plot(x)
plt.plot(v_b * 1e16 - 5e5 - 1.0, '--')

# %%

# %%
