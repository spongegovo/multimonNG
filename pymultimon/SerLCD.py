#!/usr/bin/env python
#
# Libary for SerLCD v2.5 Controller
# by Robert Postill <rpostill@gmail.com>
#
# Currently support only 16x2 LCD
#

import serial
import time

class SerLCD:
  # Display Commands to enter command mode
  COMMAND1 = 254
  COMMAND2 = 124

  # LCD Type setup (not fully supported)
  CHARWIDTH20 = 3
  CHARWIDTH16 = 5
  LINES4 = 5
  LINES2 = 6

  # Extended LCD Commands
  CLEARDISPLAY = 01
  MOVERIGHT = 20
  MOVELEFT = 16
  SCROLLRIGHT = 28
  SCROLLLEFT = 24
  DISPLAYON = 12
  DISPLAYOFF = 8
  BLINKON = 13  
  BLINKOFF = 12
  POSITION = 128

  # Other options
  SPLASHTOGGLE = 9
  SETSPLASH = 10

  def __init__(self, port="/dev/ttyAMA0", baud="9600"):
    self.lcd = serial.Serial(port=port,baudrate=baud)

  def __del__(self):
    self.lcd.close()

  def clear(self):
    if self.lcd.isOpen():
      self.lcd.write(chr(SerLCD.COMMAND1))
      self.lcd.write(chr(SerLCD.CLEARDISPLAY))

  def pos(self,x,y):
    self.position = 0
    if x == 1:
      self.position = y - 1
    elif x == 2:
      self.position = y + 63
    self.position = self.position + 128
    if self.lcd.isOpen():
      self.lcd.write(chr(SerLCD.COMMAND1))
      self.lcd.write(chr(self.position))

  def write(self,text):
    if self.lcd.isOpen():
      self.lcd.write(text)

  def display(self,x):
    if self.lcd.isOpen():
      if x == 0:
        self.lcd.write(chr(SerLCD.COMMAND1))
        self.lcd.write(chr(SerLCD.DISPLAYOFF))
      else:
        self.lcd.write(chr(SerLCD.COMMAND1))
        self.lcd.write(chr(SerLCD.DISPLAYON))

  def scroll_right(self):
    if self.lcd.isOpen():
      self.lcd.write(chr(SerLCD.COMMAND1))
      self.lcd.write(chr(SerLCD.SCROLLRIGHT))

  def scroll_left(self):
    if self.lcd.isOpen():
      self.lcd.write(chr(SerLCD.COMMAND1))
      self.lcd.write(chr(SerLCD.SCROLLLEFT))

  def move_right(self):
    if self.lcd.isOpen():
      self.lcd.write(chr(SerLCD.COMMAND1))
      self.lcd.write(chr(SerLCD.MOVERIGHT))

  def move_left(self):
    if self.lcd.isOpen():
      self.lcd.write(chr(SerLCD.COMMAND1))
      self.lcd.write(chr(SerLCD.MOVELEFT))

  def blink(self, x):
    if self.lcd.isOpen():
      if x == 1:
        self.lcd.write(chr(SerLCD.COMMAND1))
        self.lcd.write(chr(SerLCD.BLINKON))
      else:
        self.lcd.write(chr(SerLCD.COMMAND1))
        self.lcd.write(chr(SerLCD.BLINKOFF))

  def splash_set(self, line1, line2):
    if self.lcd.isOpen():
      self.pos(1, 1)
      self.write("                ")   # We want to write blanks to all
      self.pos(2, 1)                   # lines to prevent bug in firmware
      self.write("                ")   # where old text can appear
      self.clear()
      self.screen(line1, line2)
      time.sleep(1)
      self.lcd.write(chr(SerLCD.COMMAND2))
      self.lcd.write(chr(SerLCD.SETSPLASH))
      time.sleep(1)
      self.clear()

  def splash_toggle(self):
    if self.lcd.isOpen():
      self.lcd.write(chr(SerLCD.COMMAND2))
      self.lcd.write(chr(SerLCD.SPLASHTOGGLE))

  def brightness(self, x):
    self.brightness = x + 127
    if self.lcd.isOpen():
      self.write(chr(SerLCD.COMMAND1))
      self.write(chr(self.brightness))

  def screen(self, line1, line2):
    self.clear()
    self.write(line1)
    self.pos(2, 1)
    self.write(line2)

if __name__ == "__main__":
  display = SerLCD()

  display.clear()
  display.write(" Python Serial ")
  display.pos(2,1)
  display.write("LCD Library v0.1")
