/**
 * ESP32-Network-Scanner – combined WiFi + BLE scanner
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
// #include <BLEDevice.h>   // uncomment when you implement the BLE scan

LilyGo_Class amoled;

// ===================================================================
// State
// ===================================================================
enum Screen { SCREEN_MODE_SELECT, SCREEN_SCAN_LIST, SCREEN_DETAIL };
enum Mode   { MODE_WIFI, MODE_BLE };

// One record per scanned signal. WiFi fills some fields, BLE others;
// leave the unused ones at a sentinel (e.g. rssi = 0, channel = -1).
struct Signal {
    String name;        // WiFi: SSID ("" if hidden) | BLE: device name ("" if none)
    String address;     // WiFi: BSSID (MAC)         | BLE: device address
    int    rssi;        // both: signal strength in dBm (negative, closer = higher)

    // --- WiFi only ---
    int    channel;     // 1..13
    int    encryption;  // wifi_auth_mode_t (WIFI_AUTH_OPEN, _WPA2_PSK, ...)

    // --- BLE only ---
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

    // TODO (STEP 0 – input): set up the two buttons (GPIO0 and GPIO21)
    //   pinMode(0, INPUT_PULLUP); pinMode(21, INPUT_PULLUP);
    //   Decide a scheme, e.g. btnA = move selection, btnB = confirm,
    //   hold btnB = back. Remember to debounce (track last state + millis).

    goTo(SCREEN_MODE_SELECT);
}

void loop()
{
    lv_task_handler();  // lets LVGL draw, keep this here
    delay(5);

    // TODO (STEP 0 – input): read buttons, then act on currentScreen:
    //   MODE_SELECT : move highlight between WiFi/BLE; confirm -> startScan()
    //   SCAN_LIST   : move selectedIndex; confirm -> goTo(SCREEN_DETAIL)
    //   DETAIL      : (no confirm action needed)
    //
    //   BACK (same button on every screen) -> goBack()
    //   goBack() already knows the hierarchy: DETAIL->LIST->MODE_SELECT.
    //   Suggested scheme with two buttons: btnA = move/next,
    //   btnB short press = confirm, btnB long press (hold) = back.
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
void buildModeSelect()
{
    // TODO: show a title and two choices (WiFi / Bluetooth).
    //   A simple lv_list with two lv_list_add_btn entries works, or two
    //   big lv_btn widgets side by side on the wide screen.
    //   Highlight the current choice so the buttons have something to move.
}

// Called when the user confirms a mode on the select screen.
void startScan(Mode mode)
{
    currentMode = mode;
    resultCount = 0;
    selectedIndex = 0;

    // TODO: show a quick "Scanning..." label so it isn't a frozen screen
    //       (scans block for a few seconds). Call lv_task_handler() once after.

    if (mode == MODE_WIFI) scanWifi();
    else                   scanBle();

    goTo(SCREEN_SCAN_LIST);
}

// ===================================================================
// Scanning – fill results[] / resultCount
// ===================================================================
void scanWifi()
{
    // TODO:
    //   int n = WiFi.scanNetworks();
    //   for i in 0..n (cap at MAX_RESULTS): fill results[i] with
    //     WiFi.SSID(i), WiFi.BSSIDstr(i), WiFi.RSSI(i),
    //     WiFi.channel(i), WiFi.encryptionType(i)
    //   WiFi.scanDelete();  resultCount = min(n, MAX_RESULTS);
}

void scanBle()
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
    // NOTE: WiFi and BLE share one radio – only one scan runs at a time.
    // Here that's fine because we scan on demand, one mode at a time.
}

// ===================================================================
// Screen 2 – scan results list
// ===================================================================
void buildScanList()
{
    // TODO: make an lv_list, add one row per results[0..resultCount].
    //   Row label idea: name (or "<hidden>"/address) + RSSI, maybe a
    //   bars glyph. Show the selected row highlighted.
    //   If resultCount == 0, show "No signals found".
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
    //   BLE  : advertised TX power, service UUIDs, connectable, ...
    //
    // Approx distance from RSSI (log-distance path-loss model, very rough):
    //   distance_m = pow(10, (txPowerAt1m - rssi) / (10 * n))
    //     txPowerAt1m : reference RSSI at 1 m. BLE: use s.txPower if present,
    //                   else ~ -59. WiFi: no standard value, pick a constant
    //                   like -40..-50 and label it "approximate".
    //     n           : path-loss exponent, ~2.0 open space .. 3-4 indoors.
    //   Be honest in the UI that this is a rough estimate, not a real range.
    //
    // Reality check on "protocol versions":
    //   WiFi scan gives channel + auth/encryption mode, NOT 802.11 a/b/g/n/ax.
    //   BLE advertisements give TX power / service UUIDs / appearance, NOT the
    //   BT core version. Show what's actually in the scan rather than invent it.
}
