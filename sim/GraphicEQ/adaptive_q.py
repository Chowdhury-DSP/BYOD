import numpy as np
import audio_dspy as adsp
import matplotlib.pyplot as plt

# estimating the adaptive Q characteristic from the Aengus EQ

FS = 48000.0
FC = 1000
worN = 2 * np.logspace(1, 4, num=500)

gains = [12, 9, 6, 4, 2, 0, -2, -4, -6, -9, -12]
q_vals = [2.0, 1.414, 1.0, 0.7071, 0.5, 0.35, 0.5, 0.7071, 1.0, 1.414, 2.0]

plt.figure()
for gain, q in zip(gains, q_vals):
    b, a = adsp.design_bell(FC, q, 10 ** (gain / 20.0), FS)
    adsp.plot_magnitude_response(b, a, worN=worN, fs=FS)

plt.title('Frequency responses at different gains')
plt.grid()

plt.figure()
plt.plot(gains, q_vals, label='actual')

fit = np.polyfit(gains, q_vals, 8)
print(fit)
plt.plot(gains, np.poly1d(fit)(gains), label='fit')

plt.title('Gain vs. Q characteristic')
plt.xlabel('Gain [dB]')
plt.ylabel('Q value')

plt.show()
