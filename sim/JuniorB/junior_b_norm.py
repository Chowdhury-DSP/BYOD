# %%
import numpy as np
import matplotlib.pyplot as plt

# %%
os_rates = [1, 2, 4, 8, 16]
sample_rates = [48000 * x for x in os_rates]
sample_rate_factors = [x / 96000 for x in sample_rates]
print(sample_rate_factors)

# %%
Rs = [1150, 1000, 820, 650, 530]
plt.plot(sample_rate_factors, Rs)

Rs_adjust = np.zeros(len(Rs))
for i in range(len(Rs_adjust)):
    if (sample_rate_factors[i] <= 1.0):
        Rs_adjust[i] = 1000 / sample_rate_factors[i]**0.2
    else:
        Rs_adjust[i] = 1000 / sample_rate_factors[i]**0.28

plt.plot(sample_rate_factors, Rs_adjust)

# %%
