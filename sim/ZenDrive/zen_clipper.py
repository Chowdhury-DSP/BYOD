import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

# 1N4148
# 2N7002

spice_data = np.genfromtxt('ZenClipper.txt')

time = np.array(spice_data[1:,0])
x = np.array(spice_data[1:,1])
y = np.array(spice_data[1:,2])

# plt.plot(time, x)
# plt.plot(time, y * -1e2)
# plt.title('SPICE Output')
# plt.xlabel('Time [s]')
# plt.ylabel('Voltage')

# plt.show()
# exit()

def clipper_func(x, gain_in=1, gain_out=1):
    x_in = np.copy(x)
    x_in *= gain_in
    y = -10**gain_out * np.sinh(x_in)
    return y

popt, pcov = curve_fit(clipper_func, x, y, p0=[12.5, -6], bounds=([1, -12], [20, 0]))
print(f'Is = {10**popt[1]}')
print(f'Vt = {1.0 / popt[0]}')
# popt = [1.10362719 -9.28055002]
y_test = clipper_func(x, *popt)
# y_test = clipper_func(x, 1, -7.7)

# plt.plot(time, x)
plt.plot(time, y * 1e2)
plt.plot(time, y_test * 1e2, '--')

plt.title('Test Output')
plt.xlabel('Time [s]')
plt.ylabel('Voltage')
plt.xlim(0, 0.2)
# plt.xlim(0, 0.05)
# plt.ylim(-1.85, 0.02)

plt.show()
