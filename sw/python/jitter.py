import numpy as np
import matplotlib.pyplot as plt

# MODEL 1: Normally distributed flight times 
# Desired mean and size of the series
mean_flight_time = 50
size = 10000
# Desired variance
desired_variance = 12.0
# Calculate the standard deviation
standard_deviation = np.sqrt(desired_variance)
# Generate the random series that represents the flight times
# of each packet. Jitter introduces some randomness in this process.
# TODO: NEED TO PICK THE RIGHT STATISTICAL MODEL
flight_times = np.random.normal(loc=mean_flight_time, scale=standard_deviation, size=size)

# Verify the variance of the generated series (will be close to the desired variance)
#calculated_variance = np.var(random_series)

#print(f"Desired variance: {desired_variance}")
#print(f"Calculated standard deviation: {standard_deviation}")
#print(f"Calculated variance of the generated series: {calculated_variance}")

# This represents now long we wait before playing the packet. This number
# will adjust dynamically.
delay = 60

_di = delay
_di_1 = delay
_vi = 0
_vi_1 = 0
# An important paper on this topic: https://ee.lbl.gov/papers/congavoid.pdf
# RFC793 section 3.7
_alpha = 0.998002
_beta = 4

t_array = []
ni_array = []
pi_array = []

missed_packets = 0

for t in range(0, size):
    t_array.append(t)
    ni = flight_times[t]
    _di = _alpha * _di_1 + (1 - _alpha) * ni
    _di_1 = _di
    _vi = _alpha * _vi_1 + (1 - _alpha) * abs(_di - ni)
    _vi_1 = _vi
    # This is the playout point for the packet
    _pi = _di + _beta * _vi
    ni_array.append(ni)
    pi_array.append(_pi)
    if ni > _pi:
        missed_packets += 1

print("Missed packets", missed_packets)
print("Missed packet%", 100 * (missed_packets / size))

plt.plot(t_array, ni_array, label='Arrival Time')

# Plot the second series
plt.plot(t_array, pi_array, label='Playout Time (miss%=' + "{:.2f}".format(100 * (missed_packets / size)) + ")")

# Add labels and title for clarity
plt.xlabel('Sample')
plt.ylabel('ms')

# Add a legend to differentiate the series
plt.legend()

# Display the plot
plt.show()
