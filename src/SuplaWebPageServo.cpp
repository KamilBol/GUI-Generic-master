#include "SuplaWebPageServo.h"
#include "SuplaWebServer.h"
#include "SuplaConfigManager.h"
#include "Markup.h"
#include "GUIGenericCommon.h"

#if defined(SUPLA_SERVO_CUSTOM)

const char S_0[] PROGMEM = "0";
const char S_1[] PROGMEM = "1";
const char S_2[] PROGMEM = "2";
const char S_3[] PROGMEM = "3";
const char S_4[] PROGMEM = "4";
const char S_5[] PROGMEM = "5";
const char* const MAX_SERVO_P[] PROGMEM = {S_0, S_1, S_2, S_3, S_4, S_5};

const char S_SERVO_TYPE_180[] PROGMEM = "Serwo 180&deg; / Kątowe";
const char S_SERVO_TYPE_360[] PROGMEM = "Serwo 360&deg; / Ciągłe";
const char* const SERVO_TYPE_P[] PROGMEM = {S_SERVO_TYPE_180, S_SERVO_TYPE_360};

const char S_SERVO_MODE_180_0[] PROGMEM = "Roleta / Zawór";
const char S_SERVO_MODE_180_1[] PROGMEM = "Ściemniacz";
const char S_SERVO_MODE_180_2[] PROGMEM = "Włącznik (ON/OFF)";
const char* const SERVO_MODE_180_P[] PROGMEM = {S_SERVO_MODE_180_0, S_SERVO_MODE_180_1, S_SERVO_MODE_180_2};

const char S_SERVO_MODE_360_0[] PROGMEM = "Silnik ON/OFF";
const char S_SERVO_MODE_360_1[] PROGMEM = "Roleta / Zawór";
const char* const SERVO_MODE_360_P[] PROGMEM = {S_SERVO_MODE_360_0, S_SERVO_MODE_360_1};

const char S_SERVO_STATE_0[] PROGMEM = "LOW";
const char S_SERVO_STATE_1[] PROGMEM = "HIGH";
const char* const SERVO_STATE_P[] PROGMEM = {S_SERVO_STATE_0, S_SERVO_STATE_1};

void createWebPageServo() {
  WebServer->httpServer->on(getURL(PATH_SERVO), [&]() {
    if (!WebServer->isLoggedIn()) return;
    if (WebServer->httpServer->method() == HTTP_GET) handlePageServo();
    else handlePageServoSave();
  });
}

void handlePageServoSave() {
  if (!WebServer->isLoggedIn()) return;

  int max_servo = WebServer->httpServer->arg(F("max_servo")).toInt();
  if (max_servo < 0) max_servo = 0;
  if (max_servo > 5) max_servo = 5;
  ConfigManager->set(KEY_MAX_SERVO, String(max_servo).c_str());

  String s_type = "", s_ch_typ = "", s_inv = "";
  String s_min_us = "", s_mid_us = "", s_max_us = "";
  String s_trans_ms = "", s_detach_ms = "", s_deadb = "";
  String s_l_up_s = "", s_l_dn_s = "";

  for (int i = 0; i < max_servo; i++) {
    String sep = (i < max_servo - 1 ? "," : "");

    WebServer->saveGPIO("s_gpio" + String(i), FUNCTION_SERVO, i);
    WebServer->saveGPIO("s_l_up" + String(i), FUNCTION_LIMIT_SWITCH, i);
    WebServer->saveGPIO("s_l_dn" + String(i), FUNCTION_LIMIT_SWITCH, i + 5);

    String v_type = WebServer->httpServer->arg("s_type" + String(i));
    if (v_type == "") v_type = "0";
    s_type += v_type + sep;

    String v_ch = (v_type == "0") ? WebServer->httpServer->arg("s_ch180_" + String(i)) : WebServer->httpServer->arg("s_ch360_" + String(i));
    if (v_ch == "") v_ch = "0";
    s_ch_typ += v_ch + sep;

    s_inv += String(WebServer->httpServer->hasArg("s_inv" + String(i)) ? "1" : "0") + sep;

    String v_min = WebServer->httpServer->arg("s_min_us" + String(i));
    if (v_min == "") v_min = "500";
    s_min_us += v_min + sep;

    String v_mid = WebServer->httpServer->arg("s_mid_us" + String(i));
    if (v_mid == "") v_mid = "1500";
    s_mid_us += v_mid + sep;

    String v_max = WebServer->httpServer->arg("s_max_us" + String(i));
    if (v_max == "") v_max = "2500";
    s_max_us += v_max + sep;

    String v_trans = WebServer->httpServer->arg("s_trans_ms" + String(i));
    if (v_trans == "") v_trans = "500";
    s_trans_ms += v_trans + sep;

    String v_detach = WebServer->httpServer->arg("s_detach_ms" + String(i));
    if (v_detach == "") v_detach = "0";
    s_detach_ms += v_detach + sep;

    String v_deadb = WebServer->httpServer->arg("s_deadb" + String(i));
    if (v_deadb == "") v_deadb = "3";
    s_deadb += v_deadb + sep;

    String v_lups = WebServer->httpServer->arg("s_l_up_s" + String(i));
    if (v_lups == "") v_lups = "0";
    s_l_up_s += v_lups + sep;

    String v_ldns = WebServer->httpServer->arg("s_l_dn_s" + String(i));
    if (v_ldns == "") v_ldns = "0";
    s_l_dn_s += v_ldns + sep;
  }

  if (max_servo > 0) {
      ConfigManager->set(KEY_SERVO_TYPE, s_type.c_str());
      ConfigManager->set(KEY_SERVO_CH_TYPE, s_ch_typ.c_str());
      ConfigManager->set(KEY_SERVO_INVERTED, s_inv.c_str());
      
      // Zapis do bezpiecznych kluczy
      ConfigManager->set(KEY_SERVO_ZERO, s_min_us.c_str());
      ConfigManager->set(KEY_SERVO_STOP_US, s_mid_us.c_str()); 
      ConfigManager->set(KEY_SERVO_MAX_ANGLE, s_max_us.c_str());
      ConfigManager->set(KEY_SERVO_SPEED, s_trans_ms.c_str()); 
      ConfigManager->set(KEY_SERVO_TIME, s_detach_ms.c_str());
      ConfigManager->set(KEY_SERVO_SOFT_START, s_deadb.c_str());
      
      ConfigManager->set(KEY_SERVO_LIMIT_UP_STATE, s_l_up_s.c_str());
      ConfigManager->set(KEY_SERVO_LIMIT_DOWN_STATE, s_l_dn_s.c_str());
  }

  switch (ConfigManager->save()) {
    case E_CONFIG_OK: handlePageServo(1); break;
    default: handlePageServo(2); break;
  }
}

