from common.kalman.simple_kalman import KF1D
from selfdrive.can.parser import CANParser
from selfdrive.config import Conversions as CV
from selfdrive.car.tesla.ACC_module import ACCMode
from selfdrive.car.tesla.PCC_module import PCCModes
from selfdrive.car.tesla.values import CAR, DBC
from selfdrive.car.modules.UIBT_module import UIButtons
from selfdrive.car.modules.UIEV_module import UIEvents
from selfdrive.car.tesla.readconfig import read_config_file
import os
import subprocess
from common.params import read_db, write_db
 
def parse_gear_shifter(can_gear_shifter, car_fingerprint):

  # TODO: Use VAL from DBC to parse this field
  if car_fingerprint == CAR.MODELS:
    # VAL_ 280 DI_gear 4 "D" 0 "INVALID" 3 "N" 1 "P" 2 "R" 7 "SNA" ;
    if can_gear_shifter == 0x1:
      return "park"
    elif can_gear_shifter == 0x2:
      return "reverse"
    elif can_gear_shifter == 0x3:
      return "neutral"
    elif can_gear_shifter == 0x4:
      return "drive"

  return "unknown"



def calc_cruise_offset(offset, speed):
  # euristic formula so that speed is controlled to ~ 0.3m/s below pid_speed
  # constraints to solve for _K0, _K1, _K2 are:
  # - speed = 0m/s, out = -0.3
  # - speed = 34m/s, offset = 20, out = -0.25
  # - speed = 34m/s, offset = -2.5, out = -1.8
  _K0 = -0.3
  _K1 = -0.01879
  _K2 = 0.01013
  return min(_K0 + _K1 * speed + _K2 * speed * offset, 0.)


