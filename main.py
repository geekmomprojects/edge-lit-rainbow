import board
import neopixel
from time import sleep
import adafruit_fancyled.adafruit_fancyled as fancy
from adafruit_circuitplayground.express import cpx

PIX_PER_STRIP = 7

#If true, color stripes alternate in interactive mode
COLORS_ALTERNATING = True

#Set brightnessto 1, we'll use gamma adjustment for brightness control
strip1 = neopixel.NeoPixel(board.D2, PIX_PER_STRIP, brightness=1, auto_write=False)
strip2 = neopixel.NeoPixel(board.D3, PIX_PER_STRIP, brightness=1, auto_write=False)

# BaseHue is an evolvoing hue value, allowing colors to drift
# across the sprectrum with time. FancyLED accepts any real number
# to specify hue, but only the decimal portion matters
baseHue = .86
hueInc = -0.003
hueSpeed = 3
maxHueSpeed = 8

# Specfies the difference in hue between adjacent rainbow arcs.
# the value is < 0 because longest wavelengths are on the outside
arcHueInc = -0.14

# Set the rate at which the baseHue evolves
def incrementHueSpeed():
  global hueSpeed
  hueSpeed = hueSpeed + 2
  if hueSpeed > maxHueSpeed:
    hueSpeed = 1
    
# Increments the baseHue
def incrementHue():
  global baseHue
  baseHue += hueInc*hueSpeed

#Turn all LEDs off
def ledsOff():
  strip1.fill((0,0,0))
  strip2.fill((0,0,0))
  strip1.show()
  strip2.show()

#These brightness levels seem to help balance color by toning down blue and upping red,
#but there is no real quantitative rationale behind them
levels=(0.75,0.9,0.45)
def setRainbow(strip, startHue, inc):
    for j in range(PIX_PER_STRIP):
      col = fancy.gamma_adjust(fancy.CHSV(startHue),brightness=levels)
      strip[j] = col.pack()
      startHue += inc

def clamp(n, min, max):
  if n < min:
    return min
  elif n > max:
    return max
  else:
    return n

#Clamps acceleration value to g (hopefully ignore jitter) and scales it to [0,1.0]
def accelToRange(accel):
  accel = clamp(accel, -9.8, 9.8)
  return 0.5 + accel/(2*9.8)

# Multiply a tuple by a scalar
def multiplyTuple(factor, tup):
  return tuple([factor*x for x in tup])

# Track the state of the buttons (pressed=True, unpressed=False) for debouncing
buttonStateA = False
buttonStateB = False

# contrast is the difference in hue between the two sides of each 
# rainbow arc. If contrast is an integer, the hues are identical and
# if contrast is an odd multiple of 0.5, hues are color wheel opposites.
contrast = 0.0

while True:
  # Check for button presses
  
  # Button A increments the rate at which the hue changes.
  if not cpx.button_a and buttonStateA:
    incrementHueSpeed()
    buttonStateA = False
    #Flashing LEDs off makes it easier to see when we've pushed the button
    #because changing hue rotation speed isn't super obvious
    ledsOff()
  elif cpx.button_a and not buttonStateA:
    buttonStateA = True

  # Button B toggles between alternating and identical color modes
  if not cpx.button_b and buttonStateB:
    COLORS_ALTERNATING = not COLORS_ALTERNATING
    contrast += 0.5
    buttonStateB = False
  elif cpx.button_b and not buttonStateB:
    buttonStateB = True

  if cpx.switch:  # Spectrum mode
    setRainbow(strip1, baseHue, arcHueInc)
    setRainbow(strip2, baseHue+contrast, arcHueInc)

  else: # Color mixing mode
    # Change the relative brightness with side-to-side tilting of the CPX
    x, y, z = cpx.acceleration
    bright = accelToRange(x)

    # Set the relative brightness of the base color and its contrasting color
    col1 = fancy.gamma_adjust(fancy.CHSV(baseHue),brightness=multiplyTuple(bright,levels)).pack()
    col2 = fancy.gamma_adjust(fancy.CHSV(baseHue+0.5),brightness=multiplyTuple((1.0-bright),levels)).pack()
    if COLORS_ALTERNATING:  # Each rainbow side alternates colors
      colors = [col1,col2]
      index = 0
      for i in range(PIX_PER_STRIP):
        strip1[i] = colors[index]
        index = (index + 1) % 2
        strip2[i] = colors[index]
    else:  # Each rainbow side contains uniform color
      strip1.fill(col1)
      strip2.fill(col2)

  strip1.show()
  strip2.show()
  incrementHue()