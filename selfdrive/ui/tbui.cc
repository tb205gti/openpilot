#include <stdio.h>
#include <string.h>
#include <math.h>
#include <map>
#include "ui.hpp"
#include "paint.hpp"

void ui_draw_infobar(UIState *s) {
  const UIScene *scene = &s->scene;
  int ui_viz_rx = scene->viz_rect.x;
  bool hasSidebar = !s->scene.uilayout_sidebarcollapsed;
  int rect_w = s->b.scr_w - ui_viz_rx - bdr_s;
  int rect_h = 60;
  int rect_x = 0;
  // rect_y = screen height - board - background height
  int rect_y = s->b.scr_h - bdr_s - (int) (rect_h/2) - 5;
  rect_x = rect_x + ui_viz_rx;

  int text_x = rect_w / 2;
  text_x = text_x + ui_viz_rx;
  int text_y = rect_y + 45;

  // Get local time to display
  char infobar[68];
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  char uom_str[5];
  snprintf(uom_str, sizeof(uom_str), s->is_metric ? "km/h" : "mph");
  char spd[6];
  float v_ego = s->scene.controls_state.getVEgo();
  snprintf(spd, sizeof(spd), "%.1f ", s->is_metric ? v_ego * 3.6 : v_ego * 2.2369363);

  char ang_steer[9];
  snprintf(ang_steer, sizeof(ang_steer), "%s%03.1f°", s->b.angleSteers < 0? "-" : "+", fabs(s->b.angleSteers));

  char lead_dist[8];
  if (s->scene.lead_data[0].getStatus()) {
    snprintf(lead_dist, sizeof(lead_dist), "%05.2fm", s->scene.lead_data[0].getDRel());
  } else {
    snprintf(lead_dist, sizeof(lead_dist), "%3s", "N/A");
  }

  char maxspeed_str[7];
  float v_cruise = s->scene.controls_state.getVCruise();
  if (s->scene.controls_state.getEnabled() && v_cruise > 0) {
    float maxspeed = s->is_metric ? v_cruise : v_cruise / 1.609;
    snprintf(maxspeed_str, sizeof(maxspeed_str), "/%.1f", maxspeed);
  }

  snprintf(
    infobar,
    sizeof(infobar),
    "%04d/%02d/%02d %02d:%02d:%02d | %s%s %s | DST: %s | ANG: %s",
    tm.tm_year + 1900,
    tm.tm_mon + 1,
    tm.tm_mday,
    tm.tm_hour,
    tm.tm_min,
    tm.tm_sec,
    spd,
    maxspeed_str,
    uom_str,
    lead_dist,
    ang_steer
  );

  nvgBeginPath(s->vg);
  nvgRoundedRect(s->vg, rect_x, rect_y, rect_w, rect_h, 5);
  nvgFillColor(s->vg, nvgRGBA(0, 0, 0, 180));
  nvgFill(s->vg);

  nvgFontSize(s->vg, hasSidebar? 40:50);
  nvgFontFace(s->vg, "sans-regular");
  nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 180));
  nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
  nvgText(s->vg, text_x, text_y, infobar, NULL);
}