def get_can_signals(CP):
# this function generates lists for signal, messages and initial values
  signals = [
      ("MCU_gpsVehicleHeading", "MCU_gpsVehicleSpeed", 0),
      ("MCU_gpsVehicleSpeed", "MCU_gpsVehicleSpeed", 0),
      ("MCU_userSpeedOffset", "MCU_gpsVehicleSpeed", 0),
      ("MCU_userSpeedOffsetUnits", "MCU_gpsVehicleSpeed", 0),
      ("MCU_mppSpeedLimit", "MCU_gpsVehicleSpeed", 0),
      ("MCU_mapSpeedLimitUnits", "MCU_gpsVehicleSpeed", 0),
      ("MCU_gpsAccuracy", "MCU_locationStatus", 0),
      ("MCU_latitude", "MCU_locationStatus", 0),
      ("MCU_longitude", "MCU_locationStatus", 0),
      ("MCU_elevation", "MCU_locationStatus2", 0),
      ("MCU_latControlEnable", "MCU_chassisControl", 0),
      ("MCU_fcwSensitivity", "MCU_chassisControl", 0),
      ("MCU_ldwEnable", "MCU_chassisControl", 0),
      ("MCU_aebEnable", "MCU_chassisControl", 0),
      ("MCU_pedalSafetyEnable", "MCU_chassisControl", 0),
      #("StW_AnglHP", "STW_ANGLHP_STAT", 0),
      ("DI_gear", "DI_torque2", 3),
      ("DI_brakePedal", "DI_torque2", 0),
      ("DI_vehicleSpeed", "DI_torque2", 0),
      ("DOOR_STATE_FL", "GTW_carState", 1),
      ("DOOR_STATE_FR", "GTW_carState", 1),
      ("DOOR_STATE_RL", "GTW_carState", 1),
      ("DOOR_STATE_RR", "GTW_carState", 1),
      #("DI_stateCounter", "DI_state", 0),
      #("GTW_driverPresent", "GTW_status", 0),
      ("DI_cruiseSet", "DI_state", 0),
      ("DI_cruiseState", "DI_state", 0),
      #("TSL_P_Psd_StW","SBW_RQ_SCCM" , 0),
      #("DI_motorRPM", "DI_torque1", 0),
      #("DI_pedalPos", "DI_torque1", 0),
      ("DI_torqueMotor", "DI_torque1",0),
      ("DI_speedUnits", "DI_state", 0),
      # Steering wheel stalk signals (useful for managing cruise control)
      ("SpdCtrlLvr_Stat", "STW_ACTN_RQ", 0),
      ("VSL_Enbl_Rq", "STW_ACTN_RQ", 0),
      ("SpdCtrlLvrStat_Inv", "STW_ACTN_RQ", 0),
      ("DTR_Dist_Rq", "STW_ACTN_RQ", 0),
      ("TurnIndLvr_Stat", "STW_ACTN_RQ", 0),
      ("HiBmLvr_Stat", "STW_ACTN_RQ", 0),
      ("WprWashSw_Psd", "STW_ACTN_RQ", 0),
      ("WprWash_R_Sw_Posn_V2", "STW_ACTN_RQ", 0),
      ("StW_Lvr_Stat", "STW_ACTN_RQ", 0),
      ("StW_Cond_Flt", "STW_ACTN_RQ", 0),
      ("StW_Cond_Psd", "STW_ACTN_RQ", 0),
      ("HrnSw_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw00_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw01_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw02_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw03_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw04_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw05_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw06_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw07_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw08_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw09_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw10_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw11_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw12_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw13_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw14_Psd", "STW_ACTN_RQ", 0),
      ("StW_Sw15_Psd", "STW_ACTN_RQ", 0),
      ("WprSw6Posn", "STW_ACTN_RQ", 0),
      ("MC_STW_ACTN_RQ", "STW_ACTN_RQ", 0),
      ("CRC_STW_ACTN_RQ", "STW_ACTN_RQ", 0),
      ("DI_regenLight", "DI_state",0),
      ("GTW_dasHw", "GTW_carConfig",0),
      ("GTW_fourWheelDrive", "GTW_carConfig",0),
      ("GTW_performanceConfig", "GTW_carConfig",0),

      ("SDM_bcklDrivStatus", "SDM1", 0),

      ("UI_csaOfframpCurvC3", "UI_csaOfframpCurvature", 0),
      ("UI_csaOfframpCurvC2", "UI_csaOfframpCurvature", 0),
      ("UI_csaOfframpCurvRange", "UI_csaOfframpCurvature", 0),
      ("UI_csaOfframpCurvUsingTspline", "UI_csaOfframpCurvature", 0),

      ("UI_csaRoadCurvC3", "UI_csaRoadCurvature", 0),
      ("UI_csaRoadCurvC2", "UI_csaRoadCurvature", 0),
      ("UI_csaRoadCurvRange", "UI_csaRoadCurvature", 0),
      ("UI_csaRoadCurvUsingTspline", "UI_csaRoadCurvature", 0),

      ("UI_roadCurvHealth", "UI_roadCurvature", 0),
      ("UI_roadCurvRange", "UI_roadCurvature", 0),
      ("UI_roadCurvC0", "UI_roadCurvature", 0),
      ("UI_roadCurvC1", "UI_roadCurvature", 0),
      ("UI_roadCurvC2", "UI_roadCurvature", 0),
      ("UI_roadCurvC3", "UI_roadCurvature", 0),

      ("UI_mapSpeedUnits", "UI_driverAssistMapData", 0),
      ("UI_mapSpeedLimit", "UI_driverAssistMapData", 0),

      ("UI_roadSign","UI_driverAssistRoadSign", 0),
      ("UI_meanFleetSplineAccelMPS2", "UI_driverAssistRoadSign", 0),
      ("UI_meanFleetSplineSpeedMPS", "UI_driverAssistRoadSign", 0),
      ("UI_medianFleetSpeedMPS", "UI_driverAssistRoadSign", 0),
      ("UI_topQrtlFleetSpeedMPS", "UI_driverAssistRoadSign", 0),
      ("UI_splineLocConfidence", "UI_driverAssistRoadSign", 0),
      ("UI_baseMapSpeedLimitMPS", "UI_driverAssistRoadSign", 0),
      ("UI_bottomQrtlFleetSpeedMPS", "UI_driverAssistRoadSign", 0),
      ("UI_rampType", "UI_driverAssistRoadSign", 0),

      
  ]

  checks = [
      #("STW_ANGLHP_STAT",  200), #JCT Actual message freq is 40 Hz (0.025 sec)
      ("STW_ACTN_RQ",  20), #JCT Actual message freq is 3.5 Hz (0.285 sec)
      #("SBW_RQ_SCCM", 175), #JCT Actual message freq is 35 Hz (0.0286 sec)
      ("DI_torque1", 59), #JCT Actual message freq is 11.8 Hz (0.084 sec)
      ("DI_torque2", 18), #JCT Actual message freq is 3.7 Hz (0.275 sec)
      #("MCU_gpsVehicleSpeed", 2), #JCT Actual message freq is 0.487 Hz (2.05 sec)
      ("GTW_carState", 20), #JCT Actual message freq is 3.3 Hz (0.3 sec)
      #("GTW_status", 2), #JCT Actual message freq is 0.5 Hz (2 sec)
      ("DI_state", 7), #JCT Actual message freq is 1 Hz (1 sec)
      #("MCU_locationStatus", 5), #JCT Actual message freq is 1.3 Hz (0.76 sec)
      ("GTW_carConfig", 7), #BB Actual message freq  is 1 Hz (1 sec)
  ]

  #checks = []
  return signals, checks
  
