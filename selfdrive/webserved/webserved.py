#!/usr/bin/env python3
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
import cgi
import os
import urllib
from selfdrive.car.tesla.readconfig import read_config_file
import configparser
from os import listdir
from os.path import isfile, join

HOST_NAME = '0.0.0.0' 
PORT_NUMBER = 8089

videopath = '/sdcard/videos/'
datastorage= []

class configSettings():

  jsonstr = '{}'
  allowed_sections = {'OP_CONFIG','LOGGING', 'OP_PREFERENCES','JETSON_PREFERENCES'}

  def setValue(self, section, key, value):
    print("Setting config value: "+section+"\\"+key+" to: "+value)
    read_config_file(self)
    config = configparser.RawConfigParser(comment_prefixes='/',allow_no_value=True)
    config.read('/data/bb_openpilot.cfg')

    config.set(section, key, value)

    with open('/data/bb_openpilot.cfg', 'w') as configfile:
      config.write(configfile)


  def __init__(self):
    read_config_file(self)

  def transformToJson(self):
    st = '{ "OP_CONFIG": {'

#Switch to for key in ... iteration..
    st = st + ' "user_handle" : "' + self.userHandle + '",'
    st = st +  ' "tether_ip" : "' + self.tetherIP + '",'
    st = st + ' "limit_battery_min" : "' + str(self.limitBattery_Min) + '",'
    st = st + ' "limit_battery_max" : "' + str(self.limitBattery_Max) + '",'
    st = st + ' "radar_epas_type" : "' + str(self.radarEpasType) + '",'
    st = st +' "radar_position" : "' + str(self.radarPosition) + '",'
    st = st +' "radar_offset" : "' + str(self.radarOffset) + '",'
    st = st +' "radar_vin" : "' + self.radarVIN + '",'
    st = st + '"force_pedal_over_cc" : '
    st = st +  '"True",' if self.forcePedalOverCC else st + '"False",'
    st = st + ' "force_fingerprint_tesla" : '
    st = st +  '"True",' if self.forceFingerprintTesla else st + '"False",'
    st = st + ' "uses_a_pillar_harness" : '
    st = st +  '"True",' if self.usesApillarHarness else st + '"False",'
    st = st + ' "enable_hso" : '
    st = st +  '"True",' if self.enableHSO else st + '"False",'
    st = st + ' "enable_alca" : '
    st = st +  '"True",' if self.enableALCA else st + '"False",'
    st = st + ' "enable_das_emulation" : '
    st = st +  '"True",' if self.enableDasEmulation else st + '"False",'
    st = st + ' "enable_radar_emulation" : '
    st = st +  '"True",' if self.enableRadarEmulation else st + '"False",'
    st = st + ' "enable_driver_monitor" : '
    st = st +  '"True",' if self.enableDriverMonitor else st + '"False",'
    st = st + ' "has_noctua_fan" : '
    st = st +  '"True",' if self.hasNoctuaFan else st + '"False",'
    st = st + ' "limit_battery_minmax" : '
    st = st +  '"True",' if self.limitBatteryMinMax else st + '"False",'
    st = st + ' "block_upload_while_tethering" : '
    st = st +  '"True",' if self.blockUploadWhileTethering else st + '"False",'
    st = st + ' "use_tesla_gps" : '
    st = st +  '"True",' if self.useTeslaGPS else st + '"False",'
    st = st + ' "use_tesla_map_data" : '
    st = st +  '"True",' if self.useTeslaMapData else st + '"False",'
    st = st + ' "has_tesla_ic_integration" : '
    st = st +  '"True",' if self.hasTeslaIcIntegration else st + '"False",'
    st = st + ' "use_tesla_radar" : '
    st = st +  '"True",' if self.useTeslaRadar else st + '"False",'
    st = st + ' "use_without_harness" : '
    st = st +  '"True",' if self.useWithoutHarness else st + '"False",'
    st = st + ' "fix_1916" : '
    st = st +  '"True",' if self.fix1916 else st + '"False",'
    st = st + ' "do_auto_update" : '
    st = st +  '"True",' if self.doAutoUpdate else st + '"False"'
    st = st +  '},'

    st = st + '"OP_PREFERENCES": {'
    st = st + ' "spinner_text" : "' + self.spinnerText + '",'
    st = st + ' "hso_numb_period" : "' + str(self.hsoNumbPeriod) + '",'
    st = st + ' "enable_ldw" : "' + str(self.enableLdw) + '",'
    st = st + ' "ldw_numb_period" : "' + str(self.ldwNumbPeriod) + '",'
    st = st + ' "tap_blinker_extension" : "' + str(self.tapBlinkerExtension) + '",'
    st = st + ' "enable_show_car" : '
    st = st +  '"True",' if self.enableShowCar else st + '"False",'
    st = st + ' "enable_show_logo" : '
    st = st +  '"True",' if self.enableShowLogo else st + '"False",'
    st = st + ' "ahb_off_duration" : "' + str(self.ahbOffDuration) + '"'

    st = st +  '},'
    st = st + '"LOGGING": {'
    st = st + ' "should_log_can_errors" : '
    st = st +  '"True",' if self.shouldLogCanErrors else st + '"False",'
    st = st + ' "should_log_process_comm_errors" : '
    st = st +  '"True",' if self.shouldLogProcessCommErrors else st + '"False"'


    st = st +  '},'
    st = st + '"JETSON_PREFERENCES": {'
    st = st + ' "jetson_road_camera_id" : "' + str(self.roadCameraID) + '",'
    st = st + ' "jetson_driver_camera_id" : "' + str(self.driverCameraID) + '",'
    st = st + ' "jetson_road_camera_fx" : "' + str(self.roadCameraFx) + '",'
    st = st + ' "jetson_driver_camera_fx" : "' + str(self.driverCameraFx) + '",'
    st = st + ' "jetson_road_camera_flip" : "' + str(self.roadCameraFlip) + '",'
    st = st + ' "jetson_driver_camera_flip" : "' + str(self.driverCameraFlip) + '",'
    st = st + ' "jetson_monitor_forced_resolution" : "' + str(self.monitorForcedRes) + '"'


