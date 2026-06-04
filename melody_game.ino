#include <WiFi.h>
#include <WebServer.h>

#define BUZZER_PIN D5
#define LEDC_RES   8

const char* ssid     = "your_network";
const char* password = "your_pass";

WebServer server(80);

#define DO  1047
#define RE  1175
#define MI  1319
#define FA  1397
#define SOL 1568
#define LA  1760

void svirNota(int freq, int duration = 400) {
  ledcSetup(0, freq, LEDC_RES);
  ledcAttachPin(BUZZER_PIN, 0);
  ledcWrite(0, 128);
  delay(duration);
  ledcWrite(0, 0);
  ledcDetachPin(BUZZER_PIN);
  delay(80);
}

const char* PAGE = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>Melody Game</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: Arial; background: #1e1e2e; color: #cdd6f4; display: flex; flex-direction: column; align-items: center; padding: 20px; }
    h2 { color: #cba6f7; margin-bottom: 16px; }
    .stats { display: flex; gap: 12px; margin-bottom: 16px; }
    .stat { background: #313244; padding: 8px 20px; border-radius: 10px; font-size: 14px; }
    .stat span { font-weight: bold; color: #cba6f7; }
    #dots { display: flex; gap: 10px; justify-content: center; margin-bottom: 16px; min-height: 20px; }
    .dot { width: 14px; height: 14px; border-radius: 50%; border: 2px solid #585b70; background: #313244; display: inline-block; }
    .dot.done  { background: #1D9E75; border-color: #1D9E75; }
    .dot.wrong { background: #E24B4A; border-color: #E24B4A; }
    .dot.active{ background: #7F77DD; border-color: #7F77DD; }
    #status { font-size: 15px; color: #a6adc8; margin-bottom: 16px; min-height: 22px; }
    .grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; margin-bottom: 20px; width: 100%; max-width: 320px; }
    .note-btn {
      height: 80px; border-radius: 12px; border: 2px solid transparent;
      font-size: 18px; font-weight: bold; cursor: pointer; transition: transform 0.1s, opacity 0.15s;
    }
    .note-btn:active { transform: scale(0.92); }
    .note-btn.flash  { opacity: 0.25; }
    .note-btn:disabled { cursor: not-allowed; opacity: 0.35; }
    #btn-DO  { background:#0c2a4a; border-color:#378ADD; color:#85B7EB; }
    #btn-RE  { background:#162b08; border-color:#639922; color:#97C459; }
    #btn-MI  { background:#2e1a00; border-color:#BA7517; color:#EF9F27; }
    #btn-FA  { background:#1a1840; border-color:#7F77DD; color:#AFA9EC; }
    #btn-SOL { background:#2e1000; border-color:#D85A30; color:#F0997B; }
    #btn-LA  { background:#04251a; border-color:#1D9E75; color:#5DCAA5; }
    .btns { display: flex; gap: 12px; }
    .action-btn {
      padding: 10px 24px; border-radius: 10px; border: 1px solid #585b70;
      background: #313244; color: #cdd6f4; font-size: 14px; cursor: pointer;
    }
    .action-btn:hover { background: #45475a; }
    .action-btn:disabled { opacity: 0.4; cursor: not-allowed; }
  </style>
</head>
<body>
  <h2>Повтори мелодията</h2>
  <div class="stats">
    <div class="stat">Ниво: <span id="level">1</span></div>
    <div class="stat">Точки: <span id="score">0</span></div>
    <div class="stat">Живот: <span id="lives">3</span></div>
  </div>
  <div id="dots"></div>
  <div id="status">Натисни "Нова мелодия"</div>
  <div class="grid">
    <button class="note-btn" id="btn-DO"  onclick="pressNote('DO')">До</button>
    <button class="note-btn" id="btn-RE"  onclick="pressNote('RE')">Ре</button>
    <button class="note-btn" id="btn-MI"  onclick="pressNote('MI')">Ми</button>
    <button class="note-btn" id="btn-FA"  onclick="pressNote('FA')">Фа</button>
    <button class="note-btn" id="btn-SOL" onclick="pressNote('SOL')">Сол</button>
    <button class="note-btn" id="btn-LA"  onclick="pressNote('LA')">Ла</button>
  </div>
  <div class="btns">
    <button class="action-btn" onclick="newGame()">▶ Нова мелодия</button>
    <button class="action-btn" id="replayBtn" onclick="replay()" disabled>↺ Чуй пак</button>
  </div>

<script>
const NOTES = ['DO','RE','MI','FA','SOL','LA'];
let melody = [], input = [], level = 1, score = 0, lives = 3, busy = false;

function rand(n) { return Math.floor(Math.random() * n); }

function genMelody() {
  const len = level <= 2 ? 4 : level <= 4 ? 5 : 6;
  melody = Array.from({length: len}, () => NOTES[rand(NOTES.length)]);
}

function setStatus(t) { document.getElementById('status').textContent = t; }

function updateDots(idx, wrong) {
  const c = document.getElementById('dots');
  c.innerHTML = '';
  melody.forEach((_, i) => {
    const d = document.createElement('span');
    d.className = 'dot' + (wrong && i===idx ? ' wrong' : i<idx ? ' done' : i===idx ? ' active' : '');
    c.appendChild(d);
  });
}

function disableAll(d) {
  NOTES.forEach(n => document.getElementById('btn-'+n).disabled = d);
  document.getElementById('replayBtn').disabled = d;
}

function flashBtn(note) {
  const b = document.getElementById('btn-'+note);
  b.classList.add('flash');
  setTimeout(() => b.classList.remove('flash'), 300);
}

async function playNote(note) {
  flashBtn(note);
  await fetch('/play?note=' + note);
  await new Promise(r => setTimeout(r, 500));
}

async function playMelody() {
  busy = true;
  disableAll(true);
  setStatus('Слушай...');
  for (const n of melody) await playNote(n);
  input = [];
  updateDots(0, false);
  setStatus('Повтори мелодията!');
  disableAll(false);
  busy = false;
}

async function newGame() {
  score = 0; lives = 3; level = 1;
  document.getElementById('score').textContent = 0;
  document.getElementById('lives').textContent = 3;
  document.getElementById('level').textContent = 1;
  genMelody();
  await playMelody();
}

async function replay() {
  if (busy) return;
  await playMelody();
}

async function pressNote(note) {
  if (busy || input.length >= melody.length) return;
  fetch('/play?note=' + note);
  flashBtn(note);
  input.push(note);
  const idx = input.length - 1;

  if (input[idx] !== melody[idx]) {
    updateDots(idx, true);
    setStatus('Грешка!');
    lives--;
    document.getElementById('lives').textContent = lives;
    disableAll(true);
    await new Promise(r => setTimeout(r, 900));
    if (lives <= 0) {
      setStatus('Край! Резултат: ' + score);
      document.getElementById('dots').innerHTML = '';
      return;
    }
    input = [];
    await playMelody();
    return;
  }

  updateDots(idx + 1, false);

if (input.length === melody.length) {
    score += level * 10;
    lives = 3;  // <-- ресетване на животите
    document.getElementById('score').textContent = score;
    document.getElementById('lives').textContent = lives;
    setStatus('Браво! +' + (level * 10) + ' точки');
    disableAll(true);
    await new Promise(r => setTimeout(r, 1000));
    level++;
    document.getElementById('level').textContent = level;
    genMelody();
    await playMelody();
  }
}
</script>
</body>
</html>
)rawhtml";

void handleRoot() {
  server.send(200, "text/html", PAGE);
}

void handlePlay() {
  if (server.hasArg("note")) {
    String note = server.arg("note");
    int freq = 0;
    if (note == "DO")  freq = DO;
    else if (note == "RE")  freq = RE;
    else if (note == "MI")  freq = MI;
    else if (note == "FA")  freq = FA;
    else if (note == "SOL") freq = SOL;
    else if (note == "LA")  freq = LA;
    if (freq > 0) svirNota(freq, 400);
  }
  server.send(200, "text/plain", "ok");
}

void setup() {
  Serial0.begin(115200);
  delay(3000);
  Serial0.println("Стартиране...");

  WiFi.begin(ssid, password);
  Serial0.print("Свързване към WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial0.print(".");
  }
  Serial0.println();
  Serial0.print("IP адрес: ");
  Serial0.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/play", handlePlay);
  server.begin();
  Serial0.println("Сървърът стартира!");
}

void loop() {
  server.handleClient();
}