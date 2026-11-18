#include <WiFi.h>
#include <WebServer.h>

// ================ WIFI SETTINGS ================
const char* ssid = "SmartParking";
const char* password = "parking123";

WebServer server(80);

// ================ SENSOR WIRING ================
const uint8_t sensorPins[4] = {18, 19, 23, 25};
const char* slotNames[4] = {"A1", "A2", "B1", "B2"};

// true: LOW means object detected
// false: HIGH means object detected
const bool ACTIVE_LOW = true;

// Require a stable signal before changing slot status
const unsigned long DEBOUNCE_MS = 150;

bool occupied[4] = {false, false, false, false};
bool candidate[4] = {false, false, false, false};
unsigned long candidateSince[4] = {0, 0, 0, 0};

bool readOccupied(int i) {
  int raw = digitalRead(sensorPins[i]);
  return ACTIVE_LOW ? raw == LOW : raw == HIGH;
}

void updateSensors() {
  unsigned long now = millis();

  for (int i = 0; i < 4; i++) {
    bool reading = readOccupied(i);

    if (reading != candidate[i]) {
      candidate[i] = reading;
      candidateSince[i] = now;
    }

    if (occupied[i] != candidate[i] &&
        now - candidateSince[i] >= DEBOUNCE_MS) {
      occupied[i] = candidate[i];

      Serial.print(slotNames[i]);
      Serial.println(occupied[i] ? ": OCCUPIED" : ": AVAILABLE");
    }
  }
}