# Add video's
    st = st +  '},'
    st = st + '"VIDEOS": {'

    datastorage = [f for f in listdir(videopath) if isfile(join(videopath, f))]
    for x in range(len(datastorage)):
      if datastorage[x] != 'favicon.ico':
        st = st + '"'+ str(x)+'" : "' + datastorage[x]+'"'
        if x < len(datastorage)-2:
          st = st +','

    st = st + '}}'
    self.jsonstr = st


class webserver:
  def __init__(self):
    server_class = HTTPServer
    httpd = server_class((HOST_NAME, PORT_NUMBER), MyHandler)
    print(time.asctime(), "Server Starts - %s:%s" % (HOST_NAME, PORT_NUMBER))
    try:
         httpd.serve_forever()     
    except KeyboardInterrupt:
         pass
    httpd.server_close()
    print(time.asctime(), "Server Stops - %s:%s" % (HOST_NAME, PORT_NUMBER))

class MyHandler(BaseHTTPRequestHandler):
    #lets kill the log spamming..
    def log_message(self, format, *args):
        return

    def do_HEAD(s):
        s.send_response(200)
        s.send_header("Content-type", "text/html")
        s.end_headers()

    def do_POST(s):
      print("POST received from :")
      config = configSettings()

      if s.path == '/setConfig':
        if (s.headers['key'] is None) or (s.headers['value'] is None)  or (s.headers['section'] is None) or (s.headers['section'] == '') or (s.headers['key'] == '') or (s.headers['value'] == '') or s.headers['section'] not in  config.allowed_sections:

          s.send_response(500)
          s.send_header("Content-type", "text/html")
          s.end_headers()
          return

        config.setValue(s.headers['section'], s.headers['key'],s.headers['value'])

        s.send_response(200)
        s.send_header("Content-type", "text/html")
        s.end_headers()
        s.wfile.write(bytes('<html><head><title>config</title></head><body></body></html>','utf-8'))

      else:
        s.send_response(200)
        s.send_header("Content-type", "text/html")
        s.end_headers()
        s.wfile.write(bytes('<html><head><title>Video Deleted OK</title></head><body></body></html>','utf-8'))
        s.wfile.write(bytes("<a href='/'>Back to overview</a>", 'utf-8'))

        print(s.headers)
        contentLengthAsList = s.headers.get('Content-Length')
        contentLengthAsInt = int(contentLengthAsList)
        print(contentLengthAsInt)
        x = urllib.parse.parse_qs(s.rfile.read(contentLengthAsInt), keep_blank_values=1)
        file_to_delete = x[b'indx'][0]
        print(file_to_delete.decode("utf-8"))
        os.remove(videopath + "/"+ file_to_delete.decode("utf-8"))

    def do_GET(s):

        if (s.path == '/getConfig'):
          config = configSettings()
          config.transformToJson()
          s.send_response(200)
          s.send_header("Content-type", "application/json")
          s.end_headers()
          s.wfile.write(bytes(config.jsonstr, 'utf-8'))
          return;

        else:
          if (s.path != '/'):
              video = s.path.strip('\/\'')
              #print("NON root request - "+'../' + video)
              s.send_response(200)
              s.send_header('Content-type', 'application/octet-stream')
              with open('/sdcard/videos/' + video, 'rb') as file: 
                  fsize = str(os.fstat(file.fileno())[6])
                  #print(fsize)
                  s.send_header('Content-Length',fsize)
                  s.end_headers()
                  s.wfile.write(file.read())
              return

        datastorage = [f for f in listdir(videopath) if isfile(join(videopath, f))]
        """Respond to a GET request."""
        s.send_response(200)
        s.send_header("Content-type", "text/html")
        s.end_headers()
        s.wfile.write(b'<html><head><title>OpenPilot Dashcam Videos</title></head>')
        s.wfile.write(b'<body><p>Stored Videos</p>')
        for x in range(len(datastorage)):
          if datastorage[x] != 'favicon.ico':
              st = '<form action="/" method="POST"><a href="'+datastorage[x]+'">'+datastorage[x]+'</a><input type="hidden" name="indx" value="'+datastorage[x]+'"><input type="hidden" name="_method" value="DELETE"/><input type="submit" value="Remove"/></form></br>'
              s.wfile.write(bytes(st, 'utf-8'))

        s.wfile.write(b'</body></html>')

if __name__ == '__main__':
  print("starting webserver");
  webserver()

def main(gctx=None):
  print("starting webserver");
  webserver()
