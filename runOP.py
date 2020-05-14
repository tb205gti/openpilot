#!/usr/bin/env python3
import numpy as np
import time
import atexit
import cereal.messaging as messaging
import argparse
import threading
from common.params import Params
from common.realtime import Ratekeeper

parser = argparse.ArgumentParser(description='Bridge between CARLA and openpilot.')
parser.add_argument('--autopilot', action='store_true')
parser.add_argument('--joystick', action='store_true')
parser.add_argument('--realmonitoring', action='store_true')
args = parser.parse_args()

pm = messaging.PubMaster(['frame', 'sensorEvents', 'can'])

def health_function():
  pm = messaging.PubMaster(['health'])
  rk = Ratekeeper(1.0)
  while 1:
    dat = messaging.new_message('health')
    dat.valid = True
    dat.health = {
      'ignitionLine': True,
      'hwType': "whitePanda",
      'controlsAllowed': True
    }
    pm.send('health', dat)
    rk.keep_time()

def fake_driver_monitoring():
  if args.realmonitoring:
    return
  pm = messaging.PubMaster(['driverState'])
  while 1:
    dat = messaging.new_message('driverState')
    dat.driverState.faceProb = 1.0
    pm.send('driverState', dat)
    time.sleep(0.1)

def go():
  threading.Thread(target=health_function).start()
  threading.Thread(target=fake_driver_monitoring).start()

  def destroy():
    print("clean exit")
    imu.destroy()
    camera.destroy()
    vehicle.destroy()
    print("done")
  atexit.register(destroy)

  # can loop
  sendcan = messaging.sub_sock('sendcan')
  rk = Ratekeeper(100, print_delay_threshold=0.05)

  is_openpilot_engaged = False
  in_reverse = False

  throttle_out = 0
  brake_out = 0
  steer_angle_out = 0

  while 1:
    cruise_button = 0

    rk.keep_time()

if __name__ == "__main__":
  params = Params()
  params.delete("Offroad_ConnectivityNeeded")
  from selfdrive.version import terms_version, training_version
  params.put("HasAcceptedTerms", terms_version)
  params.put("CompletedTrainingVersion", training_version)
  params.put("CommunityFeaturesToggle", "1")

  health_function()
#  go()

