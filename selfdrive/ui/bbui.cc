#include <math.h>
#include "cereal/gen/cpp/ui.capnp.h"

#if !defined(QCOM) && !defined(QCOM2)
#ifndef __APPLE__
#define GLFW_INCLUDE_ES2
#else
#define GLFW_INCLUDE_GLCOREARB
#endif

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
int linux_abs_x = 0;
int linux_abs_y = 0;
UIState *mouse_ui_state;
#endif
// TODO: this is also hardcoded in common/transformations/camera.py

#ifdef QCOM2
const int vwp_w = 2160;
#else
const int vwp_w = 1920;
#endif
const int vwp_h = 1080;
#include "paint.hpp"

void bb_ui_preinit(UIState *s) {
  s->b.scr_w = vwp_w;
  s->b.scr_h = vwp_h;
  s->b.scr_scale_x = 1.0f;
  s->b.scr_scale_y = 1.0f;
  s->b.scr_device_factor = 1.0f;
  s->b.scr_scissor_offset = 0.0f;
}

vec3 bb_car_space_to_full_frame(const  UIState *s, vec4 car_space_projective) {
  const UIScene *scene = &s->scene;

  // We'll call the car space point p.
  // First project into normalized image coordinates with the extrinsics matrix.
  const vec4 Ep4 = matvecmul(scene->extrinsic_matrix, car_space_projective);

  // The last entry is zero because of how we store E (to use matvecmul).
  const vec3 Ep = {{Ep4.v[0], Ep4.v[1], Ep4.v[2]}};
  const vec3 KEp = matvecmul3(intrinsic_matrix, Ep);

  // Project.
  const vec3 p_image = {{KEp.v[0] / KEp.v[2], KEp.v[1] / KEp.v[2], 1.}};
  return p_image;
}

void bb_ui_draw_car(  UIState *s) {
  // replaces the draw_chevron function when button in mid position
  //static void draw_chevron(UIState *s, float x_in, float y_in, float sz,
  //                        NVGcolor fillColor, NVGcolor glowColor) {
  if (!s->scene.lead_data[0].getStatus()) {
    //no lead car to draw
    return;
  }
  if (!s->b.icShowCar) {
    return;
  }
  const UIScene *scene = &s->scene;
  float x_in = scene->lead_data[0].getDRel()+2.7;
  float y_in = scene->lead_data[0].getYRel();

  nvgSave(s->vg);
  nvgTranslate(s->vg, 240.0f, 0.0);
  nvgTranslate(s->vg, -1440.0f / 2, -1080.0f / 2);
  nvgScale(s->vg, 2.0, 2.0);
  nvgScale(s->vg, 1440.0f / s->stream.bufs_info.width, 1080.0f / s->stream.bufs_info.height);

  const vec4 p_car_space = (vec4){{x_in, y_in, 0., 1.}};
  const vec3 p_full_frame = bb_car_space_to_full_frame(s, p_car_space);

  float x = p_full_frame.v[0];
  float y = p_full_frame.v[1];

  // glow

  float bbsz =0.1*  0.1 + 0.6 * (180-x_in*3.3-20)/180;
  if (bbsz < 0.1) bbsz = 0.1;
  if (bbsz > 0.6) bbsz = 0.6;
  float car_alpha = .8;
  float car_w = 750 * bbsz;
  float car_h = 531 * bbsz;
  float car_y = y - car_h/2;
  float car_x = x - car_w/2;
  
  nvgBeginPath(s->vg);
  NVGpaint imgPaint = nvgImagePattern(s->vg, car_x, car_y,
  car_w, car_h, 0, s->b.img_car, car_alpha);
  nvgRect(s->vg, car_x, car_y, car_w, car_h);
  nvgFillPaint(s->vg, imgPaint);
  nvgFill(s->vg);
  nvgRestore(s->vg);
}

void bb_draw_lane_fill ( UIState *s) {

  const UIScene *scene = &s->scene;

  nvgSave(s->vg);
  nvgTranslate(s->vg, 240.0f, 0.0); // rgb-box space
  nvgTranslate(s->vg, -1440.0f / 2, -1080.0f / 2); // zoom 2x
  nvgScale(s->vg, 2.0, 2.0);
  nvgScale(s->vg, 1440.0f / s->stream.bufs_info.width, 1080.0f / s->stream.bufs_info.height);
  nvgBeginPath(s->vg);

  //BB let the magic begin
  //define variables
  float off;
  NVGcolor bb_color = nvgRGBA(138, 140, 142,180);
  bool started = false;
  bool is_ghost = true;
  //left lane, first init
  auto path = scene->model.getLaneLines()[1];
  auto points = path.getY();
  off = 0.025*scene->model.getLaneLineProbs()[1];
  //draw left
  float px = 0;
  for (auto point = points.begin(); point != points.end(); point++) {
    float py = *point - off;
    vec4 p_car_space = (vec4){{px, py, 0., 1.}};
    vec3 p_full_frame = bb_car_space_to_full_frame(s, p_car_space);
    float x = p_full_frame.v[0];
    float y = p_full_frame.v[1];
    if (x < 0 || y < 0.) {
      px++;
      continue;
    }
    if (!started) {
      nvgMoveTo(s->vg, x, y);
      started = true;
    } else {
      nvgLineTo(s->vg, x, y);
    }
    px++;
  }
  //right lane, first init
  path = scene->model.getLaneLines()[2];
  points = path.getY();
  off = 0.025*scene->model.getLaneLineProbs()[2];
  //draw right
  px = points.size();
  for (auto point = points.end(); point != points.begin();) {
    --point;
    px--;
    float py = is_ghost?(*point- - off):(*point + off);
    vec4 p_car_space = (vec4){{px, py, 0., 1.}};
    vec3 p_full_frame = bb_car_space_to_full_frame(s, p_car_space);
    float x = p_full_frame.v[0];
    float y = p_full_frame.v[1];
    if (x < 0 || y < 0.) {
      continue;
    }
    nvgLineTo(s->vg, x, y);
  }

  nvgClosePath(s->vg);
  nvgFillColor(s->vg, bb_color);
  nvgFill(s->vg);
  nvgRestore(s->vg);
}





long bb_currentTimeInMilis() {
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (res.tv_sec * 1000) + res.tv_nsec/1000000;
}

int bb_get_status( UIState *s) {
    //BB select status based on main s->status w/ priority over s->b.custom_message_status
    int bb_status = -1;
    if ((s->status == STATUS_ENGAGED) && (s->b.custom_message_status > STATUS_ENGAGED)) {
      bb_status = s->b.custom_message_status;
    } else {
      bb_status = s->status;
    }
    return bb_status;
}

