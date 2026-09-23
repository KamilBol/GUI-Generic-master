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

const char S_SERVO_TYPE_180[] PROGMEM = "Serwo 180&deg; (Kątowe)";
const char S_SERVO_TYPE_360[] PROGMEM = "Serwo 360&deg; (Ciągłe)";
const char* const SERVO_TYPE_P[] PROGMEM = {S_SERVO_TYPE_180, S_SERVO_TYPE_360};

const char S_SERVO_MODE_180_0[] PROGMEM = "Roleta / Zawór";
const char S_SERVO_MODE_180_1[] PROGMEM = "Ściemniacz";
const char S_SERVO_MODE_180_2[] PROGMEM = "Włącznik (ON/OFF)";
const char* const SERVO_MODE_180_P[] PROGMEM = {S_SERVO_MODE_180_0, S_SERVO_MODE_180_1, S_SERVO_MODE_180_2};

const char S_SERVO_MODE_360_0[] PROGMEM = "Silnik ON/OFF";
const char S_SERVO_MODE_360_1[] PROGMEM = "Roleta / Zawór";
const char* const SERVO_MODE_360_P[] PROGMEM = {S_SERVO_MODE_360_0, S_SERVO_MODE_360_1};

const char S_SERVO_DIR_0[] PROGMEM = "Lewo";
const char S_SERVO_DIR_1[] PROGMEM = "Prawo";
const char* const SERVO_DIR_P[] PROGMEM = {S_SERVO_DIR_0, S_SERVO_DIR_1};

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

  String s_type = "", s_ch_typ = "", s_inv = "", s_zero = "", s_mang = "";
  String s_spd = "", s_sst = "", s_offp = "", s_stpus = "", s_dir = "";
  String s_time = "", s_spddn = "", s_timedn = "", s_odrv = "";
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

    String v_zero = WebServer->httpServer->arg("s_zero" + String(i));
    if (v_zero == "") v_zero = "50";
    s_zero += v_zero + sep;

    String v_mang = WebServer->httpServer->arg("s_mang" + String(i));
    if (v_mang == "") v_mang = "45";
    s_mang += v_mang + sep;

    String v_spd = (v_type == "0") ? WebServer->httpServer->arg("s_spd180_" + String(i)) : WebServer->httpServer->arg("s_spd360_" + String(i));
    if (v_spd == "") v_spd = "100";
    s_spd += v_spd + sep;

    String v_sst = (v_type == "0") ? WebServer->httpServer->arg("s_sst180_" + String(i)) : WebServer->httpServer->arg("s_sst360_" + String(i));
    if (v_sst == "") v_sst = "0";
    s_sst += v_sst + sep;

    s_offp += String(WebServer->httpServer->hasArg("s_offp" + String(i)) ? "1" : "0") + sep;

    String v_stpus = WebServer->httpServer->arg("s_stpus" + String(i));
    if (v_stpus == "") v_stpus = "1500";
    s_stpus += v_stpus + sep;

    String v_dir = WebServer->httpServer->arg("s_dir" + String(i));
    if (v_dir == "") v_dir = "0";
    s_dir += v_dir + sep;

    String v_time = WebServer->httpServer->arg("s_time" + String(i));
    if (v_time == "") v_time = "0";
    s_time += v_time + sep;

    String v_spddn = WebServer->httpServer->arg("s_spddn" + String(i));
    if (v_spddn == "") v_spddn = "100";
    s_spddn += v_spddn + sep;

    String v_timedn = WebServer->httpServer->arg("s_timedn" + String(i));
    if (v_timedn == "") v_timedn = "0";
    s_timedn += v_timedn + sep;

    String v_odrv = WebServer->httpServer->arg("s_odrv" + String(i));
    if (v_odrv == "") v_odrv = "0";
    s_odrv += v_odrv + sep;

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
      ConfigManager->set(KEY_SERVO_ZERO, s_zero.c_str());
      ConfigManager->set(KEY_SERVO_MAX_ANGLE, s_mang.c_str());
      ConfigManager->set(KEY_SERVO_SPEED, s_spd.c_str());
      ConfigManager->set(KEY_SERVO_SOFT_START, s_sst.c_str());
      ConfigManager->set(KEY_SERVO_OFF_PWM, s_offp.c_str());
      ConfigManager->set(KEY_SERVO_STOP_US, s_stpus.c_str());
      ConfigManager->set(KEY_SERVO_DIR, s_dir.c_str());
      ConfigManager->set(KEY_SERVO_TIME, s_time.c_str());
      ConfigManager->set(KEY_SERVO_SPEED_DOWN, s_spddn.c_str());
      ConfigManager->set(KEY_SERVO_TIME_DOWN, s_timedn.c_str());
      ConfigManager->set(KEY_SERVO_OVERDRIVE, s_odrv.c_str());
      ConfigManager->set(KEY_SERVO_LIMIT_UP_STATE, s_l_up_s.c_str());
      ConfigManager->set(KEY_SERVO_LIMIT_DOWN_STATE, s_l_dn_s.c_str());
  }

  switch (ConfigManager->save()) {
    case E_CONFIG_OK:
      handlePageServo(1);
      break;
    default:
      handlePageServo(2);
      break;
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
    "    toggleField('s_inv'+i, is180);\n"
    "    toggleField('s_zero'+i, is180);\n"
    "    toggleField('s_mang'+i, is180);\n"
    "    toggleField('s_spd180_'+i, is180);\n"
    "    toggleField('s_sst180_'+i, is180);\n"
    "    toggleField('s_offp'+i, true); /* TERAZ ZAWSZE WIDOCZNE DLA OBU TYPÓW */ \n"
    "    \n"
    "    toggleField('s_stpus'+i, is360);\n"
    "    toggleField('s_ch360_'+i, is360);\n"
    "    toggleField('s_dir'+i, is360);\n"
    "    toggleField('s_spd360_'+i, is360);\n"
    "    toggleField('s_time'+i, is360);\n"
    "    toggleField('s_sst360_'+i, is360);\n"
    "    \n"
    "    toggleField('s_spddn'+i, isRol);\n"
    "    toggleField('s_timedn'+i, isRol);\n"
    "    toggleField('s_odrv'+i, isRol);\n"
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
  int m_srv = ConfigManager->get(KEY_MAX_SERVO)->getValueInt();
  
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
    
    int v_type = ConfigManager->get(KEY_SERVO_TYPE)->getElement(i).toInt();
    int v_ch = ConfigManager->get(KEY_SERVO_CH_TYPE)->getElement(i).toInt();
    int v_inv = ConfigManager->get(KEY_SERVO_INVERTED)->getElement(i).toInt();
    int v_offp = ConfigManager->get(KEY_SERVO_OFF_PWM)->getElement(i).toInt();
    int v_dir = ConfigManager->get(KEY_SERVO_DIR)->getElement(i).toInt();
    int v_lus = ConfigManager->get(KEY_SERVO_LIMIT_UP_STATE)->getElement(i).toInt();
    int v_lds = ConfigManager->get(KEY_SERVO_LIMIT_DOWN_STATE)->getElement(i).toInt();
    
    String v_zero = ConfigManager->get(KEY_SERVO_ZERO)->getElement(i);
    if (v_zero == "") v_zero = "50";
    String v_mang = ConfigManager->get(KEY_SERVO_MAX_ANGLE)->getElement(i);
    if (v_mang == "") v_mang = "45";
    String v_spd = ConfigManager->get(KEY_SERVO_SPEED)->getElement(i);
    if (v_spd == "") v_spd = "100";
    String v_sst = ConfigManager->get(KEY_SERVO_SOFT_START)->getElement(i);
    if (v_sst == "") v_sst = "0";
    String v_stpus = ConfigManager->get(KEY_SERVO_STOP_US)->getElement(i);
    if (v_stpus == "") v_stpus = "1500";
    String v_time = ConfigManager->get(KEY_SERVO_TIME)->getElement(i);
    if (v_time == "") v_time = "0";
    String v_spddn = ConfigManager->get(KEY_SERVO_SPEED_DOWN)->getElement(i);
    if (v_spddn == "") v_spddn = "100";
    String v_timedn = ConfigManager->get(KEY_SERVO_TIME_DOWN)->getElement(i);
    if (v_timedn == "") v_timedn = "0";
    String v_odrv = ConfigManager->get(KEY_SERVO_OVERDRIVE)->getElement(i);
    if (v_odrv == "") v_odrv = "0";

    addListGPIOBox("s_gpio" + String(i), F("Pin PWM"), FUNCTION_SERVO, i, true);
    addListBox("s_type" + String(i), F("Typ Serwa"), SERVO_TYPE_P, 2, v_type, 0, true);

    addListBox("s_ch180_" + String(i), F("Widok w aplikacji"), SERVO_MODE_180_P, 3, v_type == 0 ? v_ch : 0, 0, true);
    addCheckBox("s_inv" + String(i), F("Odwróć kierunek"), v_inv);
    addNumberBox("s_zero" + String(i), F("Punkt ZERO [%]"), F("50"), true, v_zero);
    addNumberBox("s_mang" + String(i), F("Max wychył od zera [&deg;]"), F("45"), true, v_mang);
    addNumberBox("s_spd180_" + String(i), F("Prędkość ruchu [%]"), F("100"), true, v_spd);
    addNumberBox("s_sst180_" + String(i), F("Miękki Start/Stop [%]"), F("0"), true, v_sst);
    
    addNumberBox("s_stpus" + String(i), F("Punkt STOP [us]"), F("1500"), true, v_stpus);
    addListBox("s_ch360_" + String(i), F("Tryb pracy"), SERVO_MODE_360_P, 2, v_type == 1 ? v_ch : 0, 0, true);
    addListBox("s_dir" + String(i), F("Kierunek"), SERVO_DIR_P, 2, v_dir, 0, true);
    addNumberBox("s_spd360_" + String(i), F("Prędkość ruchu / otw. [%]"), F("100"), true, v_spd);
    addNumberBox("s_time" + String(i), F("Czas pracy / otw. [s]"), F("0"), true, v_time);
    addNumberBox("s_sst360_" + String(i), F("Miękki Start/Stop [s]"), F("0"), true, v_sst);
    addNumberBox("s_spddn" + String(i), F("Prędkość zamykania [%]"), F("100"), true, v_spddn);
    addNumberBox("s_timedn" + String(i), F("Czas zamknięcia [s]"), F("0"), true, v_timedn);
    addNumberBox("s_odrv" + String(i), F("Dociąganie krańcowe [s]"), F("0"), true, v_odrv);
    
    addCheckBox("s_offp" + String(i), F("Odcinanie PWM po ruchu"), v_offp); // TERAZ WIDOCZNE DLA WSZYSTKICH!
    
    addListGPIOBox("s_l_up" + String(i), F("Krańcówka Otwarcia"), FUNCTION_LIMIT_SWITCH, i, true);
    addListBox("s_l_up_s" + String(i), F("Stan aktywny Otwarcia"), SERVO_STATE_P, 2, v_lus, 0, true);
    addListGPIOBox("s_l_dn" + String(i), F("Krańcówka Zamknięcia"), FUNCTION_LIMIT_SWITCH, i+5, true); 
    addListBox("s_l_dn_s" + String(i), F("Stan aktywny Zamknięcia"), SERVO_STATE_P, 2, v_lds, 0, true);
    
    addFormHeaderEnd();
  }

  addButtonSubmit(S_SAVE);
  addFormEnd();
  addButton(S_RETURN, PATH_DEVICE_SETTINGS);
  WebServer->sendHeaderEnd();
}

#endif // SUPLA_SERVO_CUSTOM