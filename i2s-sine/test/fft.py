from scipy.fft import rfft, rfftfreq
from matplotlib import pyplot
import numpy as np

SAMPLE_RATE = 16000
DURATION  = 1

t = np.linspace(0, DURATION, SAMPLE_RATE * DURATION, endpoint=False)
v = np.genfromtxt("sine.dat", delimiter=" ")

# format of 'sine.dat':
# 'sample float cordic16 c16_delta float cordic32 c32_delta'

trig = v[:,1]
c16 = v[:,2]
c32 = v[:,5]

N = SAMPLE_RATE * DURATION

c16_f = rfft(c16)
c32_f = rfft(c32)
trig_f = rfft(trig)
xf = rfftfreq(N, 1 / SAMPLE_RATE)

p1 = pyplot.figure(1)
pyplot.plot(xf, np.abs(trig_f))

p2 = pyplot.figure(2)
pyplot.plot(xf, np.abs(c16_f))

p3 = pyplot.figure(3)
pyplot.plot(xf, np.abs(c32_f))

pyplot.show()