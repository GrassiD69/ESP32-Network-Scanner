/**
 * ESP32-Network-Scanner – combined WiFi + BT scanner
 * for LilyGO T-Display-S3 AMOLED Plus 1.91" (536 x 240)
 *
 * Passive scanning only: reads what nearby devices already broadcast.
 * Use on your own network / devices or where you have permission.
 *
 * Flow:  Mode Select (WiFi / Bluetooth)  ->  Scan List  ->  Detail
 */
#include <WiFi.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
// #include <BLEDevice.h>   // uncomment when you implement the BT scan (API stays BLE*)

LilyGo_Class amoled;

// ===================================================================
// State
// ===================================================================
enum Screen { SCREEN_MODE_SELECT, SCREEN_SCAN_LIST, SCREEN_DETAIL };
enum Mode   { MODE_WIFI, MODE_BT };

// One record per scanned signal. WiFi fills some fields, BT others;
// leave the unused ones at a sentinel (e.g. rssi = 0, channel = -1).
struct Signal {
    String name;        // WiFi: SSID ("" if hidden) | BT: device name ("" if none)
    String address;     // WiFi: BSSID (MAC)         | BT: device address
    int    rssi;        // both: signal strength in dBm (negative, closer = higher)

    // --- WiFi only ---
    int    channel;     // 1..13
    int    encryption;  // wifi_auth_mode_t (WIFI_AUTH_OPEN, _WPA2_PSK, ...)

    // --- BT only ---
    int    txPower;     // advertised TX power at 1 m, if present (else e.g. 127)
    // TODO: add fields you want on the detail page, e.g.
    //       String serviceUuids;  int appearance;  bool isConnectable;
};

#define MAX_RESULTS 40
Signal  results[MAX_RESULTS];
int     resultCount   = 0;
int     selectedIndex = 0;

Screen  currentScreen = SCREEN_MODE_SELECT;
Mode    currentMode   = MODE_WIFI;

// ===================================================================
// Setup / loop
// ===================================================================
void setup()
{
    Serial.begin(115200);

    if (!amoled.begin()) {
        while (1) {
            Serial.println("Board not detected!");
            delay(1000);
        }
    }

    amoled.setRotation(1);  // landscape, must be set before beginLvglHelper
    beginLvglHelper(amoled);

    WiFi.mode(WIFI_STA);    // station mode (not connected) is required to scan
    WiFi.disconnect();

    // INPUT MODEL: touch = select (wired via LVGL event callbacks inside the
    // build* screens); one physical button = BACK. No long-press needed.
    pinMode(0, INPUT_PULLUP);
    pinMode(21, INPUT_PULLUP);
    // use GPIO21 as BACK (GPIO0 is also BOOT)
    // Optional sanity check: Serial.println(amoled.hasTouch()); to confirm
    // the touch panel was detected.

    goTo(SCREEN_MODE_SELECT);
}

void loop()
{
    lv_task_handler();  // lets LVGL draw, keep this here
    delay(5);

    // TODO (STEP 0 – input): selection happens through TOUCH, and LVGL calls
    // your widget callbacks automatically, so the only thing to poll here is
    // the BACK button:
    //   read GPIO21 with debounced edge detection (HIGH->LOW = a press);
    //   on a confirmed press -> goBack();
    //   goBack() already knows the hierarchy DETAIL->LIST->MODE_SELECT.
    // (Edge + ~30 ms debounce is still needed; long-press is not.)
}

// ===================================================================
// Navigation – wipe the screen and build the one we're going to.
// ===================================================================
void goTo(Screen s)
{
    currentScreen = s;
    lv_obj_clean(lv_scr_act());   // remove all widgets from the active screen

    switch (s) {
        case SCREEN_MODE_SELECT: buildModeSelect();           break;
        case SCREEN_SCAN_LIST:   buildScanList();             break;
        case SCREEN_DETAIL:      buildDetail(selectedIndex);  break;
    }
}

// Go one level up in the hierarchy:
//   DETAIL  -> SCAN_LIST  -> MODE_SELECT  -> (already at the top)
// Wire this to your "back" button press (see loop()). Each list/detail
// screen should also show a small "Back" hint so it's discoverable.
void goBack()
{
    switch (currentScreen) {
        case SCREEN_DETAIL:      goTo(SCREEN_SCAN_LIST);   break;
        case SCREEN_SCAN_LIST:   goTo(SCREEN_MODE_SELECT); break;
        case SCREEN_MODE_SELECT: /* top level, nothing above */ break;
    }
}

// ===================================================================
// Screen 1 – mode select
// ===================================================================
void onWifiChosen(lv_event_t *e)
{
    startScan(MODE_WIFI);
}

void onBtChosen(lv_event_t *e)
{
    startScan(MODE_BT);
}

