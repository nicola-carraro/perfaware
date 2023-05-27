# ========================================================================
# LISTING 21
# ========================================================================

import json
import time
from math import asin, cos, radians, sin, sqrt

JSONFile = open('data\input.json')

#
# Read the input
#

StartTime = time.time()
JSONInput = json.load(JSONFile)
MidTime = time.time()

#
# Average the haversines
#

def HaversineOfDegrees(X0, Y0, X1, Y1, R):

  dY = radians(Y1 - Y0)
  dX = radians(X1 - X0)
  Y0 = radians(Y0)
  Y1 = radians(Y1)

  RootTerm = (sin(dY/2)**2) + cos(Y0)*cos(Y1)*(sin(dX/2)**2)
  Result = 2*R*asin(sqrt(RootTerm))

  return Result

EarthRadiuskm = 6371
Sum = 0
Count = 0
for Pair in JSONInput['pairs']: 
    Distance = HaversineOfDegrees(Pair['x1'], Pair['y1'], Pair['x2'], Pair['y2'], EarthRadiuskm)
    Sum += Distance
    print("Distance : ", Distance)
    Count += 1
Average = Sum / Count
EndTime = time.time()

#
# Display the result
#
TestDistance = HaversineOfDegrees(86.9250, 27.9881, -73.9857, 40.7484, EarthRadiuskm)
print("TestDistance: " + str(TestDistance))
print("Count: " + str(Count))
print("Sum: " + str(Sum))
print("Result: " + str(Average))
print("Input = " + str(MidTime - StartTime) + " seconds")
print("Math = " + str(EndTime - MidTime) + " seconds")
print("Total = " + str(EndTime - StartTime) + " seconds")
print("Throughput = " + str(Count/(EndTime - StartTime)) + " haversines/second")