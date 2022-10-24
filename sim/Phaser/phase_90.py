# %%
import numpy as np
import matplotlib.pyplot as plt
import scipy.signal as signal

# %%
R1 = 10.0e3
C1 = 47.0e-9
R5 = 10.0e3
R6 = 24.0e3

FS = 48000

# %%
def get_stage_coefs(x=0.05):
    return np.array([C1 * (x*R6), -1]), np.array([C1 * (x*R6), 1])

worN = 2 * np.logspace(1, 4)
b, a = get_stage_coefs()
w, H = signal.freqs(b, a, worN=worN*(2*np.pi))
plt.semilogx(w/(2*np.pi), np.unwrap(np.angle(H)))

# %%