def get_epas_can_signals(CP):
# this function generates lists for signal, messages and initial values
  signals = [
      ("EPAS_torsionBarTorque", "EPAS_sysStatus", 0), # Used in interface.py
      ("EPAS_eacStatus", "EPAS_sysStatus", 0),
      ("EPAS_eacErrorCode", "EPAS_sysStatus", 0),
      ("EPAS_handsOnLevel", "EPAS_sysStatus", 0),
      ("EPAS_steeringFault", "EPAS_sysStatus", 0),
      ("EPAS_internalSAS",  "EPAS_sysStatus", 0), #BB see if this works better than STW_ANGLHP_STAT for angle
  ]

  checks = [
      ("EPAS_sysStatus", 12), #JCT Actual message freq is 1.3 Hz (0.76 sec)
  ]

  #checks = []
  return signals, checks

def get_pedal_can_signals(CP):
# this function generates lists for signal, messages and initial values
  signals = [
      ("INTERCEPTOR_GAS", "GAS_SENSOR", 0),
      ("INTERCEPTOR_GAS2", "GAS_SENSOR", 0),
      ("STATE", "GAS_SENSOR", 0),
      ("IDX", "GAS_SENSOR", 0),
  ]

  checks = []
  return signals, checks
  
def get_can_parser(CP,mydbc):
  signals, checks = get_can_signals(CP)
  return CANParser(mydbc, signals, checks, 0)

def get_epas_parser(CP,epascan):
  signals, checks = get_epas_can_signals(CP)
  return CANParser(DBC[CP.carFingerprint]['pt']+"_epas", signals, checks, epascan)

def get_pedal_parser(CP):
  signals, checks = get_pedal_can_signals(CP)
  return CANParser(DBC[CP.carFingerprint]['pt']+"_pedal", signals, checks, 2)

