# %%
import enum
import numpy as np
import matplotlib.pyplot as plt

# %%
N = 1024
x = np.linspace(0, 1, N)
y = np.zeros((9, N))

def ramp_up(x, off):
    o16 = 1 / 16
    y = 16 * (x - off * o16)
    return np.where (y > 1,  np.zeros_like(x), np.where(y < 0, np.zeros_like(x), y))

def ramp_down(x, off):
    o16 = 1 / 16
    y = 1 - 16 * (x - off * o16)
    return np.where (y > 1,  np.zeros_like(x), np.where(y < 0, np.zeros_like(x), y))

y[0] = ramp_up(x, 0) + ramp_down(x, 1)
y[1] = ramp_up(x, 15) + ramp_down(x, 0) + ramp_up(x, 1) + ramp_down(x, 2)
y[2] = ramp_up(x, 2) + ramp_down(x, 3) + ramp_up(x, 14) + ramp_down(x, 15)
y[3] = ramp_up(x, 3) + ramp_down(x, 4) + ramp_up(x, 13) + ramp_down(x, 14)
y[4] = ramp_up(x, 4) + ramp_down(x, 5) + ramp_up(x, 12) + ramp_down(x, 13)
y[5] = ramp_up(x, 5) + ramp_down(x, 6) + ramp_up(x, 11) + ramp_down(x, 12)
y[6] = ramp_up(x, 6) + ramp_down(x, 7) + ramp_up(x, 10) + ramp_down(x, 11)
y[7] = ramp_up(x, 7) + ramp_down(x, 8) + ramp_up(x, 9) + ramp_down(x, 10)
y[8] = ramp_up(x, 8) + ramp_down(x, 9)

# %%
for n, yy in enumerate(y):
    plt.plot(x, yy + n)
plt.xlim(0, 1)

# %%