int bb_ui_draw_measure( UIState *s,  const char* bb_value, const char* bb_uom, const char* bb_label, 
		int bb_x, int bb_y, int bb_uom_dx,
		NVGcolor bb_valueColor, NVGcolor bb_labelColor, NVGcolor bb_uomColor, 
		int bb_valueFontSize, int bb_labelFontSize, int bb_uomFontSize ) {
  
  nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
  int dx = 0;
  if (strlen(bb_uom) > 0) {
  	dx = (int)(bb_uomFontSize*2.5/2);
   }
  //print value
  nvgFontFace(s->vg, "sans-semibold");
  nvgFontSize(s->vg, bb_valueFontSize*2.5);
  nvgFillColor(s->vg, bb_valueColor);
  nvgText(s->vg, bb_x-dx/2, bb_y+ (int)(bb_valueFontSize*2.5)+5, bb_value, NULL);
  //print label
  nvgFontFace(s->vg, "sans-regular");
  nvgFontSize(s->vg, bb_labelFontSize*2.5);
  nvgFillColor(s->vg, bb_labelColor);
  nvgText(s->vg, bb_x, bb_y + (int)(bb_valueFontSize*2.5)+5 + (int)(bb_labelFontSize*2.5)+5, bb_label, NULL);
  //print uom
  if (strlen(bb_uom) > 0) {
      nvgSave(s->vg);
	  int rx =bb_x + bb_uom_dx + bb_valueFontSize -3;
	  int ry = bb_y + (int)(bb_valueFontSize*2.5/2)+25;
	  nvgTranslate(s->vg,rx,ry);
	  nvgRotate(s->vg, -1.5708); //-90deg in radians
	  nvgFontFace(s->vg, "sans-regular");
	  nvgFontSize(s->vg, (int)(bb_uomFontSize*2.5));
	  nvgFillColor(s->vg, bb_uomColor);
	  nvgText(s->vg, 0, 0, bb_uom, NULL);
	  nvgRestore(s->vg);
  }
  return (int)((bb_valueFontSize + bb_labelFontSize)*2.5) + 5;
}



bool bb_handle_ui_touch( UIState *s, int touch_x, int touch_y) {
#if !defined(QCOM) && !defined(QCOM2)
  touch_x = (int)(vwp_w * touch_x / 1280);
  touch_y = (int)(vwp_h * touch_y / 720);
  printf("Linux mouse up at %d, %d  ( %d, %d)\n",(int)touch_x, (int)touch_y, linux_abs_x, linux_abs_y);
#endif
  for(int i=0; i<6; i++) {
    if (s->b.btns_r[i] > 0) {
      if ((abs(touch_x - s->b.btns_x[i]) < s->b.btns_r[i]) && (abs(touch_y - s->b.btns_y[i]) < s->b.btns_r[i])) {
        //found it; change the status
        if (s->b.btns_status[i] > 0) {
          s->b.btns_status[i] = 0;
        } else {
          s->b.btns_status[i] = 1;
        }
        //now let's send the cereal
        
        ::capnp::MallocMessageBuilder message;
        cereal::UIButtonStatus::Builder btn = message.initRoot<cereal::UIButtonStatus>();

        btn.setBtnId(i);
        btn.setBtnStatus(s->b.btns_status[i]);

        kj::Array<capnp::word> words = messageToFlatArray(message);
        kj::ArrayPtr<kj::byte> bytes = words.asBytes();

        s->b.uiButtonStatus_sock->send((char *)(bytes.begin()), bytes.size());

        return true;
      }
    }
  }
  return false;
};

#if !defined(QCOM) && !defined(QCOM2)
void bb_mouse_event_handler(GLFWwindow* window, int button, int action, int mods) {
   if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {

        double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	int w_width,w_height;
	glfwGetWindowSize(window, &linux_abs_x, &linux_abs_y);
        bb_handle_ui_touch(mouse_ui_state,(int) xpos, (int)ypos);
   }
}
#endif


int bb_get_button_status(UIState *s, char *btn_name) {
  int ret_status = -1;
  for (int i = 0; i< 6; i++) {
    if (strcmp(s->b.btns[i].btn_name,btn_name)==0) {
      ret_status = s->b.btns_status[i];
    }
  }
  return ret_status;
}

void bb_draw_button( UIState *s, int btn_id) {
  const UIScene *scene = &s->scene;

  const Rect &viz_rect = s->scene.viz_rect;
  const int box_y = viz_rect.y;

  int viz_button_x = 0;
  int viz_button_y = (box_y + (bdr_s*1.5)) + 20;
  int viz_button_w = 140;
  int viz_button_h = 140;

  char *btn_text, *btn_text2;
  
  int delta_x = viz_button_w * 1.1;
  int delta_y = 0;
  int dx1 = 200;
  int dx2 = 190;
  int dy = 0;

  if (s->b.tri_state_switch ==2) {
    delta_y = delta_x;
    delta_x = 0;
    dx1 = 20;
    dx2 = 160;
    dy = 200;
  }

  if (btn_id >2) {
    viz_button_x = scene->viz_rect.x + scene->viz_rect.w - (bdr_s*2) -dx2;
    viz_button_x -= (6-btn_id) * delta_x ;
    viz_button_y += (btn_id-3) * delta_y + dy;
    
  } else {
    viz_button_x = scene->viz_rect.x + (bdr_s*2) + dx1;
    viz_button_x +=  (btn_id) * delta_x;
    viz_button_y += btn_id * delta_y + dy;
  }
  

  btn_text = s->b.btns[btn_id].btn_label;
  btn_text2 = s->b.btns[btn_id].btn_label2;
  
  if (strcmp(btn_text,"")==0) {
    s->b.btns_r[btn_id] = 0;
  } else {
    s->b.btns_r[btn_id]= (int)((viz_button_w + viz_button_h)/4);
  }
  s->b.btns_x[btn_id]=viz_button_x + s->b.btns_r[btn_id];
  s->b.btns_y[btn_id]=viz_button_y + s->b.btns_r[btn_id];
  if (s->b.btns_r[btn_id] == 0) {
    return;
  }
  
  nvgBeginPath(s->vg);
  nvgRoundedRect(s->vg, viz_button_x, viz_button_y, viz_button_w, viz_button_h, 80);
  nvgStrokeWidth(s->vg, 12);

  
  if (s->b.btns_status[btn_id] ==0) {
    //disabled - red
    nvgStrokeColor(s->vg, nvgRGBA(255, 0, 0, 200));
    if (strcmp(btn_text2,"")==0) {
      btn_text2 = (char *)"Off";
    }
  } else
  if (s->b.btns_status[btn_id] ==1) {
    //enabled - white
    nvgStrokeColor(s->vg, nvgRGBA(255,255,255,200));
    nvgStrokeWidth(s->vg, 4);
    if (strcmp(btn_text2,"")==0) {
      btn_text2 = (char *)"Ready";
    }
  } else
  if (s->b.btns_status[btn_id] ==2) {
    //active - green
    nvgStrokeColor(s->vg, nvgRGBA(28, 204,98,200));
    if (strcmp(btn_text2,"")==0) {
      btn_text2 = (char *)"Active";
    }
  } else
  if (s->b.btns_status[btn_id] ==9) {
    //available - thin white
    nvgStrokeColor(s->vg, nvgRGBA(200,200,200,40));
    nvgStrokeWidth(s->vg, 4);
    if (strcmp(btn_text2,"")==0) {
      btn_text2 = (char *)"";
    }
  } else {
    //others - orange
    nvgStrokeColor(s->vg, nvgRGBA(255, 188, 3, 200));
    if (strcmp(btn_text2,"")==0) {
      btn_text2 = (char *)"Alert";
    }
  }

  nvgStroke(s->vg);

  nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
  nvgFontFace(s->vg, "sans-regular");
  nvgFontSize(s->vg, 14*2.5);
  nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 200));
  nvgText(s->vg, viz_button_x+viz_button_w/2, viz_button_y + 112, btn_text2, NULL);

  nvgFontFace(s->vg, "sans-semibold");
  nvgFontSize(s->vg, 28*2.5);
  nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 255));
  nvgText(s->vg, viz_button_x+viz_button_w/2, viz_button_y + 85,btn_text, NULL);
}