class CarState(object):
  def __init__(self, CP):
    self.speed_control_enabled = 0
    self.CL_MIN_V = 8.9
    self.CL_MAX_A = 20.
    # labels for buttons
    self.btns_init = [["alca",                "ALC",                      ["MadMax", "Normal", "Calm"]],
                      [ACCMode.BUTTON_NAME,   ACCMode.BUTTON_ABREVIATION, ACCMode.labels()],
                      ["dsp",               "DSP",                      ["OP","MIN","OFF","GYRO"]],
                      ["",               "",                      [""]],
                      ["msg",                 "MSG",                      [""]],
                      ["sound",               "SND",                      [""]]]
    
    ### START OF MAIN CONFIG OPTIONS ###
    ### Do NOT modify here, modify in /data/bb_openpilot.cfg and reboot
    self.forcePedalOverCC = True
    self.enableHSO = True 
    self.enableALCA = True
    self.enableDasEmulation = True
    self.enableRadarEmulation = True
    self.enableSpeedVariableDesAngle = False
    self.enableRollAngleCorrection = False
    self.enableFeedForwardAngleCorrection = True
    self.enableDriverMonitor = True
    self.enableShowCar = True
    self.enableShowLogo = True
    self.hasNoctuaFan = False
    self.limitBatteryMinMax = False
    self.limitBattery_Min = 60
    self.limitBattery_Max = 70
    self.doAutoUpdate = True
    self.blockUploadWhileTethering = False
    self.tetherIP = "127.0.0."
    self.useTeslaGPS = False
    self.useTeslaMapData = False
    self.hasTeslaIcIntegration = False
    self.useTeslaRadar = False
    self.useWithoutHarness = False
    self.radarVIN = "                 "
    self.enableLdw = True
    self.radarOffset = 0.
    self.radarPosition = 0
    self.radarEpasType = 0
    self.fix1916 = False
    self.forceFingerprintTesla = False
    self.eonToFront = 0.9
    #read config file
    read_config_file(self)
    ### END OF MAIN CONFIG OPTIONS ###

    self.apEnabled = True
    self.apFollowTimeInS =  2.5 #time in seconds to follow
    self.keepEonOff = False
    self.alcaEnabled = True
    self.mapAwareSpeed = False

    # Tesla Model
    self.teslaModelDetected = 1
    self.teslaModel = read_db('/data/params','TeslaModel')
    if self.teslaModel is None:
      self.teslaModel = "S"
      self.teslaModelDetected = 0

    # for map integration
    self.csaRoadCurvC3 = 0.
    self.csaRoadCurvC2 = 0.
    self.csaRoadCurvRange = 0.
    self.csaRoadCurvUsingTspline = 0

    self.csaOfframpCurvC3 = 0.
    self.csaOfframpCurvC2 = 0.
    self.csaOfframpCurvRange = 0.
    self.csaOfframpCurvUsingTspline = 0

    self.roadCurvHealth = 0
    self.roadCurvRange = 0.
    self.roadCurvC0 = 0.
    self.roadCurvC1 = 0.
    self.roadCurvC2 = 0.
    self.roadCurvC3 = 0.

    self.speedLimitUnits = 0
    self.speedLimit = 0

    self.meanFleetSplineSpeedMPS = 0.
    self.meanFleetSplineAccelMPS2 = 0.
    self.medianFleetSpeedMPS = 0.
    self.topQrtlFleetSplineSpeedMPS = 0.
    self.splineLocConfidence = 0
    self.baseMapSpeedLimitMPS = 0.
    self.bottomQrtlFleetSpeedMPS = 0.
    self.rampType = 0

    self.mapBasedSuggestedSpeed = 0.
    self.splineBasedSuggestedSpeed = 0.
    self.maxdrivespeed = 0.

    self.gpsLongitude = 0.
    self.gpsLatitude = 0.
    self.gpsAccuracy = 0.
    self.gpsElevation = 0.
    self.gpsHeading = 0.
    self.gpsVehicleSpeed = 0.

    self.userSpeedLimitKph = 0.
    self.userSpeedLimitOffsetKph = 0.

    self.brake_only = CP.enableCruise
    self.last_cruise_stalk_pull_time = 0
    self.CP = CP

    self.user_gas = 0.
    self.user_gas_pressed = False
    self.pedal_interceptor_state = 0
    self.pedal_interceptor_value = 0.
    self.pedal_interceptor_value2 = 0.
    self.pedal_interceptor_missed_counter = 0
    self.brake_switch_prev = 0
    self.brake_switch_ts = 0

    self.cruise_buttons = 0
    self.blinker_on = 0

    self.left_blinker_on = 0
    self.right_blinker_on = 0
    self.steer_warning = 0
    
    self.stopped = 0

    # variables used for the fake DAS creation
    self.DAS_info_frm = -1
    self.DAS_info_msg = 0
    self.DAS_status_frm = 0
    self.DAS_status_idx = 0
    self.DAS_status2_frm = 0
    self.DAS_status2_idx = 0
    self.DAS_bodyControls_frm = 0
    self.DAS_bodyControls_idx = 0
    self.DAS_lanes_frm = 0
    self.DAS_lanes_idx = 0
    self.DAS_objects_frm = 0
    self.DAS_objects_idx = 0
    self.DAS_pscControl_frm = 0
    self.DAS_pscControl_idx = 0
    self.DAS_warningMatrix0_idx = 0
    self.DAS_warningMatrix1_idx = 0
    self.DAS_warningMatrix3_idx = 0
    self.DAS_telemetryPeriodic1_idx = 0
    self.DAS_telemetryPeriodic2_idx = 0
    self.DAS_telemetryEvent1_idx = 0
    self.DAS_telemetryEvent2_idx = 0
    self.DAS_control_idx = 0

    #BB notification messages for DAS
    self.DAS_canErrors = 0
    self.DAS_plannerErrors = 0
    self.DAS_doorOpen = 0
    self.DAS_notInDrive = 0



    #BB variables for pedal CC
    self.pedal_speed_kph = 0.
    # Pedal mode is ready, i.e. hardware is present and normal cruise is off.
    self.pedal_interceptor_available = False
    self.prev_pedal_interceptor_available = False

    #BB UIEvents
    self.UE = UIEvents(self)

    #BB PCC
    self.regenLight = 0
    self.torqueLevel = 0.

    #BB variable for custom buttons
    self.cstm_btns = UIButtons(self,"Tesla Model S","tesla", self.enableShowLogo, self.enableShowCar)

    #BB custom message counter
    self.custom_alert_counter = -1 #set to 100 for 1 second display; carcontroller will take down to zero

    #BB steering_wheel_stalk last position, used by ACC and ALCA
    self.steering_wheel_stalk = None
    
    #BB carConfig data used to change IC info
    self.real_carConfig = None
    self.real_dasHw = 0

    #BB visiond last type
    self.last_visiond = self.cstm_btns.btns[3].btn_label2
    
     
    # vEgo kalman filter
    dt = 0.01
    # Q = np.matrix([[10.0, 0.0], [0.0, 100.0]])
    # R = 1e3
    self.v_ego_kf = KF1D(x0=[[0.0], [0.0]],
                         A=[[1.0, dt], [0.0, 1.0]],
                         C=[1.0, 0.0],
                         K=[[0.12287673], [0.29666309]])
    self.v_ego = 0.0

    self.imperial_speed_units = True

    # The current max allowed cruise speed. Actual cruise speed sent to the car
    # may be lower, depending on traffic.
    self.v_cruise_pcm = 0.0
    # Actual cruise speed currently active on the car.
    self.v_cruise_actual = 0.0

    #ALCA params
    self.ALCA_enabled = False
    self.ALCA_total_steps = 100
    self.ALCA_direction = 0
    self.ALCA_error = False

    self.angle_offset = 0.
    self.init_angle_offset = False
   
  def config_ui_buttons(self, pedalPresent):
    if pedalPresent:
      self.btns_init[1] = [PCCModes.BUTTON_NAME, PCCModes.BUTTON_ABREVIATION, PCCModes.labels()]
    else:
      # we don't have pedal interceptor
      self.btns_init[1] = [ACCMode.BUTTON_NAME, ACCMode.BUTTON_ABREVIATION, ACCMode.labels()]
    btn = self.cstm_btns.btns[1]
    btn.btn_name = self.btns_init[1][0]
    btn.btn_label = self.btns_init[1][1]
    btn.btn_label2 = self.btns_init[1][2][0]
    btn.btn_status = 1
    self.cstm_btns.update_ui_buttons(1, 1)    

  def compute_speed(self):
    # if one of them is zero, select max of the two
    if self.meanFleetSplineSpeedMPS == 0 or self.medianFleetSpeedMPS == 0:
      self.splineBasedSuggestedSpeed = max(self.meanFleetSplineSpeedMPS,self.medianFleetSpeedMPS)
    else:
      self.splineBasedSuggestedSpeed = (self.splineLocConfidence * self.meanFleetSplineSpeedMPS + (100-self.splineLocConfidence) * self.medianFleetSpeedMPS ) / 100.
    # if confidence over 60%, then weight between bottom speed and top speed
    # if less than 40% then use map data
    if self.splineLocConfidence > 60:
      self.mapBasedSuggestedSpeed = (self.splineLocConfidence * self.meanFleetSplineSpeedMPS + (100-self.splineLocConfidence) * self.bottomQrtlFleetSpeedMPS ) / 100.
    else:
      self.mapBasedSuggestedSpeed = self.baseMapSpeedLimitMPS
    if self.rampType > 0:
      #we are on a ramp, use the spline info if available
      if self.splineBasedSuggestedSpeed > 0:
        self.maxdrivespeed = self.splineBasedSuggestedSpeed
      else:
        self.maxdrivespeed = self.mapBasedSuggestedSpeed
    else:
      #we are on a normal road, use max of the two
      self.maxdrivespeed = max(self.mapBasedSuggestedSpeed, self.splineBasedSuggestedSpeed)

  def update_ui_buttons(self,btn_id,btn_status):
    # we only focus on btn_id=3, which is for visiond
    if (btn_id == 3) and (self.cstm_btns.btns[btn_id].btn_status > 0) and (self.last_visiond != self.cstm_btns.btns[btn_id].btn_label2):
      self.last_visiond = self.cstm_btns.btns[btn_id].btn_label2
      # we switched between wiggly and normal
      args = ["/data/openpilot/selfdrive/car/modules/ch_visiond.sh", self.cstm_btns.btns[btn_id].btn_label2]
      subprocess.Popen(args, shell = False, stdin=None, stdout=None, stderr=None, env = dict(os.environ), close_fds=True)


  def update(self, cp, epas_cp, pedal_cp):

    # car params
    v_weight_v = [0., 1.]  # don't trust smooth speed at low values to avoid premature zero snapping
    v_weight_bp = [1., 6.]   # smooth blending, below ~0.6m/s the smooth speed snaps to zero

    # update prevs, update must run once per loop
    self.prev_cruise_buttons = self.cruise_buttons
    self.prev_blinker_on = self.blinker_on

    self.prev_left_blinker_on = self.left_blinker_on
    self.prev_right_blinker_on = self.right_blinker_on

    self.steering_wheel_stalk = cp.vl["STW_ACTN_RQ"]
    self.real_carConfig = cp.vl["GTW_carConfig"]
    self.real_dasHw = cp.vl["GTW_carConfig"]['GTW_dasHw']
    self.cruise_buttons = cp.vl["STW_ACTN_RQ"]['SpdCtrlLvr_Stat']


    # ******************* parse out can *******************
    self.door_all_closed = not any([cp.vl["GTW_carState"]['DOOR_STATE_FL'], cp.vl["GTW_carState"]['DOOR_STATE_FR'],
                               cp.vl["GTW_carState"]['DOOR_STATE_RL'], cp.vl["GTW_carState"]['DOOR_STATE_RR']])  #JCT
    self.seatbelt = cp.vl["SDM1"]['SDM_bcklDrivStatus']
    #self.seatbelt = cp.vl["SDM1"]['SDM_bcklDrivStatus'] and cp.vl["GTW_status"]['GTW_driverPresent']
    if (cp.vl["GTW_carConfig"]['GTW_performanceConfig']) and (cp.vl["GTW_carConfig"]['GTW_performanceConfig'] > 0):
      prev_teslaModel = self.teslaModel
      self.teslaModel = "S"
      if (cp.vl["GTW_carConfig"]['GTW_performanceConfig'] > 1):
        self.teslaModel = self.teslaModel + "P"
      if (cp.vl["GTW_carConfig"]['GTW_fourWheelDrive'] == 1):
        self.teslaModel = self.teslaModel + "D"
      if (self.teslaModelDetected == 0) or (prev_teslaModel != self.teslaModel):
        write_db('/data/params','TeslaModel',self.teslaModel)
        self.teslaModelDetected = 1

    #Nav Map Data
    self.csaRoadCurvC3 = cp.vl['UI_csaRoadCurvature']["UI_csaRoadCurvC3"]
    self.csaRoadCurvC2 = cp.vl['UI_csaRoadCurvature']["UI_csaRoadCurvC2"]
    self.csaRoadCurvRange = cp.vl['UI_csaRoadCurvature']["UI_csaRoadCurvRange"]
    self.csaRoadCurvUsingTspline = cp.vl['UI_csaRoadCurvature']["UI_csaRoadCurvUsingTspline"]

    self.csaOfframpCurvC3 = cp.vl['UI_csaOfframpCurvature']["UI_csaOfframpCurvC3"]
    self.csaOfframpCurvC2 = cp.vl['UI_csaOfframpCurvature']["UI_csaOfframpCurvC2"]
    self.csaOfframpCurvRange = cp.vl['UI_csaOfframpCurvature']["UI_csaOfframpCurvRange"]
    self.csaOfframpCurvUsingTspline = cp.vl['UI_csaOfframpCurvature']["UI_csaOfframpCurvUsingTspline"]

    self.roadCurvHealth = cp.vl['UI_roadCurvature']["UI_roadCurvHealth"]
    self.roadCurvRange = cp.vl['UI_roadCurvature']["UI_roadCurvRange"]
    self.roadCurvC0 = cp.vl['UI_roadCurvature']["UI_roadCurvC0"]
    self.roadCurvC1 = cp.vl['UI_roadCurvature']["UI_roadCurvC1"]
    self.roadCurvC2 = cp.vl['UI_roadCurvature']["UI_roadCurvC2"]
    self.roadCurvC3 = cp.vl['UI_roadCurvature']["UI_roadCurvC3"]

    self.gpsLongitude =  cp.vl['MCU_locationStatus']["MCU_longitude"]
    self.gpsLatitude = cp.vl['MCU_locationStatus']["MCU_latitude"]
    self.gpsAccuracy = cp.vl['MCU_locationStatus']["MCU_gpsAccuracy"]
    self.gpsElevation = cp.vl['MCU_locationStatus2']["MCU_elevation"]
    self.gpsHeading = cp.vl['MCU_gpsVehicleSpeed']["MCU_gpsVehicleHeading"]
    self.gpsVehicleSpeed = cp.vl['MCU_gpsVehicleSpeed']["MCU_gpsVehicleSpeed"] * CV.KPH_TO_MS

    if (self.hasTeslaIcIntegration):
      self.apEnabled = (cp.vl["MCU_chassisControl"]["MCU_latControlEnable"] == 1)
      self.apFollowTimeInS =  1 + cp.vl["MCU_chassisControl"]["MCU_fcwSensitivity"] * 0.5
      self.keepEonOff = cp.vl["MCU_chassisControl"]["MCU_ldwEnable"] == 1
      self.alcaEnabled = cp.vl["MCU_chassisControl"]["MCU_pedalSafetyEnable"] == 1
      self.mapAwareSpeed = cp.vl["MCU_chassisControl"]["MCU_aebEnable"] == 1

    usu = cp.vl['MCU_gpsVehicleSpeed']["MCU_userSpeedOffsetUnits"]
    if usu == 1:
      self.userSpeedLimitOffsetKph = cp.vl['MCU_gpsVehicleSpeed']["MCU_userSpeedOffset"]
    else:
      self.userSpeedLimitOffsetKph = cp.vl['MCU_gpsVehicleSpeed']["MCU_userSpeedOffset"] * CV.MPH_TO_KPH
    msu = cp.vl['MCU_gpsVehicleSpeed']["MCU_mapSpeedLimitUnits"]
    if msu == 1:
      self.userSpeedLimitKph = cp.vl['MCU_gpsVehicleSpeed']["MCU_mppSpeedLimit"]
    else:
      self.userSpeedLimitKph = cp.vl['MCU_gpsVehicleSpeed']["MCU_mppSpeedLimit"] * CV.MPH_TO_KPH

    speed_limit_tesla_lookup = [0,5,7,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,130,140,150,160,0,0] 
    self.speedLimitUnits = cp.vl["UI_driverAssistMapData"]["UI_mapSpeedUnits"]
    self.speedLimitKph = speed_limit_tesla_lookup[int(cp.vl["UI_driverAssistMapData"]["UI_mapSpeedLimit"])] * (1 + 0.609 * (1 - self.speedLimitUnits))


    rdSignMsg = cp.vl["UI_driverAssistRoadSign"]["UI_roadSign"]
    if rdSignMsg == 4:
      self.meanFleetSplineSpeedMPS = cp.vl["UI_driverAssistRoadSign"]["UI_meanFleetSplineSpeedMPS"]
      self.meanFleetSplineAccelMPS2  = cp.vl["UI_driverAssistRoadSign"]["UI_meanFleetSplineAccelMPS2"]
      self.medianFleetSpeedMPS = cp.vl["UI_driverAssistRoadSign"]["UI_medianFleetSpeedMPS"]
      self.splineLocConfidence = cp.vl["UI_driverAssistRoadSign"]["UI_splineLocConfidence"]
      self.rampType = cp.vl["UI_driverAssistRoadSign"]["UI_rampType"]
      
    if rdSignMsg == 3:
      self.topQrtlFleetSplineSpeedMPS = cp.vl["UI_driverAssistRoadSign"]["UI_topQrtlFleetSpeedMPS"]
      self.splineLocConfidence = cp.vl["UI_driverAssistRoadSign"]["UI_splineLocConfidence"]
      self.baseMapSpeedLimitMPS = cp.vl["UI_driverAssistRoadSign"]["UI_baseMapSpeedLimitMPS"]
      self.bottomQrtlFleetSpeedMPS = cp.vl["UI_driverAssistRoadSign"]["UI_bottomQrtlFleetSpeedMPS"]
    
    self.compute_speed()

    # 2 = temporary 3= TBD 4 = temporary, hit a bump 5 (permanent) 6 = temporary 7 (permanent)
    # TODO: Use values from DBC to parse this field
    self.steer_error = epas_cp.vl["EPAS_sysStatus"]['EPAS_steeringFault'] == 1
    self.steer_not_allowed = epas_cp.vl["EPAS_sysStatus"]['EPAS_eacStatus'] not in [2,1] # 2 "EAC_ACTIVE" 1 "EAC_AVAILABLE" 3 "EAC_FAULT" 0 "EAC_INHIBITED"
    self.steer_warning = self.steer_not_allowed
    self.brake_error = 0 #NOT WORKINGcp.vl[309]['ESP_brakeLamp'] #JCT
    # JCT More ESP errors available, these needs to be added once car steers on its own to disable / alert driver
    self.esp_disabled = 0 #NEED TO CORRECT DBC cp.vl[309]['ESP_espOffLamp'] or cp.vl[309]['ESP_tcOffLamp'] or cp.vl[309]['ESP_tcLampFlash'] or cp.vl[309]['ESP_espFaultLamp'] #JCT

    # calc best v_ego estimate, by averaging two opposite corners
    self.v_wheel_fl = 0 #JCT
    self.v_wheel_fr = 0 #JCT
    self.v_wheel_rl = 0 #JCT
    self.v_wheel_rr = 0 #JCT
    self.v_wheel = 0 #JCT
    self.v_weight = 0 #JCT
    speed = (cp.vl["DI_torque2"]['DI_vehicleSpeed']) * CV.MPH_TO_KPH/3.6 #JCT MPH_TO_MS. Tesla is in MPH, v_ego is expected in M/S
    speed = speed * 1.01 # To match car's displayed speed

    if abs(speed - self.v_ego) > 2.0:  # Prevent large accelerations when car starts at non zero speed
      self.v_ego_kf.x = [[speed], [0.0]]

    self.v_ego_raw = speed
    v_ego_x = self.v_ego_kf.update(speed)
    self.v_ego = float(v_ego_x[0])
    self.a_ego = float(v_ego_x[1])


    #BB use this set for pedal work as the user_gas_xx is used in other places
    self.pedal_interceptor_state = pedal_cp.vl["GAS_SENSOR"]['STATE']
    self.pedal_interceptor_value = pedal_cp.vl["GAS_SENSOR"]['INTERCEPTOR_GAS']
    self.pedal_interceptor_value2 = pedal_cp.vl["GAS_SENSOR"]['INTERCEPTOR_GAS2']

    can_gear_shifter = cp.vl["DI_torque2"]['DI_gear']
    self.gear = 0 # JCT

    # self.angle_steers  = -(cp.vl["STW_ANGLHP_STAT"]['StW_AnglHP']) #JCT polarity reversed from Honda/Acura
    self.angle_steers = -(epas_cp.vl["EPAS_sysStatus"]['EPAS_internalSAS'])  #BB see if this works better than STW_ANGLHP_STAT for angle
    
    self.angle_steers_rate = 0 #JCT

    self.blinker_on = (cp.vl["STW_ACTN_RQ"]['TurnIndLvr_Stat'] == 1) or (cp.vl["STW_ACTN_RQ"]['TurnIndLvr_Stat'] == 2)
    self.left_blinker_on = cp.vl["STW_ACTN_RQ"]['TurnIndLvr_Stat'] == 1
    self.right_blinker_on = cp.vl["STW_ACTN_RQ"]['TurnIndLvr_Stat'] == 2

    #if self.CP.carFingerprint in (CAR.CIVIC, CAR.ODYSSEY):
    #  self.park_brake = cp.vl["EPB_STATUS"]['EPB_STATE'] != 0
    #  self.brake_hold = cp.vl["VSA_STATUS"]['BRAKE_HOLD_ACTIVE']
    #  self.main_on = cp.vl["SCM_FEEDBACK"]['MAIN_ON']
    #else:
    self.park_brake = 0  # TODO
    self.brake_hold = 0  # TODO

    self.main_on = 1 #cp.vl["SCM_BUTTONS"]['MAIN_ON']
    self.imperial_speed_units = cp.vl["DI_state"]['DI_speedUnits'] == 0
    self.DI_cruiseSet = cp.vl["DI_state"]['DI_cruiseSet']
    if self.imperial_speed_units:
      self.DI_cruiseSet = self.DI_cruiseSet * CV.MPH_TO_KPH
    self.cruise_speed_offset = calc_cruise_offset(self.DI_cruiseSet*CV.KPH_TO_MS, self.v_ego)
    self.gear_shifter = parse_gear_shifter(can_gear_shifter, self.CP.carFingerprint)

    self.pedal_gas = 0. # cp.vl["DI_torque1"]['DI_pedalPos'] / 102 #BB: to make it between 0..1
    self.car_gas = self.pedal_gas

    self.steer_override = abs(epas_cp.vl["EPAS_sysStatus"]['EPAS_handsOnLevel']) > 0
    self.steer_torque_driver = 0 #JCT

    # brake switch has shown some single time step noise, so only considered when
    # switch is on for at least 2 consecutive CAN samples
    # Todo / refactor: This shouldn't have to do with epas == 3..
    # was wrongly set to epas_cp.vl["EPAS_sysStatus"]['EPAS_eacErrorCode'] == 3 and epas_cp.vl["EPAS_sysStatus"]['EPAS_eacStatus'] == 0
    self.brake_switch = cp.vl["DI_torque2"]['DI_brakePedal']
    self.brake_pressed = cp.vl["DI_torque2"]['DI_brakePedal']

    self.standstill = cp.vl["DI_torque2"]['DI_vehicleSpeed'] == 0
    self.torqueMotor = cp.vl["DI_torque1"]['DI_torqueMotor']
    self.pcm_acc_status = cp.vl["DI_state"]['DI_cruiseState']
    
    self.regenLight = cp.vl["DI_state"]['DI_regenLight'] == 1
    
    self.prev_pedal_interceptor_available = self.pedal_interceptor_available
    pedal_has_value = bool(self.pedal_interceptor_value) or bool(self.pedal_interceptor_value2)
    pedal_interceptor_present = self.pedal_interceptor_state in [0, 5] and pedal_has_value
    # Add loggic if we just miss some CAN messages so we don't immediately disable pedal
    if pedal_has_value:
      self.pedal_interceptor_missed_counter = 0
    if pedal_interceptor_present:
      self.pedal_interceptor_missed_counter = 0
    else:
      self.pedal_interceptor_missed_counter += 1
    pedal_interceptor_present = pedal_interceptor_present and (self.pedal_interceptor_missed_counter < 10)
    # Mark pedal unavailable while traditional cruise is on.
    self.pedal_interceptor_available = pedal_interceptor_present and (self.forcePedalOverCC or not bool(self.pcm_acc_status))
    if self.pedal_interceptor_available != self.prev_pedal_interceptor_available:
        self.config_ui_buttons(self.pedal_interceptor_available)

    self.v_cruise_actual = self.DI_cruiseSet
    self.hud_lead = 0 #JCT
    self.cruise_speed_offset = calc_cruise_offset(self.v_cruise_pcm, self.v_ego)


# carstate standalone tester
if __name__ == '__main__':

  class CarParams(object):
    def __init__(self):
      self.carFingerprint = "TESLA MODEL S"
      self.enableCruise = 0
  CP = CarParams()
  CS = CarState(CP)

  # while 1:
  #   CS.update()
  #   time.sleep(0.01)
