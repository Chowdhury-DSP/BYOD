# %%
import numpy as np
import matplotlib.pyplot as plt

# %%
FS = 48000
fc = 100
N = 20000

x = 0.125 * np.sin(2 * np.pi * np.arange(N) * fc / FS)
plt.plot(x)

# %%
def process(x, gain_01):
    gr4 = 1 / 100.0e3
    gain_factor = (gain_01 * 0.98 + 0.01)**5
    grfp = 1 / ((1 - gain_factor) * 1.0e3)
    grfm = 1 / (gain_factor * 1.0e3)
    gc1 = 2 * FS * 2.2e-6
    gc2 = 2 * FS * 20.0e-6

    vt = 0.026 * 0.5
    Iq = 0.5e-10

    y = np.zeros_like(x)

    ic1eq = 0
    ic2eq = 0

    vo = 0
    io = 0
    for n in range(len(x)):
        vi = x[n]

        # initial guess
        v1 = -((-gr4*(grfp*ic2eq+(gc2+grfm+grfp)*io) + (gr4*(gc2+grfm)+(gc2+gr4+grfm)*grfp) * (ic1eq+io-gc1*vi)) / (gc1*gr4*(gc2+grfm) + gr4*(gc2+grfm)*grfp + gc1*(gc2+gr4+grfm)*grfp))

        delta = 100
        n_iters = 0
        while np.abs(delta) > 1.0e-5 or n_iters > 5:
            io = Iq * np.exp(-v1 / vt)
            vo = (ic1eq + io + (gc1+gr4)*v1 - gc1*vi) / gr4
            # vo = ((gc1+gr4) * grfp*ic2eq+(gc2+grfm+grfp) * (-gr4*ic1eq+gc1*io+gc1*gr4*vi)) / (gc1*gr4*(gc2+grfm) + gr4*(gc2+grfm)*grfp + gc1*(gc2+gr4+grfm)*grfp)

            F = (gc1*(vi - v1) - ic1eq) + gr4*(vo - v1) - io
            F_p = -gc1 - gr4 + (Iq/vt) * np.exp(-v1 / vt)
            delta = F / F_p
            v1 -= delta

            n_iters += 1
        
        io = Iq * np.exp(-v1 / vt)
        vo = (ic1eq + io + (gc1+gr4)*v1 - gc1*vi) / gr4
        # vo = ((gc1+gr4) * grfp*ic2eq+(gc2+grfm+grfp) * (-gr4*ic1eq+gc1*io+gc1*gr4*vi)) / (gc1*gr4*(gc2+grfm) + gr4*(gc2+grfm)*grfp + gc1*(gc2+gr4+grfm)*grfp)

        v2 = -((io + gr4*v1 - (gr4+grfp)*vo) / grfp)
        ic1eq  = 2 * gc1 * (vi - v1) - ic1eq
        ic2eq  = 2 * gc2 * (v2) - ic2eq

        y[n] = 4 * vo

    return y - np.mean(y)

# %%

plt.plot(x[N - 2000:])

for g in [0, 0.25, 0.5, 0.75, 1.0]:
    y = process(x, g)
    plt.plot(y[N - 2000:], '--')

# %%