void bb_draw_buttons( UIState *s) {
  for (int i = 0; i < 6; i++) {
    bb_draw_button(s,i);
  }
}

void ui_draw_vision_alert_b(UIState *s);
void bb_ui_draw_custom_alert( UIState *s) {
    if ((strlen(s->b.custom_message) > 0) && (s->scene.alert_text1.length()==0)){
      if ((!((bb_get_button_status(s,(char *)"msg") == 0) && (s->b.custom_message_status<=3))) && (s->vision_connected == true)) {
        s->scene.alert_size = cereal::ControlsState::AlertSize::MID;
        s->scene.alert_text1 = s->b.custom_message;
        ui_draw_vision_alert_b(s);
      }
    } 
}

void bb_ui_draw_measures_left( UIState *s, int bb_x, int bb_y, int bb_w ) {
  int bb_rx = bb_x + (int)(bb_w/2);
	int bb_ry = bb_y;
	int bb_h = 5; 
	NVGcolor lab_color = nvgRGBA(255, 255, 255, 200);
	NVGcolor uom_color = nvgRGBA(255, 255, 255, 200);
	int value_fontSize=30;
	int label_fontSize=15;
	int uom_fontSize = 15;
	int bb_uom_dx =  (int)(bb_w /2 - uom_fontSize*2.5) ;
	
	//add CPU temperature
	if (true) {
	    	char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
			if((int)(s->b.maxCpuTemp/10) > 80) {
				val_color = nvgRGBA(255, 188, 3, 200);
			}
			if((int)(s->b.maxCpuTemp/10) > 92) {
				val_color = nvgRGBA(255, 0, 0, 200);
			}
			// temp is alway in C * 10
			if (s->is_metric) {
				 snprintf(val_str, sizeof(val_str), "%d C", (int)(s->b.maxCpuTemp/10));
			} else {
				 snprintf(val_str, sizeof(val_str), "%d F", (int)(32+9*(s->b.maxCpuTemp/10)/5));
			}
		snprintf(uom_str, sizeof(uom_str), "");
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "CPU TEMP", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}

   //add battery temperature
	if (true) {
		char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
		if((int)(s->b.maxBatTemp/1000) > 40) {
			val_color = nvgRGBA(255, 188, 3, 200);
		}
		if((int)(s->b.maxBatTemp/1000) > 50) {
			val_color = nvgRGBA(255, 0, 0, 200);
		}
		// temp is alway in C * 1000
		if (s->is_metric) {
			 snprintf(val_str, sizeof(val_str), "%d C", (int)(s->b.maxBatTemp/1000));
		} else {
			 snprintf(val_str, sizeof(val_str), "%d F", (int)(32+9*(s->b.maxBatTemp/1000)/5));
		}
		snprintf(uom_str, sizeof(uom_str), "");
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "BAT TEMP", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
	
	//add grey panda GPS accuracy
	if (true) {
		char val_str[16];
		char uom_str[3];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
		//show red/orange if gps accuracy is high
	    if(s->b.gpsAccuracy > 0.59) {
	       val_color = nvgRGBA(255, 188, 3, 200);
	    }
	    if(s->b.gpsAccuracy > 0.8) {
	       val_color = nvgRGBA(255, 0, 0, 200);
	    }


		// gps accuracy is always in meters
		if (true) {
			 snprintf(val_str, sizeof(val_str), "%d", (int)(s->b.gpsAccuracy*100.0));
		} else {
			 snprintf(val_str, sizeof(val_str), "%.1f", s->b.gpsAccuracy * 3.28084 * 12);
		}
		if (true) {
			snprintf(uom_str, sizeof(uom_str), "cm");;
		} else {
			snprintf(uom_str, sizeof(uom_str), "in");
		}
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "GPS PREC", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
  //add free space - from bthaler1
	if (true) {
		char val_str[16];
		char uom_str[3];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);

		//show red/orange if free space is low
		if(s->b.freeSpace < 0.4) {
			val_color = nvgRGBA(255, 188, 3, 200);
		}
		if(s->b.freeSpace < 0.2) {
			val_color = nvgRGBA(255, 0, 0, 200);
		}

		snprintf(val_str, sizeof(val_str), "%.1f", s->b.freeSpace* 100);
		snprintf(uom_str, sizeof(uom_str), "%%");

		bb_h +=bb_ui_draw_measure(s, val_str, uom_str, "FREE", 
			bb_rx, bb_ry, bb_uom_dx,
			val_color, lab_color, uom_color, 
			value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}	
	//finally draw the frame
	bb_h += 20;
	nvgBeginPath(s->vg);
  	nvgRoundedRect(s->vg, bb_x, bb_y, bb_w, bb_h, 20);
  	nvgStrokeColor(s->vg, nvgRGBA(255,255,255,80));
  	nvgStrokeWidth(s->vg, 6);
  	nvgStroke(s->vg);
}

void bb_ui_draw_measures_left2( UIState *s, int bb_x, int bb_y, int bb_w ) {
	int bb_rx = bb_x + (int)(bb_w/2);
	int bb_ry = bb_y;
	int bb_h = 5; 
	NVGcolor lab_color = nvgRGBA(255, 255, 255, 200);
	NVGcolor uom_color = nvgRGBA(255, 255, 255, 200);
	int value_fontSize=30;
	int label_fontSize=15;
	int uom_fontSize = 15;
	int bb_uom_dx =  (int)(bb_w /2 - uom_fontSize*2.5) ;
	
	//add CPU temperature
	if (true) {
	    	char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
			if((int)(s->b.maxCpuTemp/10) > 80) {
				val_color = nvgRGBA(255, 188, 3, 200);
			}
			if((int)(s->b.maxCpuTemp/10) > 92) {
				val_color = nvgRGBA(255, 0, 0, 200);
			}
			// temp is alway in C * 10
			if (s->is_metric) {
				 snprintf(val_str, sizeof(val_str), "%d C", (int)(s->b.maxCpuTemp/10));
			} else {
				 snprintf(val_str, sizeof(val_str), "%d F", (int)(32+9*(s->b.maxCpuTemp/10)/5));
			}
		snprintf(uom_str, sizeof(uom_str), "");
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "CPU TEMP", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}

   //add battery temperature
	if (true) {
		char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
		if((int)(s->b.maxBatTemp/1000) > 40) {
			val_color = nvgRGBA(255, 188, 3, 200);
		}
		if((int)(s->b.maxBatTemp/1000) > 50) {
			val_color = nvgRGBA(255, 0, 0, 200);
		}
		// temp is alway in C * 1000
		if (s->is_metric) {
			 snprintf(val_str, sizeof(val_str), "%d C", (int)(s->b.maxBatTemp/1000));
		} else {
			 snprintf(val_str, sizeof(val_str), "%d F", (int)(32+9*(s->b.maxBatTemp/1000)/5));
		}
		snprintf(uom_str, sizeof(uom_str), "");
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "BAT TEMP", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}

  //add battery level (%)
	if (true) {
		char val_str[16];
		char uom_str[3];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);

		//show red/orange if free space is low
		if(s->b.batteryPercent < 40) {
			val_color = nvgRGBA(255, 188, 3, 200);
		}
		if(s->b.batteryPercent < 20) {
			val_color = nvgRGBA(255, 0, 0, 200);
		}

		snprintf(val_str, sizeof(val_str), "%d", s->b.batteryPercent);
		snprintf(uom_str, sizeof(uom_str), "%%");

		bb_h +=bb_ui_draw_measure(s, val_str, uom_str, "BAT LOAD", 
			bb_rx, bb_ry, bb_uom_dx,
			val_color, lab_color, uom_color, 
			value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}	
	
	//add charger status
	if (true) {
		char val_str[16];
		char uom_str[3];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
		//show orange if charger is off
	    if(!s->b.chargingEnabled) {
	       val_color = nvgRGBA(255, 188, 3, 200);
	    }
		// gps accuracy is always in meters
		if (s->b.chargingEnabled) {
			 snprintf(val_str, sizeof(val_str), "ON");
		} else {
			 snprintf(val_str, sizeof(val_str), "OFF");
		}
		snprintf(uom_str, sizeof(uom_str), " ");
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "CHARGER", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
  
	//finally draw the frame
	bb_h += 20;
	nvgBeginPath(s->vg);
  	nvgRoundedRect(s->vg, bb_x, bb_y, bb_w, bb_h, 20);
  	nvgStrokeColor(s->vg, nvgRGBA(255,255,255,80));
  	nvgStrokeWidth(s->vg, 6);
  	nvgStroke(s->vg);
}


void bb_ui_draw_measures_right( UIState *s, int bb_x, int bb_y, int bb_w ) {
	const UIScene *scene = &s->scene;		
	int bb_rx = bb_x + (int)(bb_w/2);
	int bb_ry = bb_y;
	int bb_h = 5; 
	NVGcolor lab_color = nvgRGBA(255, 255, 255, 200);
	NVGcolor uom_color = nvgRGBA(255, 255, 255, 200);
	int value_fontSize=30;
	int label_fontSize=15;
	int uom_fontSize = 15;
	int bb_uom_dx =  (int)(bb_w /2 - uom_fontSize*2.5) ;
	
	//add visual radar relative distance
	if (true) {
		char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
		if (scene->lead_data[0].getStatus()) {
			//show RED if less than 5 meters
			//show orange if less than 15 meters
			if((int)(scene->lead_data[0].getDRel()) < 15) {
				val_color = nvgRGBA(255, 188, 3, 200);
			}
			if((int)(scene->lead_data[0].getDRel()) < 5) {
				val_color = nvgRGBA(255, 0, 0, 200);
			}
			// lead car relative distance is always in meters
			if (s->is_metric) {
				 snprintf(val_str, sizeof(val_str), "%d", (int)scene->lead_data[0].getDRel());
			} else {
				 snprintf(val_str, sizeof(val_str), "%d", (int)(scene->lead_data[0].getDRel() * 3.28084));
			}
		} else {
		   snprintf(val_str, sizeof(val_str), "-");
		}
		if (s->is_metric) {
			snprintf(uom_str, sizeof(uom_str), "m   ");
		} else {
			snprintf(uom_str, sizeof(uom_str), "ft");
		}
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "REL DIST", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
	
	//add visual radar relative speed
	if (true) {
		char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
		if (scene->lead_data[0].getStatus()) {
			//show Orange if negative speed (approaching)
			//show Orange if negative speed faster than 5mph (approaching fast)
			if((int)(scene->lead_data[0].getVRel()) < 0) {
				val_color = nvgRGBA(255, 188, 3, 200);
			}
			if((int)(scene->lead_data[0].getVRel()) < -5) {
				val_color = nvgRGBA(255, 0, 0, 200);
			}
			// lead car relative speed is always in meters
			if (s->is_metric) {
				 snprintf(val_str, sizeof(val_str), "%d", (int)(scene->lead_data[0].getVRel() * 3.6 + 0.5));
			} else {
				 snprintf(val_str, sizeof(val_str), "%d", (int)(scene->lead_data[0].getVRel() * 2.2374144 + 0.5));
			}
		} else {
		   snprintf(val_str, sizeof(val_str), "-");
		}
		if (s->is_metric) {
			snprintf(uom_str, sizeof(uom_str), "km/h");;
		} else {
			snprintf(uom_str, sizeof(uom_str), "mph");
		}
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "REL SPD", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
	
	//add  steering angle
	if (true) {
		char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
			//show Orange if more than 6 degrees
			//show red if  more than 12 degrees
			if(((int)(s->b.angleSteers) < -6) || ((int)(s->b.angleSteers) > 6)) {
				val_color = nvgRGBA(255, 188, 3, 200);
			}
			if(((int)(s->b.angleSteers) < -12) || ((int)(s->b.angleSteers) > 12)) {
				val_color = nvgRGBA(255, 0, 0, 200);
			}
			// steering is in degrees
			snprintf(val_str, sizeof(val_str), "%.1f",(s->b.angleSteers));

	    snprintf(uom_str, sizeof(uom_str), "deg");
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "STEER", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
	
	//add  desired steering angle
	if (true) {
		char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
			//show Orange if more than 6 degrees
			//show red if  more than 12 degrees
			if(((int)(s->b.angleSteersDes) < -6) || ((int)(s->b.angleSteersDes) > 6)) {
				val_color = nvgRGBA(255, 188, 3, 200);
			}
			if(((int)(s->b.angleSteersDes) < -12) || ((int)(s->b.angleSteersDes) > 12)) {
				val_color = nvgRGBA(255, 0, 0, 200);
			}
			// steering is in degrees
			snprintf(val_str, sizeof(val_str), "%.1f",(s->b.angleSteersDes));

	    snprintf(uom_str, sizeof(uom_str), "deg");
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "DES STEER", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
	
	
	//finally draw the frame
	bb_h += 20;
	nvgBeginPath(s->vg);
  	nvgRoundedRect(s->vg, bb_x, bb_y, bb_w, bb_h, 20);
  	nvgStrokeColor(s->vg, nvgRGBA(255,255,255,80));
  	nvgStrokeWidth(s->vg, 6);
  	nvgStroke(s->vg);
}

void bb_ui_draw_measures_right2( UIState *s, int bb_x, int bb_y, int bb_w ) {
	int bb_rx = bb_x + (int)(bb_w/2);
	int bb_ry = bb_y;
	int bb_h = 5; 
	NVGcolor lab_color = nvgRGBA(255, 255, 255, 200);
	NVGcolor uom_color = nvgRGBA(255, 255, 255, 200);
	int value_fontSize=30;
	int label_fontSize=15;
	int uom_fontSize = 15;
	int bb_uom_dx =  (int)(bb_w /2 - uom_fontSize*2.5) ;
	
	//Fan speed
	if (true) {
		char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
    //show RED if at max
    //show orange if lower than 16000
    if (s->b.fanSpeed == 0) {
      snprintf(val_str, sizeof(val_str), "OFF");
    } else {
      if((int)(s->b.fanSpeed) < 12000) {
        val_color = nvgRGBA(255, 188, 3, 200);
        s->b.fanSpeed =  12000;
      }
      if((int)(s->b.fanSpeed) > 65000) {
        val_color = nvgRGBA(255, 0, 0, 200);
      }
      snprintf(val_str, sizeof(val_str), "%d", (int)(s->b.fanSpeed * 4500.0 /65535.0));
    }
    snprintf(uom_str, sizeof(uom_str), "RPM");
    bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "FAN SPEED", 
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color, 
        value_fontSize, label_fontSize, uom_fontSize );
    bb_ry = bb_y + bb_h;
  }
	
	//add grey panda GPS accuracy
	if (true) {
		char val_str[16];
		char uom_str[3];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
		//show red/orange if gps accuracy is high
	    if(s->b.gpsAccuracy > 0.59) {
	       val_color = nvgRGBA(255, 188, 3, 200);
	    }
	    if(s->b.gpsAccuracy > 0.8) {
	       val_color = nvgRGBA(255, 0, 0, 200);
	    }


		// gps accuracy is always in meters
		if (true) {
			 snprintf(val_str, sizeof(val_str), "%d", (int)(s->b.gpsAccuracy*100.0));
		} else {
			 snprintf(val_str, sizeof(val_str), "%.1f", s->b.gpsAccuracy * 3.28084 * 12);
		}
		if (true) {
			snprintf(uom_str, sizeof(uom_str), "cm");;
		} else {
			snprintf(uom_str, sizeof(uom_str), "in");
		}
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "GPS PREC", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
	
	//add  steering angle
	if (true) {
		char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
			//show Orange if more than 6 degrees
			//show red if  more than 12 degrees
			if(((int)(s->b.angleSteers) < -6) || ((int)(s->b.angleSteers) > 6)) {
				val_color = nvgRGBA(255, 188, 3, 200);
			}
			if(((int)(s->b.angleSteers) < -12) || ((int)(s->b.angleSteers) > 12)) {
				val_color = nvgRGBA(255, 0, 0, 200);
			}
			// steering is in degrees
			snprintf(val_str, sizeof(val_str), "%.1f",(s->b.angleSteers));

	    snprintf(uom_str, sizeof(uom_str), "deg");
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "STEER", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
	
	//add  desired steering angle
	if (true) {
		char val_str[16];
		char uom_str[6];
		NVGcolor val_color = nvgRGBA(255, 255, 255, 200);
			//show Orange if more than 6 degrees
			//show red if  more than 12 degrees
			if(((int)(s->b.angleSteersDes) < -6) || ((int)(s->b.angleSteersDes) > 6)) {
				val_color = nvgRGBA(255, 188, 3, 200);
			}
			if(((int)(s->b.angleSteersDes) < -12) || ((int)(s->b.angleSteersDes) > 12)) {
				val_color = nvgRGBA(255, 0, 0, 200);
			}
			// steering is in degrees
			snprintf(val_str, sizeof(val_str), "%.1f",(s->b.angleSteersDes));

	    snprintf(uom_str, sizeof(uom_str), "deg");
		bb_h +=bb_ui_draw_measure(s,  val_str, uom_str, "DES STEER", 
				bb_rx, bb_ry, bb_uom_dx,
				val_color, lab_color, uom_color, 
				value_fontSize, label_fontSize, uom_fontSize );
		bb_ry = bb_y + bb_h;
	}
	
	
	//finally draw the frame
	bb_h += 20;
	nvgBeginPath(s->vg);
  	nvgRoundedRect(s->vg, bb_x, bb_y, bb_w, bb_h, 20);
  	nvgStrokeColor(s->vg, nvgRGBA(255,255,255,80));
  	nvgStrokeWidth(s->vg, 6);
  	nvgStroke(s->vg);
}

//draw grid from wiki
void ui_draw_vision_grid( UIState *s) {
  const UIScene *scene = &s->scene;
  bool is_cruise_set = false;//(s->scene.v_cruise != 0 && s->scene.v_cruise != 255);
  if (!is_cruise_set) {
    const int grid_spacing = 30;

    const Rect &viz_rect = s->scene.viz_rect;
    const int box_y = viz_rect.y;
    const int box_h  = viz_rect.h;
    int ui_viz_rx = scene->viz_rect.x;
    int ui_viz_rw = scene->viz_rect.w;

    nvgSave(s->vg);

    // path coords are worked out in rgb-box space
    nvgTranslate(s->vg, 240.0f, 0.0);

    // zooom in 2x
    nvgTranslate(s->vg, -1440.0f / 2, -1080.0f / 2);
    nvgScale(s->vg, 2.0, 2.0);

    nvgScale(s->vg, 1440.0f / s->stream.bufs_info.width, 1080.0f / s->stream.bufs_info.height);

    nvgBeginPath(s->vg);
    nvgStrokeColor(s->vg, nvgRGBA(255,255,255,128));
    nvgStrokeWidth(s->vg, 1);

    for (int i=box_y; i < box_h; i+=grid_spacing) {
      nvgMoveTo(s->vg, ui_viz_rx, i);
      //nvgLineTo(s->vg, ui_viz_rx, i);
      nvgLineTo(s->vg, ((ui_viz_rw + ui_viz_rx) / 2)+ 10, i);
    }

    for (int i=ui_viz_rx + 12; i <= ui_viz_rw; i+=grid_spacing) {
      nvgMoveTo(s->vg, i, 0);
      nvgLineTo(s->vg, i, 1000);
    }
    nvgStroke(s->vg);
    nvgRestore(s->vg);
  }
}

void bb_ui_draw_logo( UIState *s) {
  if ((s->status != STATUS_DISENGAGED)) { //&& (s->status != STATUS_STOPPED)) { //(s->status != STATUS_DISENGAGED) {//
    return;
  }
  if (!s->b.icShowLogo) {
    return;
  }
  int rduration = 8000;
  int logot = (bb_currentTimeInMilis() % rduration);
  int logoi = s->b.img_logo;
  if ((logot > (int)(rduration/4)) && (logot < (int)(3*rduration/4))) {
    logoi = s->b.img_logo2;
  }
  if (logot < (int)(rduration/2)) {
    logot = logot - (int)(rduration/4);
  } else {
    logot = logot - (int)(3*rduration/4);
  }
  float logop = fabs(4.0*logot/rduration);
  const UIScene *scene = &s->scene;
  const int ui_viz_rx = scene->viz_rect.x;
  const int ui_viz_rw = scene->viz_rect.w;
  const int viz_event_w = (int)(820 * logop);
  const int viz_event_h = 820;
  const int viz_event_x = (ui_viz_rx + (ui_viz_rw - viz_event_w - bdr_s*2)/2);
  const int viz_event_y = 200;
  // bool is_engageable = scene->controls_state.getEngageable();
  float viz_event_alpha = 1.0f;
  nvgBeginPath(s->vg);
  NVGpaint imgPaint = nvgImagePattern(s->vg, viz_event_x, viz_event_y,
  viz_event_w, viz_event_h, 0, logoi, viz_event_alpha);
  nvgRect(s->vg, viz_event_x, viz_event_y, (int)viz_event_w, viz_event_h);
  nvgFillPaint(s->vg, imgPaint);
  nvgFill(s->vg);
}

void bb_ui_draw_gyro(UIState *s) {
  //draw the gyro data
  const UIScene *scene = &s->scene;
  const float prc = 0.6;
  const int sc_h = 820;
  const int sc_w = scene->viz_rect.w;
  const int sc_x = bdr_s + scene->viz_rect.x;
  const int sc_y = 100;
  const int sc_cx = (int)(sc_x + sc_w /2);
  const int sc_cy =  (int)(sc_y + sc_h/2);
  const int l_w = (int)(sc_w * prc / 2);

  const int p_w = (int)((sc_cx - l_w - sc_x)*prc);
  const int p_x = (int)(bdr_s +(sc_cx - l_w - p_w)/2);
  const int p_max_h = (int)(sc_h * prc /2);

  nvgBeginPath(s->vg);
  //white for the base line
  nvgStrokeColor(s->vg, nvgRGBA(255,255,255,200));
  nvgStrokeWidth(s->vg, 4);
  nvgMoveTo(s->vg, sc_cx - l_w, sc_cy);
  nvgLineTo(s->vg, sc_cx +l_w, sc_cy);
  nvgStroke(s->vg);

  //compute angle vs horizontal axis based on roll
  const float r_ang_rad = -s->b.accRoll; 
  const float p_ang_rad = -s->b.accPitch; 

  //roll
  //acc roll - green
  nvgSave(s->vg);
  nvgTranslate(s->vg, sc_cx, sc_cy);
  nvgRotate(s->vg, r_ang_rad);
  nvgBeginPath(s->vg);
  //green for the real horizontal line based on ACC
  nvgStrokeColor(s->vg, nvgRGBA(28, 204,98,200));
  nvgStrokeWidth(s->vg, 4);
  nvgMoveTo(s->vg,  -l_w, 0);
  nvgLineTo(s->vg,  +l_w, 0);
  nvgStroke(s->vg);
  nvgRestore(s->vg);
  //mag roll - yellow
  nvgSave(s->vg);
  nvgTranslate(s->vg, sc_cx, sc_cy);
  nvgRotate(s->vg, -s->b.magRoll);
  nvgBeginPath(s->vg);
  //yellow for the real horizontal line based on MAG
  nvgStrokeColor(s->vg, nvgRGBA(255, 255,0,200));
  nvgStrokeWidth(s->vg, 4);
  nvgMoveTo(s->vg,  -l_w, 0);
  nvgLineTo(s->vg,  +l_w, 0);
  nvgStroke(s->vg);
  nvgRestore(s->vg);
  //gyro roll - red
  nvgSave(s->vg);
  nvgTranslate(s->vg, sc_cx, sc_cy);
  nvgRotate(s->vg, -s->b.gyroRoll);
  nvgBeginPath(s->vg);
  //red for the real horizontal line based on GYRO
  nvgStrokeColor(s->vg, nvgRGBA(255, 0,0,200));
  nvgStrokeWidth(s->vg, 4);
  nvgMoveTo(s->vg,  -l_w, 0);
  nvgLineTo(s->vg,  +l_w, 0);
  nvgStroke(s->vg);
  nvgRestore(s->vg);

  //pitch
  const int p_y = sc_cy;
  //double height to accentuate change
  const int p_h = (int)(p_max_h * sin(p_ang_rad) );
  nvgBeginPath(s->vg);
  //fill with red
  nvgFillColor(s->vg, nvgRGBA(255, 0, 0, 200));
  nvgStrokeColor(s->vg, nvgRGBA(255, 0, 0, 200)); 
  nvgRect(s->vg, p_x, p_y, p_w, p_h);
  nvgFill(s->vg);
  nvgStroke(s->vg);
  //draw middle line white
  nvgBeginPath(s->vg);
  nvgStrokeColor(s->vg, nvgRGBA(255,255,255,200));
  nvgStrokeWidth(s->vg, 4);
  nvgMoveTo(s->vg,  p_x,p_y);
  nvgLineTo(s->vg,  p_x + p_w, p_y);
  nvgStroke(s->vg);
  //draw horizon with green
  nvgBeginPath(s->vg);
  nvgStrokeColor(s->vg, nvgRGBA(28, 204,98,200));
  nvgMoveTo(s->vg,  p_x,p_y+p_h);
  nvgLineTo(s->vg,  p_x + p_w, p_y+p_h);
  nvgStroke(s->vg);
}

/// returns true on user input
bool bb_ui_read_triState_switch( UIState *s) {
  //get 3-state switch position
  // int tri_state_fd;
  // char buffer[10];
  if  (bb_currentTimeInMilis() - s->b.tri_state_switch_last_read > 2000)  {
    //tri_stated_fd = open ("/sys/devices/virtual/switch/tri-state-key/state", O_RDONLY);
    //if we can't open then switch should be considered in the middle, nothing done
    /* if (tri_state_fd == -1) {
      s->b.tri_state_switch = 3;
    } else {
      read (tri_state_fd, &buffer, 10);
      s->b.tri_state_switch = buffer[0] -48;
      close(tri_state_fd);
      s->b.tri_state_switch_last_read = bb_currentTimeInMilis();
    }*/
    s->b.tri_state_switch_last_read = bb_currentTimeInMilis(); 
    s->b.tri_state_switch = 3;
    if (strcmp(s->b.btns[2].btn_label2,"OP")==0) {
      s->b.tri_state_switch = 1;
    }
    if (strcmp(s->b.btns[2].btn_label2,"MIN")==0) {
      s->b.tri_state_switch = 2;
    }
    if (strcmp(s->b.btns[2].btn_label2,"OFF")==0) {
      s->b.tri_state_switch = 3;
    }
    if (strcmp(s->b.btns[2].btn_label2,"GYRO")==0) {
      s->b.tri_state_switch = 4;
    }
    int i = bb_get_button_status(s,(char *)"dsp");
    s->b.keepEonOff = (i==9);
    return true;
  }
  return false;
}

void bb_ui_draw_UI( UIState *s) {
  
  const UIScene *scene = &s->scene;
  const Rect &viz_rect = s->scene.viz_rect;
  const int box_y = viz_rect.y;

  if (s->b.tri_state_switch == 1) {

	  const int bb_dml_w = 180;
	  const int bb_dml_x =  (scene->viz_rect.x + (bdr_s*2));
	  const int bb_dml_y = (box_y + (bdr_s*1.5))+220;
	  
	  const int bb_dmr_w = 180;
	  const int bb_dmr_x = scene->viz_rect.x + scene->viz_rect.w - bb_dmr_w - (bdr_s*2) ; 
	  const int bb_dmr_y = (box_y + (bdr_s*1.5))+220;
    bb_ui_draw_measures_left(s,bb_dml_x, bb_dml_y, bb_dml_w );
    bb_ui_draw_measures_right(s,bb_dmr_x, bb_dmr_y, bb_dmr_w );
    bb_draw_buttons(s);
    bb_ui_draw_custom_alert(s);
    bb_ui_draw_logo(s);
	 }
   if (s->b.tri_state_switch ==2) {
	 	// const UIScene *scene = &s->scene;
	  // const int bb_dml_w = 180;
	  // const int bb_dml_x =  (scene->viz_rect.x + (bdr_s*2));
	  // const int bb_dml_y = (box_y + (bdr_s*1.5))+220;
	  
	  // const int bb_dmr_w = 180;
	  // const int bb_dmr_x = scene->viz_rect.x + scene->viz_rect.w - bb_dmr_w - (bdr_s*2) ; 
	  // const int bb_dmr_y = (box_y + (bdr_s*1.5))+220;
    bb_draw_buttons(s);
    bb_ui_draw_custom_alert(s);
    bb_ui_draw_logo(s);
	 }
	 if (s->b.tri_state_switch ==3) {
    //we now use the state 3 for minimalistic data alerts
	 	const UIScene *scene = &s->scene;
	  const int bb_dml_w = 180;
	  const int bb_dml_x =  (scene->viz_rect.x + (bdr_s*2));
	  const int bb_dml_y = (box_y + (bdr_s*1.5))+220;
	  
	  const int bb_dmr_w = 180;
	  const int bb_dmr_x = scene->viz_rect.x + scene->viz_rect.w - bb_dmr_w - (bdr_s*2) ; 
	  const int bb_dmr_y = (box_y + (bdr_s*1.5))+220;
    bb_ui_draw_measures_left2(s,bb_dml_x, bb_dml_y, bb_dml_w );
    bb_ui_draw_measures_right2(s,bb_dmr_x, bb_dmr_y, bb_dmr_w );
    bb_draw_buttons(s);
    bb_ui_draw_custom_alert(s);
	 }
   if (s->b.tri_state_switch ==4) {
    //we use the state 4 for gyro info
    // const UIScene *scene = &s->scene;
    // const int bb_dml_w = 180;
    // const int bb_dml_x =  (scene->viz_rect.x + (bdr_s*2));
    // const int bb_dml_y = (box_y + (bdr_s*1.5))+220;
    
    // const int bb_dmr_w = 180;
    // const int bb_dmr_x = scene->viz_rect.x + scene->viz_rect.w - bb_dmr_w - (bdr_s*2) ; 
    // const int bb_dmr_y = (box_y + (bdr_s*1.5))+220;
    bb_draw_buttons(s);
    bb_ui_draw_custom_alert(s);
    bb_ui_draw_gyro(s);
   }
}



void bb_ui_init(UIState *s) {

    //BB INIT
    s->b.shouldDrawFrame = true;
    s->status = STATUS_DISENGAGED;
    strcpy(s->b.car_model,"Tesla");
    strcpy(s->b.car_folder,"tesla");
    s->b.tri_state_switch = -1;
    s->b.tri_state_switch_last_read = 0;
    s->b.touch_last = false;
    s->b.touch_last_x = 0;
    s->b.touch_last_y =0;
    s->b.touch_last_width = s->scene.viz_rect.w;

    s->b.ctx = Context::create();

    //BB Define CAPNP sock
    s->b.uiButtonInfo_sock = SubSocket::create(s->b.ctx, "uiButtonInfo"); //zsock_new_sub(">tcp://127.0.0.1:8201", "");
    s->b.uiCustomAlert_sock = SubSocket::create(s->b.ctx, "uiCustomAlert"); //zsock_new_sub(">tcp://127.0.0.1:8202", "");
    s->b.uiSetCar_sock = SubSocket::create(s->b.ctx, "uiSetCar"); //zsock_new_sub(">tcp://127.0.0.1:8203", "");
    s->b.uiPlaySound_sock = SubSocket::create(s->b.ctx, "uiPlaySound"); //zsock_new_sub(">tcp://127.0.0.1:8205", "");
    s->b.uiButtonStatus_sock = PubSocket::create(s->b.ctx, "uiButtonStatus"); //zsock_new_pub("@tcp://127.0.0.1:8204");
    s->b.gps_sock = SubSocket::create(s->b.ctx, "gpsLocationExternal"); //zsock_new_sub(">tcp://127.0.0.1:8032","");
    s->b.uiGyroInfo_sock = SubSocket::create(s->b.ctx, "uiGyroInfo"); //zsock_new_sub(">tcp://127.0.0.1:8207", "");
    s->b.poller = Poller::create({
                              s->b.uiButtonInfo_sock,
                              s->b.uiCustomAlert_sock,
                              s->b.uiSetCar_sock,
                              s->b.uiPlaySound_sock,
                              s->b.gps_sock,
                              s->b.uiGyroInfo_sock
                             });

    //BB Load Images
    s->b.img_logo = nvgCreateImage(s->vg, "../assets/img_spinner_comma.png", 1);
    s->b.img_logo2 = nvgCreateImage(s->vg, "../assets/img_spinner_comma2.png", 1);
    s->b.img_car = nvgCreateImage(s->vg, "../assets/img_car_tesla.png", 1);

    s->b.icShowCar = true;
    s->b.icShowLogo = true;
}

// void bb_ui_play_sound( UIState *s, int sound) {
//    char* snd_command;
//    int bts = bb_get_button_status(s,"sound");
//    if ((bts > 0) || (bts == -1)) {
//        asprintf(&snd_command, "python /data/openpilot/selfdrive/car/modules/snd/playsound.py %d &", sound);
//        system(snd_command);
//    }
// }

void bb_ui_set_car( UIState *s, char *model, char *folder) {
    strcpy(s->b.car_model,model);
    strcpy(s->b.car_folder, folder);
}

bool bb_ui_poll_update(UIState* s) {
  //check tri-state switch
  bool user_input = bb_ui_read_triState_switch(s);

  while (true) {
    auto polls = s->b.poller->poll(0);
    if (polls.size() == 0)
      return user_input;

    for (auto sock : polls) {
      auto msg = sock->receive();
      if (!msg) {
        continue;
      }
      auto amsg = kj::heapArray<capnp::word>((msg->getSize() / sizeof(capnp::word)) + 1);
      memcpy(amsg.begin(), msg->getData(), msg->getSize());
      capnp::FlatArrayMessageReader cmsg(amsg);

      if (sock == s->b.uiButtonInfo_sock) {
        cereal::UIButtonInfo::Reader customButton = cmsg.getRoot<cereal::UIButtonInfo>();

        int id = customButton.getBtnId();
        // printf("*** got button info: ID = (%d)\n", id);
        // printf("*button name = '%s'**\n", (char *)customButton.getBtnName().cStr());
        strcpy(s->b.btns[id].btn_name, (char*)customButton.getBtnName().cStr());
        strcpy(s->b.btns[id].btn_label, (char*)customButton.getBtnLabel().cStr());
        strcpy(s->b.btns[id].btn_label2, (char*)customButton.getBtnLabel2().cStr());
        s->b.btns_status[id] = customButton.getBtnStatus();
      }
      else if (sock == s->b.uiCustomAlert_sock) {
        cereal::UICustomAlert::Reader data = cmsg.getRoot<cereal::UICustomAlert>();

        strcpy(s->b.custom_message, data.getCaText().cStr());
        s->b.custom_message_status = (UIStatus)data.getCaStatus();

        if ((strlen(s->b.custom_message) > 0) && (s->scene.alert_text1.length() == 0)) {
          if ((!((bb_get_button_status(s, (char*)"msg") == 0) && (s->b.custom_message_status <= 3))) && (s->vision_connected == true)) {
            user_input |= true;
          }
        }

        if ((s->scene.alert_text1.length() > 0) || (s->scene.alert_text2.length() > 0)) {
          user_input |= true;
        }
      }
      else if (sock == s->b.uiSetCar_sock) {
        //set car model socket
        cereal::UISetCar::Reader data = cmsg.getRoot<cereal::UISetCar>();

        if ((strcmp(s->b.car_model, (char*)data.getIcCarName().cStr()) != 0) || (strcmp(s->b.car_folder, (char*)data.getIcCarFolder().cStr()) != 0)) {
          strcpy(s->b.car_model, (char*)data.getIcCarName().cStr());
          strcpy(s->b.car_folder, (char*)data.getIcCarFolder().cStr());
          //LOGW("Car folder set (%s)", s->b.car_folder);

          if (strcmp(s->b.car_folder, "tesla") == 0) {
            s->b.img_logo = nvgCreateImage(s->vg, "../assets/img_spinner_comma.png", 1);
            s->b.img_logo2 = nvgCreateImage(s->vg, "../assets/img_spinner_comma2.png", 1);
            //LOGW("Spinning logo set for Tesla");
          }
          else if (strcmp(s->b.car_folder, "honda") == 0) {
            s->b.img_logo = nvgCreateImage(s->vg, "../assets/img_spinner_comma.honda.png", 1);
            s->b.img_logo2 = nvgCreateImage(s->vg, "../assets/img_spinner_comma.honda2.png", 1);
            //LOGW("Spinning logo set for Honda");
          }
          else if (strcmp(s->b.car_folder, "toyota") == 0) {
            s->b.img_logo = nvgCreateImage(s->vg, "../assets/img_spinner_comma.toyota.png", 1);
            s->b.img_logo2 = nvgCreateImage(s->vg, "../assets/img_spinner_comma.toyota2.png", 1);
            //LOGW("Spinning logo set for Toyota");
          };
        }
        s->b.icShowCar = (data.getIcShowCar() == 1);
        s->b.icShowLogo = (data.getIcShowLogo() == 1);
      }
      else if (sock == s->b.uiPlaySound_sock) {
        // play sound socket
        // cereal::UIPlaySound::Reader data = cmsg.getRoot<cereal::UIPlaySound>();

        // int snd = data.getSndSound();
        // bb_ui_play_sound(s,snd);
      }
      else if (sock == s->b.gps_sock) {
        // gps socket
        cereal::Event::Reader event = cmsg.getRoot<cereal::Event>();
        auto data = event.getGpsLocationExternal();

        s->b.gpsAccuracy = data.getAccuracy();
        if (s->b.gpsAccuracy > 100) {
          s->b.gpsAccuracy = 99.99;
        }
        else if (s->b.gpsAccuracy == 0) {
          s->b.gpsAccuracy = 99.8;
        }
      }
      else if (sock == s->b.uiGyroInfo_sock) {
        //gyro info socket
        cereal::UIGyroInfo::Reader data = cmsg.getRoot<cereal::UIGyroInfo>();

        s->b.accPitch = data.getAccPitch();
        s->b.accRoll = data.getAccRoll();
        s->b.accYaw = data.getAccYaw();
        s->b.magPitch = data.getMagPitch();
        s->b.magRoll = data.getMagRoll();
        s->b.magYaw = data.getMagYaw();
        s->b.gyroPitch = data.getGyroPitch();
        s->b.gyroRoll = data.getGyroRoll();
        s->b.gyroYaw = data.getGyroYaw();
      }
    }
  }

  return user_input;
}
