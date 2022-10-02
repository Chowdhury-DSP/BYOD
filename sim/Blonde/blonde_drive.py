# %%
import enum
import numpy as np
import matplotlib.pyplot as plt

# %%
# Zener constants
alpha = 1.0168177
log_alpha = np.log(alpha)
beta = 9.03240196
beta_exp = beta * log_alpha
c = 0.222161
bias = 8.2

max_val = 7.5
mult = 10.0
one = 0.99
oneOverMult = one / mult
betaExpOverMult = beta_exp / mult

def zener_clip(x):
    x = mult * x

    x_abs = np.abs(x)
    x_less_than = x_abs < max_val

    y = -np.exp(beta_exp * -np.abs(x + c)) + bias
    y = np.sign(x) * y * oneOverMult

    if x_less_than:
        return x * oneOverMult
    else:
        return y

def zener_clip_prime(x):
    x = mult * x

    x_abs = np.abs(x)
    x_less_than = x_abs < max_val

    y = np.exp(beta_exp * -np.abs(x + c)) + betaExpOverMult

    if x_less_than:
        return one
    else:
        return y

def zener_clip_combined(x):
    x = mult * x

    x_abs = np.abs(x)
    x_less_than = x_abs < max_val

    exp_out = np.exp(beta_exp * -np.abs(x + c))

    if x_less_than:
        return x * oneOverMult, one
    else:
        return np.sign(x) * (-exp_out + bias) * oneOverMult, exp_out + betaExpOverMult
    

# %%
x = np.linspace(-2, 2)
y = np.zeros_like(x)
y_p = np.zeros_like(x)
for i, xx in enumerate(x):
    y[i] = zener_clip(xx)
    y_p[i] = zener_clip_prime(xx)

plt.plot(x, y)
plt.plot(x, y_p)

for i, xx in enumerate(x):
    y[i], y_p[i] = zener_clip_combined(xx)

plt.plot(x, y, 'r--')
plt.plot(x, y_p, '--')

plt.grid()

# %%
def blonde_drive(x, A):
    N = len(x)
    y0 = 0
    
    y = np.zeros(N)
    for n in range(N):
        for _ in range(4):
            zener_f, zener_fp = zener_clip_combined(x[n] + A * y0)
            F = zener_f + y0
            Fp = zener_fp * A + 1
            y0 -= F / Fp

        y[n] = -y0
    return y

FS = 48000
freq = 100
x = np.sin(2 * np.pi * freq / FS *np.arange(1024)) + 0.1
plt.plot(x, label='x')

for A in [0, 0.1, 0.25, 1]:
    plt.plot(blonde_drive(x, A), label=f'A = {A}')

# plt.legend()
plt.grid()

# %%
