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

p1 = pyplot.figure(1)
pyplot.plot(t[0:500], trig[0:500])

p2 = pyplot.figure(2)
pyplot.plot(t[0:500], c16[0:500])

p3 = pyplot.figure(3)
pyplot.plot(t[0:500], c32[0:500])

pyplot.show()