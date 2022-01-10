# %%
import numpy as np
from numpy.polynomial import Polynomial
import scipy.signal as signal
import matplotlib.pyplot as plt

from sage.all import var, solve

# %%
s = var('s')
R8 = var('R8')
R5 = var('R5')
RTp = var('RTp')
RTm = var('RTm')
C8 = var('C8')
C9 = var('C9')
Vi = var('Vi')
Vo = var('Vo')

Va = var('Va')
Vb = var('Vb')

# %%
Za = var('Za')
Zb = var('Zb')
Z5 = var('Z5')
Z7 = var('Z7')
Z8 = var('Z8')
Z9 = var('Z9')
io_Vi = var('io_Vi')

# %%
H = (Z7 / (Z7 + Z8)) + (Za + (Z7 * Z8) / (Z7 + Z8)) * io_Vi
print(H)

# %%
Ni = Z5 / (Z5 + Z9) - Z7 / (Z7 + Z8)
Di = Za + Zb + (Z7 * Z8) / (Z7 + Z8) + (Z5 * Z9) / (Z5 + Z9)

# %%
H = H.substitute({io_Vi: Ni / Di})
H = H.simplify_rational()
print(H)

# %%
H = H.substitute({Z8: R8, Z5: R5})
H = H.substitute({Z7: 1/(C8 * s), Z9: 1/(C9 * s)})
H.simplify_full()
print(H)

# %%
print(H.numerator().collect(s))
print(H.denominator().collect(s))

# %%
def calc_coefs(tone):
    C8 = 10e-9
    C9 = 4e-9
    R8 = 39e3
    R5 = 22e3
    RT = 100e3
    Za = tone * RT
    Zb = (1 - tone) * RT

    phi = C8 * C9 * R5 * R8
    b2 = phi * Za
    b1 = C9 * R5 * (R8 + RT)
    b0 = R5 + Zb
    a2 = phi * RT
    a1 = C8 * R8 * (R5 + RT) + b1
    a0 = R5 + R8 + RT
    return [b2, b1, b0], [a2, a1, a0]

worN = 4 * np.pi * np.logspace(1, 4)
for t in [0.0, 0.5, 0.75, 1.0]:
    b, a = calc_coefs(t)
    w, H = signal.freqs(b, a, worN=worN)
    plt.semilogx(w / (2 * np.pi), 20*np.log10(np.abs(H)), label=f'{int(t*100)}')
plt.grid()
plt.legend()

# %%