void handlePageServo(int save) {
  if (!WebServer->isLoggedIn()) return;

  WebServer->sendHeaderStart();
  SuplaSaveResult(save);
  SuplaJavaScript(PATH_SERVO);

  WebServer->sendContent(F("<script>\n"
    "function toggleField(name, show) {\n"
    "  var el = document.getElementsByName(name)[0];\n"
    "  if(el && el.parentElement) el.parentElement.style.display = show ? 'block' : 'none';\n"
    "}\n"
    "function updateUI() {\n"
    "  for(var i=0; i<5; i++) {\n"
    "    var t = document.getElementsByName('s_type'+i)[0];\n"
    "    if(!t) continue;\n"
    "    var typ = t.value;\n"
    "    var ch360 = document.getElementsByName('s_ch360_'+i)[0];\n"
    "    var is180 = (typ == '0');\n"
    "    var is360 = (typ == '1');\n"
    "    var isRol = is360 && ch360 && (ch360.value == '1');\n"
    "    \n"
    "    toggleField('s_ch180_'+i, is180);\n"
    "    toggleField('s_inv'+i, true);\n"
    "    toggleField('s_min_us'+i, true);\n"
    "    toggleField('s_mid_us'+i, true);\n"
    "    toggleField('s_max_us'+i, true);\n"
    "    toggleField('s_trans_ms'+i, true);\n"
    "    toggleField('s_detach_ms'+i, true);\n"
    "    toggleField('s_ch360_'+i, is360);\n"
    "    toggleField('s_deadb'+i, is360);\n"
    "    toggleField('s_l_up'+i, isRol);\n"
    "    toggleField('s_l_up_s'+i, isRol);\n"
    "    toggleField('s_l_dn'+i, isRol);\n"
    "    toggleField('s_l_dn_s'+i, isRol);\n"
    "  }\n"
    "}\n"
    "window.addEventListener('load', function() {\n"
    "  updateUI();\n"
    "  var sel = document.querySelectorAll('select');\n"
    "  sel.forEach(function(s){ s.addEventListener('change', updateUI); });\n"
    "});\n"
    "</script>\n"));

  addForm(F("post"), PATH_SERVO);
  addFormHeader(F("Konfiguracja Serwomechanizmów"));
  
  auto elMaxSrv = ConfigManager->get(KEY_MAX_SERVO);
  int m_srv = elMaxSrv ? elMaxSrv->getValueInt() : 0;
  
  WebServer->sendContent(F("<i style='display:flex; justify-content:space-between; align-items:center;'>"));
  WebServer->sendContent(F("<label style='position:relative; flex-grow:1;'>Liczba urządzeń (0-5)</label>"));
  WebServer->sendContent(F("<select name='max_servo' class='w3-select w3-border' style='position:relative; width:80px; right:auto; top:auto;' onchange='this.form.submit()'>"));
  for (int j = 0; j <= 5; j++) {
    WebServer->sendContent(F("<option value='"));
    WebServer->sendContent(String(j));
    WebServer->sendContent(F("'"));
    if (m_srv == j) WebServer->sendContent(F(" selected"));
    WebServer->sendContent(F(">"));
    WebServer->sendContent(String(j));
    WebServer->sendContent(F("</option>"));
  }
  WebServer->sendContent(F("</select></i>"));
  addFormHeaderEnd();

  if (m_srv > 5) m_srv = 5;

  for (int i = 0; i < m_srv; i++) {
    addFormHeader(String(F("Ustawienia serwomechanizmu nr. ")) + String(i + 1));
    
    auto elType = ConfigManager->get(KEY_SERVO_TYPE);
    int v_type = elType ? elType->getElement(i).toInt() : 0;
    
    auto elChType = ConfigManager->get(KEY_SERVO_CH_TYPE);
    int v_ch = elChType ? elChType->getElement(i).toInt() : 0;
    
    auto elInv = ConfigManager->get(KEY_SERVO_INVERTED);
    int v_inv = elInv ? elInv->getElement(i).toInt() : 0;
    
    auto elMin = ConfigManager->get(KEY_SERVO_ZERO);
    String v_min = elMin ? elMin->getElement(i) : "500";
    if (v_min == "") v_min = "500";
    
    auto elMid = ConfigManager->get(KEY_SERVO_STOP_US);
    String v_mid = elMid ? elMid->getElement(i) : "1500";
    if (v_mid == "") v_mid = "1500";
    
    auto elMax = ConfigManager->get(KEY_SERVO_MAX_ANGLE);
    String v_max = elMax ? elMax->getElement(i) : "2500";
    if (v_max == "") v_max = "2500";
    
    auto elTrans = ConfigManager->get(KEY_SERVO_SPEED);
    String v_trans = elTrans ? elTrans->getElement(i) : "500";
    if (v_trans == "") v_trans = "500";
    
    auto elDet = ConfigManager->get(KEY_SERVO_TIME);
    String v_detach = elDet ? elDet->getElement(i) : "0";
    if (v_detach == "") v_detach = "0";

    auto elDb = ConfigManager->get(KEY_SERVO_SOFT_START);
    String v_deadb = elDb ? elDb->getElement(i) : "3";
    if (v_deadb == "") v_deadb = "3";

    auto elLUp = ConfigManager->get(KEY_SERVO_LIMIT_UP_STATE);
    int v_lus = elLUp ? elLUp->getElement(i).toInt() : 0;
    
    auto elLDn = ConfigManager->get(KEY_SERVO_LIMIT_DOWN_STATE);
    int v_lds = elLDn ? elLDn->getElement(i).toInt() : 0;

    // Standardowe klocki GUI-Generic. Krotkie nazwy, zeby wartości miały miejsce!
    addListGPIOBox("s_gpio" + String(i), F("GPIO PWM"), FUNCTION_SERVO, i, true);
    addListBox("s_type" + String(i), F("Rodzaj"), SERVO_TYPE_P, 2, v_type, 0, true);
    addListBox("s_ch180_" + String(i), F("Tryb"), SERVO_MODE_180_P, 3, v_type == 0 ? v_ch : 0, 0, true);
    addListBox("s_ch360_" + String(i), F("Tryb"), SERVO_MODE_360_P, 2, v_type == 1 ? v_ch : 0, 0, true);
    
    addNumberBox("s_min_us" + String(i), F("Impuls MIN [us]"), F("500"), true, v_min);
    addNumberBox("s_mid_us" + String(i), F("Środek/STOP [us]"), F("1500"), true, v_mid);
    addNumberBox("s_max_us" + String(i), F("Impuls MAX [us]"), F("2500"), true, v_max);
    
    addCheckBox("s_inv" + String(i), F("Odwróć kierunek"), v_inv);
    
    addNumberBox("s_trans_ms" + String(i), F("Czas ruchu [ms]"), F("500"), true, v_trans);
    addNumberBox("s_detach_ms" + String(i), F("Odłącz po [ms] (0=nie)"), F("0"), true, v_detach);
    addNumberBox("s_deadb" + String(i), F("Martwa strefa 360 [%]"), F("3"), true, v_deadb);

    addListGPIOBox("s_l_up" + String(i), F("GPIO Kranc. UP"), FUNCTION_LIMIT_SWITCH, i, true);
    addListBox("s_l_up_s" + String(i), F("Aktywny UP"), SERVO_STATE_P, 2, v_lus, 0, true);
    addListGPIOBox("s_l_dn" + String(i), F("GPIO Kranc. DOWN"), FUNCTION_LIMIT_SWITCH, i+5, true); 
    addListBox("s_l_dn_s" + String(i), F("Aktywny DOWN"), SERVO_STATE_P, 2, v_lds, 0, true);
    
    addFormHeaderEnd();
  }

  addButtonSubmit(S_SAVE);
  addFormEnd();
  addButton(S_RETURN, PATH_DEVICE_SETTINGS);
  WebServer->sendHeaderEnd();
}
#endif // SUPLA_SERVO_CUSTOM