void buildModeSelect()
{
    //Select Label
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "Select Mode");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    //WiFi mode Button
    lv_obj_t *wifiBtn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(wifiBtn, 260, 60);
    lv_obj_align(wifiBtn, LV_ALIGN_TOP_MID, 0, 70);

    // WiFi text label
    lv_obj_t *wifiLabel = lv_label_create(wifiBtn);
    lv_label_set_text(wifiLabel, "WiFi");
    lv_obj_set_style_text_font(wifiLabel, &lv_font_montserrat_28, 0);
    lv_obj_center(wifiLabel);

    //Bluetooth mode Button
    lv_obj_t *btBtn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btBtn, 260, 60);
    lv_obj_align(btBtn, LV_ALIGN_TOP_MID, 0, 145);

    // Bluetooth text label
    lv_obj_t *btLabel = lv_label_create(btBtn);
    lv_label_set_text(btLabel, "Bluetooth");
    lv_obj_set_style_text_font(btLabel, &lv_font_montserrat_28, 0);
    lv_obj_center(btLabel);

    // Add Button Events
    lv_obj_add_event_cb(wifiBtn, onWifiChosen, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btBtn,  onBtChosen,  LV_EVENT_CLICKED, NULL);
}

// Called when the user confirms a mode on the select screen.
void startScan(Mode mode)
{
    currentMode = mode;
    resultCount = 0;
    selectedIndex = 0;

    //Scanning Label
    lv_obj_t *scanning = lv_label_create(lv_scr_act());
    lv_label_set_text(scanning, "Scanning");
    lv_obj_set_style_text_font(scanning, &lv_font_montserrat_28, 0);
    lv_obj_align(scanning, LV_ALIGN_BOTTOM_MID, 0, 15);

    lv_timer_handler(); //force scanning label to be shown.
    if (mode == MODE_WIFI) scanWifi();
    else                   scanBt();

    goTo(SCREEN_SCAN_LIST);
}

// ===================================================================
// Scanning – fill results[] / resultCount
// ===================================================================
void scanWifi()
{
    int n = WiFi.scanNetworks();

    if (n < 0) n = 0;

    resultCount = (n < MAX_RESULTS) ? n : MAX_RESULTS;   // prevent looping more times than results size.

    for (int i = 0; i < resultCount; i++) {
        results[i].name       = WiFi.SSID(i);
        results[i].address    = WiFi.BSSIDstr(i);
        results[i].rssi       = WiFi.RSSI(i);
        results[i].channel    = WiFi.channel(i);
        results[i].encryption = WiFi.encryptionType(i);
    }

    WiFi.scanDelete();

    // print to serial
    Serial.printf("found %d networks\n", resultCount);
    for (int i = 0; i < resultCount; i++)
        Serial.printf("  %s  %d dBm  ch%d\n",
                      results[i].name.c_str(), results[i].rssi, results[i].channel);
}

void scanBt()
{
    // TODO:
    //   BLEDevice::init("");
    //   BLEScan *scan = BLEDevice::getScan();
    //   scan->setActiveScan(true);              // active = also asks for names
    //   BLEScanResults r = scan->start(5, false);
    //   for i in 0..r.getCount(): fill results[i] from r.getDevice(i)
    //     .getName() / .getAddress().toString() / .getRSSI()
    //     .haveTXPower() ? .getTXPower() : 127
    //   scan->clearResults();  resultCount = ...
    //
    // NOTE: WiFi and BT share one radio – only one scan runs at a time.
    // Here that's fine because we scan on demand, one mode at a time.
}

// ===================================================================
// Screen 2 – scan results list
// ===================================================================
void buildScanList()
{
    // TODO: make an lv_list, add one row per results[0..resultCount].
    //   Row label idea: name (or "<hidden>"/address) + RSSI, maybe a
    //   bars glyph. If resultCount == 0, show "No signals found".
    //   Give each row an LV_EVENT_CLICKED callback that sets selectedIndex
    //   to that row's index, then calls goTo(SCREEN_DETAIL). Pass the index
    //   through the widget (e.g. lv_obj_set_user_data / event user_data).
}

// ===================================================================
// Screen 3 – detail page for results[index]
// ===================================================================
void buildDetail(int index)
{
    Signal &s = results[index];
    (void)s;  // remove once you use it

    // TODO: lay out the fields. Suggested per mode:
    //   both : name, address, RSSI, estimated distance
    //   WiFi : channel, encryption (decode encryption int -> text)
    //   BT   : advertised TX power, service UUIDs, connectable, ...
    //
    // Approx distance from RSSI (log-distance path-loss model, very rough):
    //   distance_m = pow(10, (txPowerAt1m - rssi) / (10 * n))
    //     txPowerAt1m : reference RSSI at 1 m. BT: use s.txPower if present,
    //                   else ~ -59. WiFi: no standard value, pick a constant
    //                   like -40..-50 and label it "approximate".
    //     n           : path-loss exponent, ~2.0 open space .. 3-4 indoors.
    //   Be honest in the UI that this is a rough estimate, not a real range.
    //
    // Reality check on "protocol versions":
    //   WiFi scan gives channel + auth/encryption mode, NOT 802.11 a/b/g/n/ax.
    //   BT advertisements give TX power / service UUIDs / appearance, NOT the
    //   BT core version. Show what's actually in the scan rather than invent it.
}