// ================ LIVE SENSOR DATA ================
void handleStatus() {
  int count = 0;
  for (int i = 0; i < 4; i++) {
    if (occupied[i]) count++;
  }

  String json = "{";
  json += "\"A1\":" + String(occupied[0] ? "true" : "false") + ",";
  json += "\"A2\":" + String(occupied[1] ? "true" : "false") + ",";
  json += "\"B1\":" + String(occupied[2] ? "true" : "false") + ",";
  json += "\"B2\":" + String(occupied[3] ? "true" : "false") + ",";
  json += "\"occupied\":" + String(count) + ",";
  json += "\"available\":" + String(4 - count);
  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// ================ WEBSITE ================
void handleHome() {
  String html = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#07090b">
<title>Parkwise | Smart Parking</title>

<style>
:root{
 --bg:#07090b;--panel:#101317;--border:#242a30;
 --muted:#8b949e;--white:#f4f7fa;
 --green:#b9ff55;--red:#ff5369;--blue:#75cfff;
}
*{box-sizing:border-box}
body{
 margin:0;background:var(--bg);color:var(--white);
 font-family:Arial,Helvetica,sans-serif;
}
main{width:min(1120px,100%);margin:auto;padding:25px}
header{
 display:flex;align-items:center;justify-content:space-between;
 gap:15px;padding:8px 0 23px;border-bottom:1px solid var(--border)
}
.logo{display:flex;align-items:center;gap:12px}
.logoIcon{
 display:grid;place-items:center;width:43px;height:43px;
 border-radius:13px;background:var(--green);color:#10140b;
 font-weight:900;font-size:22px
}
.logo h1{font-size:16px;letter-spacing:2px;margin:0}
.logo p{font-size:10px;letter-spacing:1.5px;color:var(--muted);margin:5px 0 0}
.pill{
 display:flex;align-items:center;gap:8px;border:1px solid var(--border);
 padding:10px 12px;border-radius:25px;font-size:11px;color:var(--muted)
}
.dot{height:7px;width:7px;border-radius:50%;background:var(--green);flex-shrink:0}
.hero{padding:35px 0 25px}
.eyebrow{font-size:10px;letter-spacing:3px;color:var(--green);font-weight:bold}
.hero h2{font-size:clamp(30px,5vw,46px);letter-spacing:-1.5px;line-height:1.1;margin:13px 0}
.hero p{font-size:13px;line-height:1.7;color:var(--muted);margin:0}
.metrics{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:14px}
.metric{
 background:linear-gradient(140deg,#14181d,#0d1013);
 border:1px solid var(--border);border-radius:16px;padding:22px
}
.label{font-size:10px;letter-spacing:1.5px;color:var(--muted)}
.value{font-size:36px;font-weight:800;margin-top:13px;letter-spacing:-1px}
.green{color:var(--green)}.red{color:var(--red)}.blue{color:var(--blue)}
.note{font-size:11px;color:var(--muted);margin-top:7px}
.sectionHead{
 display:flex;justify-content:space-between;align-items:center;
 gap:12px;margin:34px 0 15px
}
.sectionHead h3,.cameraHead h3{font-size:17px;margin:0}
.sectionHead span{font-size:10px;color:var(--muted);letter-spacing:1px}
.slots{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:13px}
.slot{
 padding:20px;background:#101317;border:1px solid var(--border);
 border-radius:15px;min-width:0;transition:background .2s,border-color .2s
}
.slot.free{background:#11190d;border-color:#3c5222}
.slot.busy{background:#1c1014;border-color:#642631}
.slotTop{display:flex;justify-content:space-between;align-items:center}
.slotName{font-size:21px;font-weight:800}
.symbol{
 width:29px;height:29px;display:grid;place-items:center;
 background:#24282d;border-radius:9px;color:var(--muted);font-weight:bold
}
.free .symbol{color:var(--green);background:#26361a}
.busy .symbol{color:var(--red);background:#391a21}
.slotStatus{
 display:flex;align-items:center;gap:8px;margin-top:24px;
 font-size:10px;letter-spacing:1px;font-weight:bold
}
.busy .dot{background:var(--red)}
.camera{
 margin-top:32px;padding:20px;border:1px solid var(--border);
 background:var(--panel);border-radius:18px
}
.cameraHead{
 display:flex;align-items:center;justify-content:space-between;
 gap:12px;margin-bottom:17px
}
.cameraHead p{font-size:11px;color:var(--muted);margin:6px 0 0}
.live{
 font-size:10px;letter-spacing:1px;color:#ff8291;
 background:#291116;border:1px solid #58222c;
 padding:9px 11px;border-radius:8px;white-space:nowrap
}
.frame{
 width:100%;aspect-ratio:16/9;background:#020303;
 border:1px solid #292d32;border-radius:12px;overflow:hidden;
 display:flex;align-items:center;justify-content:center
}
.frame img{width:100%;height:100%;display:block;object-fit:contain}
.cameraStatus{
 display:flex;justify-content:space-between;gap:10px;
 flex-wrap:wrap;color:var(--muted);font-size:11px;margin-top:13px
}
.footer{
 display:flex;justify-content:space-between;flex-wrap:wrap;gap:10px;
 border-top:1px solid var(--border);margin-top:28px;padding:20px 0 5px;
 font-size:10px;color:#737b84;letter-spacing:1px
}
@media(max-width:760px){
 main{padding:17px}
 .slots{grid-template-columns:repeat(2,minmax(0,1fr))}
 .frame{aspect-ratio:4/3}
 .camera{padding:13px}
 .slot{padding:17px}
}
@media(max-width:390px){
 main{padding:12px}
 .metric{padding:14px 9px}
 .label{font-size:9px;letter-spacing:.4px}
 .value{font-size:27px}
 .logo h1{font-size:13px}
 .logo p{font-size:8px;letter-spacing:.7px}
 .pill{font-size:9px;padding:8px}
 .cameraHead h3{font-size:15px}
}
</style>
</head>

<body>
<main>
<header>
 <div class="logo">
  <div class="logoIcon">P</div>
  <div>
   <h1>PARKWISE</h1>
   <p>SMART PARKING SYSTEM</p>
  </div>
 </div>
 <div class="pill">
  <span class="dot" id="connDot"></span>
  <span id="connection">CONNECTING</span>
 </div>
</header>

<section class="hero">
 <div class="eyebrow">PARKING CONTROL / LIVE MONITORING</div>
 <h2>Find your space.<br>Park smarter.</h2>
 <p>Live slot availability and mobile CCTV monitoring<br>
 for a smarter parking experience.</p>
</section>

<section class="metrics">
 <div class="metric">
  <div class="label">TOTAL SPACES</div>
  <div class="value blue">04</div>
  <div class="note">Installed parking slots</div>
 </div>
 <div class="metric">
  <div class="label">AVAILABLE</div>
  <div class="value green" id="available">--</div>
  <div class="note">Spaces ready for parking</div>
 </div>
 <div class="metric">
  <div class="label">OCCUPIED</div>
  <div class="value red" id="occupied">--</div>
  <div class="note">Spaces in use</div>
 </div>
</section>

<div class="sectionHead">
 <h3>Parking overview</h3>
 <span>4 SENSOR ZONES</span>
</div>

<section class="slots">
 <div class="slot" id="A1">
  <div class="slotTop"><span class="slotName">A1</span><span class="symbol">↗</span></div>
  <div class="slotStatus"><span class="dot"></span><span class="statusText">CHECKING</span></div>
 </div>
 <div class="slot" id="A2">
  <div class="slotTop"><span class="slotName">A2</span><span class="symbol">↗</span></div>
  <div class="slotStatus"><span class="dot"></span><span class="statusText">CHECKING</span></div>
 </div>
 <div class="slot" id="B1">
  <div class="slotTop"><span class="slotName">B1</span><span class="symbol">↗</span></div>
  <div class="slotStatus"><span class="dot"></span><span class="statusText">CHECKING</span></div>
 </div>
 <div class="slot" id="B2">
  <div class="slotTop"><span class="slotName">B2</span><span class="symbol">↗</span></div>
  <div class="slotStatus"><span class="dot"></span><span class="statusText">CHECKING</span></div>
 </div>
</section>

<section class="camera">
 <div class="cameraHead">
  <div>
   <h3>Live surveillance</h3>
   <p>Mobile phone camera feed</p>
  </div>
  <span class="live">● LIVE CCTV</span>
 </div>

 <div class="frame">
  <img id="cameraFeed"
       src="http://192.168.4.2:8080/video"
       alt="Parking camera feed">
 </div>

 <div class="cameraStatus">
  <span id="cameraStatus">Connecting to mobile camera...</span>
  <span>CAMERA STREAM</span>
 </div>
</section>

<footer class="footer">
 <span>PARKWISE SMART PARKING</span>
 <span>ESP32 • LIVE MONITORING</span>
</footer>
</main>

<script>
let updating = false;

function setSlot(id, busy) {
 const el = document.getElementById(id);
 el.className = "slot " + (busy ? "busy" : "free");
 el.querySelector(".statusText").textContent =
   busy ? "OCCUPIED" : "AVAILABLE";
 el.querySelector(".symbol").textContent = busy ? "×" : "↗";
}

async function updateStatus() {
 if (updating) return;
 updating = true;

 try {
  const response = await fetch("/status", {cache:"no-store"});
  if (!response.ok) throw new Error("HTTP error");
  const data = await response.json();

  document.getElementById("available").textContent =
    String(data.available).padStart(2,"0");
  document.getElementById("occupied").textContent =
    String(data.occupied).padStart(2,"0");

  setSlot("A1",data.A1);
  setSlot("A2",data.A2);
  setSlot("B1",data.B1);
  setSlot("B2",data.B2);

  document.getElementById("connection").textContent = "ONLINE";
  document.getElementById("connDot").style.background = "var(--green)";
 } catch(e) {
  document.getElementById("connection").textContent = "RECONNECTING";
  document.getElementById("connDot").style.background = "var(--red)";
 } finally {
  updating = false;
 }
}

const camera = document.getElementById("cameraFeed");

camera.onload = function() {
 document.getElementById("cameraStatus").textContent =
   "Camera image received";
};
camera.onerror = function() {
 document.getElementById("cameraStatus").textContent =
   "Camera unavailable — check stream address and network";
};

// Only update parking data. Do not reload the camera on each poll.
updateStatus();
setInterval(updateStatus, 2000);
</script>
</body>
</html>
)HTML";

  server.send(200, "text/html", html);
}

// ================ START ESP32 ================
void setup() {
  Serial.begin(115200);
  delay(300);

  for (int i = 0; i < 4; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  // Take initial readings so the dashboard starts from
  // the sensors' current states instead of assuming all free.
  for (int i = 0; i < 4; i++) {
    occupied[i] = readOccupied(i);
    candidate[i] = occupied[i];
    candidateSince[i] = millis();
  }

  WiFi.mode(WIFI_AP);

  if (WiFi.softAP(ssid, password)) {
    Serial.println();
    Serial.println("PARKWISE SMART PARKING STARTED");
    Serial.print("Wi-Fi: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
    Serial.print("Dashboard: http://");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start Wi-Fi access point");
  }

  server.on("/", handleHome);
  server.on("/status", handleStatus);
  server.begin();

  Serial.println("Web server started");
}

void loop() {
  updateSensors();
  server.handleClient();
}