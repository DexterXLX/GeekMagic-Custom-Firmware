// Web UI, embedded in the firmware so a single .bin update is enough.
#pragma once
#include <pgmspace.h>
static const char WEB_UI[] PROGMEM = R"HTMLUI(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>SmallTV</title>
<style>
:root{--bg:#EDF0F5;--panel:#fff;--line:#D5DBE6;--text:#172030;--dim:#5E6B82;--accent:#2DD4BF;--on-accent:#0B1220;--screen:#0E121A;--screen-text:#EEF2F8;--screen-dim:#8C98AC;color-scheme:light}
:root[data-theme=dark]{--bg:#172030;--panel:#212C3F;--line:#31405A;--text:#E9EEF6;--dim:#97A5BD;color-scheme:dark}
@media (prefers-color-scheme:dark){:root:not([data-theme=light]){--bg:#172030;--panel:#212C3F;--line:#31405A;--text:#E9EEF6;--dim:#97A5BD;color-scheme:dark}}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{margin:0;background:var(--bg);color:var(--text);font:16px/1.45 "Avenir Next","Segoe UI",system-ui,sans-serif;padding:0 0 88px}
header{display:flex;align-items:center;gap:12px;padding:16px max(16px,env(safe-area-inset-left));max-width:760px;margin:0 auto}
header h1{font-size:1.15rem;margin:0;flex:1;font-weight:650}
header small{display:block;color:var(--dim);font-weight:400;font-size:.8rem}
button,input,select{font:inherit;color:inherit}
.icon{background:none;border:1px solid var(--line);border-radius:10px;width:40px;height:40px;cursor:pointer}
main{max-width:760px;margin:0 auto;padding:0 16px}
.hero{display:grid;grid-template-columns:minmax(150px,220px) 1fr;gap:20px;align-items:center;margin-bottom:20px}
@media(max-width:520px){.hero{grid-template-columns:1fr;justify-items:center}}
.bezel{width:100%;max-width:220px;aspect-ratio:1;background:#05070B;border-radius:26px;padding:8px;box-shadow:0 0 0 2px var(--line)}
.screen{width:100%;height:100%;border-radius:18px;background:var(--screen);color:var(--screen-text);display:flex;flex-direction:column;align-items:center;justify-content:center;gap:4px;text-align:center;font-variant-numeric:tabular-nums;overflow:hidden;padding:8px}
.screen .big{font-size:3.1rem;font-weight:300;letter-spacing:-.02em;line-height:1}
.screen .mid{font-size:1.05rem;color:var(--accent)}
.screen .sm{font-size:.8rem;color:var(--screen-dim)}
.chips{display:flex;flex-wrap:wrap;gap:8px}
.chips p{flex-basis:100%;margin:0 0 2px;color:var(--dim);font-size:.9rem}
.chip{border:1px solid var(--line);background:var(--panel);border-radius:999px;padding:8px 14px;cursor:pointer}
.chip[aria-pressed=true]{background:var(--accent);border-color:var(--accent);color:var(--on-accent);font-weight:600}
nav{display:flex;gap:4px;overflow-x:auto;border-bottom:1px solid var(--line);margin-bottom:16px}
nav button{background:none;border:0;border-bottom:3px solid transparent;padding:10px 12px;cursor:pointer;color:var(--dim);white-space:nowrap}
nav button[aria-selected=true]{color:var(--text);border-color:var(--accent);font-weight:600}
section{display:none}section.on{display:block}
fieldset{border:1px solid var(--line);background:var(--panel);border-radius:14px;padding:6px 16px 14px;margin:0 0 14px}
legend{padding:0 6px;font-weight:600}
label{display:flex;justify-content:space-between;align-items:center;gap:12px;padding:9px 0;border-bottom:1px solid var(--line)}
label:last-child{border-bottom:0}
label>span:first-child{flex:1}
label small{display:block;color:var(--dim);font-size:.8rem}
input[type=text],input[type=number],input[type=password],select{background:transparent;border:1px solid var(--line);border-radius:8px;padding:7px 9px;width:min(55%,230px);min-width:0}
input[type=number]{width:96px}
input[type=range]{width:min(55%,230px);accent-color:var(--accent)}
input[type=checkbox]{width:22px;height:22px;accent-color:var(--accent)}
input[type=color]{width:46px;height:34px;border:1px solid var(--line);border-radius:8px;background:none;padding:2px}
.checks{display:flex;flex-wrap:wrap;gap:6px 18px;padding:9px 0}
.checks label{display:flex;border:0;padding:0;gap:8px;justify-content:flex-start}
:focus-visible{outline:3px solid var(--accent);outline-offset:2px}
.btn{border:1px solid var(--line);background:var(--panel);border-radius:10px;padding:9px 16px;cursor:pointer}
.btn.primary{background:var(--accent);border-color:var(--accent);color:var(--on-accent);font-weight:650}
.btn.danger{color:#D64545;border-color:#D64545}
.savebar{position:fixed;left:0;right:0;bottom:0;padding:12px 16px calc(12px + env(safe-area-inset-bottom));background:var(--bg);border-top:1px solid var(--line);display:flex;justify-content:center}
.savebar .btn{width:100%;max-width:728px;padding:12px}
ul.files{list-style:none;margin:0;padding:0}
ul.files li{display:flex;align-items:center;gap:8px;padding:10px 0;border-bottom:1px solid var(--line)}
ul.files li:last-child{border:0}
ul.files .n{flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
ul.files .sel{color:var(--accent);font-weight:600}
.bar{height:8px;border-radius:4px;background:var(--line);overflow:hidden;margin:10px 0 4px}
.bar i{display:block;height:100%;background:var(--accent);width:0}
.dim{color:var(--dim);font-size:.85rem}
.pl{display:flex;flex-wrap:wrap;gap:8px;align-items:center;padding:10px 0;border-bottom:1px solid var(--line)}
.pl select{width:auto;flex:1 1 110px}
.pl input{width:70px}
.pl .n{width:20px;color:var(--dim)}
.pl input[type=text]{flex:1 1 130px;width:auto}
.pl input[type=date],.pl input[type=time]{width:auto}
.pl button{padding:6px 10px;min-width:36px}
.sw{display:flex;flex-wrap:wrap;gap:8px;padding:2px 0 10px;border-bottom:1px solid var(--line)}
.sw button{width:30px;height:30px;border-radius:50%;border:2px solid var(--line);cursor:pointer;padding:0}
dl{display:grid;grid-template-columns:auto 1fr;gap:6px 16px;margin:8px 0}
dt{color:var(--dim)}dd{margin:0;text-align:right;font-variant-numeric:tabular-nums}
.row{display:flex;flex-wrap:wrap;gap:10px;margin-top:10px}
#toast{position:fixed;left:50%;bottom:84px;transform:translateX(-50%);background:var(--text);color:var(--bg);padding:10px 16px;border-radius:10px;opacity:0;pointer-events:none;transition:opacity .2s;max-width:90vw}
#toast.show{opacity:1}
@media (prefers-reduced-motion:reduce){#toast{transition:none}}
</style>
</head>
<body>
<header>
  <h1 id="title">SmallTV<small id="sub">Connecting…</small></h1>
  <button class="icon" id="theme" aria-label="Switch light or dark theme">◐</button>
</header>
<main>
  <div class="hero">
    <div class="bezel"><div class="screen" id="screen" aria-live="off"><div class="sm">Waiting for device</div></div></div>
    <div class="chips" id="chips">
      <p>Show on the display now (with a single page chosen, this changes which one stays)</p>
      <button class="chip" data-p="0" aria-pressed="false">Clock</button>
      <button class="chip" data-p="1" aria-pressed="false">Weather</button>
      <button class="chip" data-p="2" aria-pressed="false">Status</button>
      <button class="chip" data-p="3" aria-pressed="false">GIF</button>
      <button class="chip" data-p="4" aria-pressed="false">Calendar</button>
      <button class="chip" data-p="5" aria-pressed="false">Progress</button>
    </div>
  </div>

  <nav role="tablist" id="tabs">
    <button role="tab" data-t="display" aria-selected="true">Display</button>
    <button role="tab" data-t="faces" aria-selected="false">Faces</button>
    <button role="tab" data-t="place" aria-selected="false">Location</button>
    <button role="tab" data-t="mqtt" aria-selected="false">Home Assistant</button>
    <button role="tab" data-t="gifs" aria-selected="false">GIFs</button>
    <button role="tab" data-t="tools" aria-selected="false">Timer</button>
    <button role="tab" data-t="sys" aria-selected="false">Device</button>
  </nav>

  <section id="display" class="on">
    <fieldset><legend>What to show</legend>
      <label><span>Display mode<small>Keep one page, rotate evenly, or build your own playlist below</small></span>
        <select data-k="mode" data-num>
          <option value="0">Rotate through pages</option>
          <option value="1">Clock only</option>
          <option value="2">Weather only</option>
          <option value="3">Status only</option>
          <option value="4">GIF only</option>
          <option value="5">Custom playlist</option>
          <option value="6">Calendar only</option>
          <option value="7">Progress only</option>
        </select></label>
    </fieldset>
    <fieldset><legend>Playlists</legend>
      <p class="dim" style="margin:8px 0">Build up to four playlists and switch between them. Each item shows for its own time. A playlist plays when Display mode is "Custom playlist".</p>
      <div class="chips" id="pltabs"></div>
      <label><span>Playlist name</span><input type="text" id="plname" maxlength="11"></label>
      <div id="plist"></div>
      <div class="row">
        <button class="btn" id="padd" type="button">Add item</button>
        <button class="btn primary" id="pplay" type="button">Save and play this playlist</button>
      </div>
      <p class="dim" id="plnote"></p>
    </fieldset>
    <fieldset><legend>Pages when rotating</legend>
      <div class="checks" id="bits">
        <label><input type="checkbox" data-bit="1"> Clock</label>
        <label><input type="checkbox" data-bit="2"> Weather</label>
        <label><input type="checkbox" data-bit="4"> Status</label>
        <label><input type="checkbox" data-bit="8"> GIF</label>
        <label><input type="checkbox" data-bit="16"> Calendar</label>
        <label><input type="checkbox" data-bit="32"> Progress</label>
      </div>
      <label><span>Seconds per page</span><input type="number" min="3" max="600" data-k="rotateSec" data-num></label>
    </fieldset>
    <fieldset><legend>Screen</legend>
      <label><span>Brightness</span><input type="range" min="0" max="100" data-k="brightness" data-num></label>
      <label><span>Flip upside down<small>Use if the stand puts the screen upside down</small></span><input type="checkbox" data-k="flip"></label>
      <label><span>24-hour clock</span><input type="checkbox" data-k="h24"></label>
      <label><span>Page dots when rotating</span><input type="checkbox" data-k="pageDots"></label>
    </fieldset>
    <fieldset><legend>Night dimming</legend>
      <label><span>Dim at night</span><input type="checkbox" data-k="nightEnabled"></label>
      <label><span>From hour (0–23)</span><input type="number" min="0" max="23" data-k="nightStart" data-num></label>
      <label><span>Until hour (0–23)</span><input type="number" min="0" max="23" data-k="nightEnd" data-num></label>
      <label><span>Night brightness</span><input type="range" min="0" max="100" data-k="nightBright" data-num></label>
    </fieldset>
  </section>

  <section id="faces">
    <fieldset><legend>Colors</legend>
      <label><span>Color theme</span>
        <select data-k="theme" data-num>
          <option value="0">Black</option><option value="1">Midnight</option>
          <option value="2">Graphite</option><option value="3">Light</option>
        </select></label>
      <label><span>Accent color<small>Highlights, bars, and the Accent clock</small></span><input type="color" data-k="accent"></label>
      <div class="sw" data-for="accent"></div>
      <label><span>Time color<small>White follows the theme</small></span><input type="color" data-k="timeColor"></label>
      <div class="sw" data-for="timeColor"></div>
    </fieldset>
    <fieldset><legend>Clock</legend>
      <label><span>Clock style<small>Stacked, Day progress, Glance, Week, Sidebar and Date have their own layouts</small></span>
        <select data-k="clockStyle" data-num>
          <option value="0">Classic</option><option value="1">Accent color</option><option value="2">Cards</option>
          <option value="3">Stacked</option><option value="4">Day progress</option><option value="5">Glance</option>
          <option value="6">Week</option><option value="7">Sidebar</option><option value="8">Date</option>
        </select></label>
      <label><span>Show date</span><input type="checkbox" data-k="showDate"></label>
      <label><span>Show info under the clock<small>Outside temperature and one more value</small></span><input type="checkbox" data-k="showInfo"></label>
      <label><span>Second info value</span>
        <select data-k="infoRight" data-num>
          <option value="0">Humidity</option><option value="1">Wind</option><option value="2">High today</option>
        </select></label>
      <label><span>Seconds line at the top</span><input type="checkbox" data-k="showSeconds"></label>
    </fieldset>
    <fieldset><legend>Weather</legend>
      <label><span>Weather style<small>Forecast: three days. Hourly: next hours. Tiles, Range and List: details for today</small></span>
        <select data-k="weatherStyle" data-num>
          <option value="0">Forecast</option><option value="1">Card</option>
          <option value="2">Tiles</option><option value="3">Range</option>
          <option value="4">Hourly</option><option value="5">List</option>
        </select></label>
      <p class="dim" style="margin:8px 0">Units (°C / °F) and your location are in the Location tab.</p>
    </fieldset>
    <fieldset><legend>Calendar</legend>
      <label><span>Calendar style<small>Month grid, Tiles, or Focus (big day + small month)</small></span>
        <select data-k="calStyle" data-num>
          <option value="0">Month</option><option value="1">Tiles</option><option value="2">Focus</option>
        </select></label>
      <label><span>Week starts on Monday</span><input type="checkbox" data-k="calMon"></label>
      <label><span>Color the weekend days</span><input type="checkbox" data-k="calWeekend"></label>
      <label><span>Weekend color</span><input type="color" data-k="weekendColor"></label>
      <div class="sw" data-for="weekendColor"></div>
      <label><span>Show days of the neighbouring months<small>Dimmed</small></span><input type="checkbox" data-k="calOther"></label>
      <label><span>Clock in the calendar header</span><input type="checkbox" data-k="calClock"></label>
      <p class="dim" style="margin:8px 0">Show it with the Calendar button above, the Calendar checkbox in Pages, "Calendar only", or as a playlist item. Today uses your accent color.</p>
    </fieldset>
    <fieldset><legend>Progress</legend>
      <label><span>Progress style<small>Watch boxes show year, month and day together. Dots shows one period. Countdown styles use your events below.</small></span>
        <select data-k="progStyle" data-num id="pstyle">
          <option value="0">Watch boxes (solid)</option><option value="1">Watch boxes (tinted)</option><option value="2">Dots</option>
          <option value="3">Countdown: Big</option><option value="4">Countdown: Sentence</option>
          <option value="5">Countdown: Boxes</option><option value="6">Countdown: Bar</option>
        </select></label>
      <label><span>Show<small>Dots: day, month or year. Countdown: which event</small></span><select data-k="progArg" data-num id="parg"></select></label>
      <label><span>Show the percentage that is left<small>Off = percentage done</small></span><input type="checkbox" data-k="progLeft"></label>
      <label><span>Year color</span><input type="color" data-k="progC1"></label>
      <div class="sw" data-for="progC1"></div>
      <label><span>Month color</span><input type="color" data-k="progC2"></label>
      <div class="sw" data-for="progC2"></div>
      <label><span>Day color</span><input type="color" data-k="progC3"></label>
      <div class="sw" data-for="progC3"></div>
      <p class="dim" style="margin:8px 0">Show it with the Progress button, the Progress checkbox in Pages, "Progress only", or as a playlist item. The three colors are also used by Dots and by the countdown boxes.</p>
    </fieldset>
    <fieldset><legend>Countdown events</legend>
      <p class="dim" style="margin:8px 0">Up to four. Type the name the way it should read in the sentence, for example "the exam" or "my flight" (English letters, digits and basic punctuation). "Count from" is optional; it defaults to the day you save.</p>
      <div id="evlist"></div>
    </fieldset>
  </section>

  <section id="place">
    <fieldset><legend>Weather</legend>
      <label><span>Latitude</span><input type="number" step="0.0001" data-k="lat" data-num style="width:130px"></label>
      <label><span>Longitude<small>Find yours by searching a city on any map</small></span><input type="number" step="0.0001" data-k="lon" data-num style="width:130px"></label>
      <label><span>Fahrenheit</span><input type="checkbox" data-k="fahrenheit"></label>
    </fieldset>
    <fieldset><legend>Time zone</legend>
      <label><span>Region</span>
        <select id="tzpreset">
          <option value="UTC0">UTC</option>
          <option value="GMT0BST,M3.5.0/1,M10.5.0">London</option>
          <option value="CET-1CEST,M3.5.0,M10.5.0/3">Central Europe</option>
          <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Eastern Europe</option>
          <option value="&lt;+03&gt;-3">UTC+3, no daylight saving</option>
          <option value="IST-5:30">India</option>
          <option value="&lt;+08&gt;-8">UTC+8</option>
          <option value="JST-9">Japan</option>
          <option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Sydney</option>
          <option value="EST5EDT,M3.2.0,M11.1.0">US Eastern</option>
          <option value="CST6CDT,M3.2.0,M11.1.0">US Central</option>
          <option value="MST7MDT,M3.2.0,M11.1.0">US Mountain</option>
          <option value="PST8PDT,M3.2.0,M11.1.0">US Pacific</option>
          <option value="">Custom</option>
        </select></label>
      <label><span>POSIX string<small>Advanced: edit for any other zone</small></span><input type="text" data-k="tz" id="tz" maxlength="39"></label>
    </fieldset>
    <fieldset><legend>Network name</legend>
      <label><span>Hostname<small>Changes after reboot. Reach the device at name.local</small></span><input type="text" data-k="hostname" maxlength="23"></label>
    </fieldset>
  </section>

  <section id="mqtt">
    <fieldset><legend>MQTT broker</legend>
      <label><span>Connect to broker</span><input type="checkbox" data-k="mqttEnabled"></label>
      <label><span>Host</span><input type="text" data-k="mqttHost" maxlength="47" placeholder="192.168.1.10"></label>
      <label><span>Port</span><input type="number" data-k="mqttPort" data-num></label>
      <label><span>Username</span><input type="text" data-k="mqttUser" maxlength="31" autocomplete="off"></label>
      <label><span>Password</span><input type="password" data-k="mqttPass" maxlength="47" placeholder="Leave blank to keep" autocomplete="new-password"></label>
      <label><span>Topic prefix</span><input type="text" data-k="mqttTopic" maxlength="31"></label>
      <label><span>Add to Home Assistant automatically<small>Creates temperature, signal and brightness entities</small></span><input type="checkbox" data-k="haDiscovery"></label>
    </fieldset>
    <p class="dim">Other commands: publish a number 0–3 to <code id="pt">smalltv/page/set</code> to switch pages.</p>
  </section>

  <section id="gifs">
    <fieldset><legend>Playback</legend>
      <label><span>Play all GIFs in turn<small>Moves to the next GIF at the end of a loop</small></span><input type="checkbox" data-k="gifAuto"></label>
      <label><span>Seconds per GIF</span><input type="number" min="3" max="3600" data-k="gifSecs" data-num></label>
      <label><span>Speed (%)<small>100 = the GIF's own speed. Lower is slower</small></span><input type="number" min="25" max="300" data-k="gifSpeed" data-num></label>
    </fieldset>
    <fieldset><legend>Your GIFs</legend>
      <ul class="files" id="files"><li class="dim">Loading…</li></ul>
      <div class="bar"><i id="fsbar"></i></div>
      <div class="dim" id="fstext"></div>
      <div class="row">
        <input type="file" id="gfile" accept=".gif,image/gif" hidden>
        <button class="btn primary" id="upbtn">Upload a GIF</button>
        <button class="btn" id="nextgif">Next GIF &#9654;</button>
      </div>
      <p class="dim">Best results: 240×240 pixels, under 500 KB. Large or long animations play slowly on this chip.</p>
    </fieldset>
  </section>

  <section id="tools">
    <fieldset><legend>Timer</legend>
      <div class="row" id="tpre"></div>
      <label><span>Minutes</span><input type="number" id="tmin" min="0" max="99" value="5"></label>
      <label><span>Seconds</span><input type="number" id="tsec" min="0" max="59" value="0"></label>
      <div class="row">
        <button class="btn primary" id="tstart" type="button">Start timer</button>
        <button class="btn" id="tpause" type="button">Pause</button>
        <button class="btn" id="tstop" type="button">Cancel</button>
      </div>
      <p class="dim" id="tstat">No timer running</p>
    </fieldset>
    <fieldset><legend>Stopwatch</legend>
      <div class="row">
        <button class="btn primary" id="swstart" type="button">Start</button>
        <button class="btn" id="swstop" type="button">Stop</button>
        <button class="btn" id="swlap" type="button">Lap</button>
        <button class="btn" id="swreset" type="button">Reset</button>
      </div>
      <p class="dim" id="swstat">Stopwatch is off</p>
    </fieldset>
    <div class="row"><button class="btn" id="toolclose" type="button">Back to the normal display</button></div>
    <p class="dim">The timer and stopwatch take over the screen while they run. When a timer ends the screen flashes for up to a minute, or until you press Back.</p>
  </section>

  <section id="sys">
    <fieldset id="fswarn" hidden><legend>Storage problem</legend>
      <p>The device could not open its storage, so settings and GIFs can't be saved. Formatting erases everything stored on the device, including the original firmware's GIFs and settings.</p>
      <button class="btn danger" id="fmt">Format storage</button>
    </fieldset>
    <fieldset><legend>Device</legend>
      <dl id="stats"></dl>
    </fieldset>
    <p class="dim">To update: open the upload page, send SmallTV-Installer.bin first, then install the new SmallTV-Modern.bin from the installer's page. (The firmware is too big to replace itself directly.)</p>
    <div class="row">
      <a class="btn" href="/update">Update firmware</a>
      <button class="btn" id="reboot">Restart</button>
      <button class="btn danger" id="reset">Factory reset</button>
    </div>
  </section>
</main>
<div class="savebar"><button class="btn primary" id="save">Save changes</button></div>
<div id="toast" role="status" aria-live="polite"></div>

<script>
const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
let cfg={},st={},dirty=false;
const toast=m=>{const t=$('#toast');t.textContent=m;t.classList.add('show');clearTimeout(toast.h);toast.h=setTimeout(()=>t.classList.remove('show'),2800)};
async function api(p,o){const r=await fetch(p,o);let j={};try{j=await r.json()}catch(e){}if(!r.ok)throw new Error(j.error||'Request failed');return j}

/* theme */
function theme(t){if(t)document.documentElement.dataset.theme=t;try{t?localStorage.setItem('th',t):0}catch(e){}}
try{const s=localStorage.getItem('th');if(s)theme(s)}catch(e){}
$('#theme').onclick=()=>{const d=matchMedia('(prefers-color-scheme:dark)').matches;const cur=document.documentElement.dataset.theme||(d?'dark':'light');theme(cur==='dark'?'light':'dark')};

/* tabs */
$$('#tabs button').forEach(b=>b.onclick=()=>{$$('#tabs button').forEach(x=>x.setAttribute('aria-selected',x===b));$$('main section').forEach(s=>s.classList.toggle('on',s.id===b.dataset.t));if(b.dataset.t==='gifs')loadGifs()});

/* progress style + countdown events */
const PERIODS=['Day','Month','Year'];
let evs=[];
const esc=t=>String(t).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const evName=i=>(evs[i]&&evs[i].n)?evs[i].n:('Event '+(i+1));
function argOptions(face){return face<=1?[]:(face===2?PERIODS:[0,1,2,3].map(evName))}
function syncArg(sel){const f=+$('#pstyle').value,a=argOptions(f),s=$('#parg');s.innerHTML=a.length?opts(a,Math.min(sel,a.length-1)):'<option value="0">-</option>';s.disabled=!a.length;if(a.length)s.value=Math.min(sel,a.length-1)}
$('#pstyle').addEventListener('change',()=>syncArg(0));
const d2s=(y,m,d)=>y?`${y}-${String(m).padStart(2,'0')}-${String(d).padStart(2,'0')}`:'';
const days2s=n=>n?new Date(n*86400000).toISOString().slice(0,10):'';
const s2days=t=>{const m=/^(\d+)-(\d+)-(\d+)$/.exec(t);return m?Math.round(Date.UTC(+m[1],+m[2]-1,+m[3])/86400000):0};
function initEvs(){evs=[0,1,2,3].map(i=>{const q=(cfg.ev||[])[i]||{};return{n:q.n||'',y:q.y||0,m:q.m||0,d:q.d||0,h:q.h||0,i:q.i||0,s:q.s||0}});renderEvs()}
function renderEvs(){
  const box=$('#evlist');box.innerHTML='';
  evs.forEach((e,k)=>{
    const d=document.createElement('div');d.className='pl';
    d.innerHTML=`<span class="n">${k+1}</span><input type="text" class="en" maxlength="23" placeholder="the exam" value="${esc(e.n)}" aria-label="Name"><input type="date" class="ed" value="${d2s(e.y,e.m,e.d)}" aria-label="Date"><input type="time" class="et" value="${String(e.h).padStart(2,'0')}:${String(e.i).padStart(2,'0')}" aria-label="Time"><label class="dim" style="border:0;padding:0;gap:6px;justify-content:flex-start">Count from <input type="date" class="es" value="${days2s(e.s)}" aria-label="Count from"></label><button class="btn" type="button">Clear</button>`;
    d.querySelector('.en').oninput=x=>{e.n=x.target.value;dirty=true;syncArg(+$('#parg').value||0);if(typeof renderList==='function')renderList()};
    d.querySelector('.ed').onchange=x=>{const m=/^(\d+)-(\d+)-(\d+)$/.exec(x.target.value);e.y=m?+m[1]:0;e.m=m?+m[2]:0;e.d=m?+m[3]:0;if(!d.querySelector('.es').value)e.s=0;dirty=true};
    d.querySelector('.et').onchange=x=>{const m=/^(\d+):(\d+)$/.exec(x.target.value);e.h=m?+m[1]:0;e.i=m?+m[2]:0;dirty=true};
    d.querySelector('.es').onchange=x=>{e.s=s2days(x.target.value);dirty=true};
    d.querySelector('button').onclick=()=>{Object.assign(e,{n:'',y:0,m:0,d:0,h:0,i:0,s:0});dirty=true;renderEvs();syncArg(+$('#parg').value||0);renderList()};
    box.appendChild(d);
  });
}

/* playlists */
const PGN=['Clock','Weather','Status','GIF','Calendar','Progress'];
const FACES=[['Classic','Accent','Cards','Stacked','Day progress','Glance','Week','Sidebar','Date'],['Forecast','Card','Tiles','Range','Hourly','List'],[],[],['Month','Tiles','Focus'],['Watch boxes (solid)','Watch boxes (tinted)','Dots','Countdown: Big','Countdown: Sentence','Countdown: Boxes','Countdown: Bar']];
let pls=[],plSel=0,plAct=0;
const opts=(a,sel)=>a.map((n,i)=>`<option value="${i}"${i===sel?' selected':''}>${n}</option>`).join('');
function initPls(){
  pls=[0,1,2,3].map(i=>{const q=(cfg.pl||[])[i]||{};return{n:q.n||('Playlist '+(i+1)),l:(q.l||[]).map(x=>({p:x.p,s:x.s,a:x.a||0,t:x.t}))}});
  plAct=cfg.plActive||0;if(plSel>3)plSel=0;renderTabs(true);renderList();
}
function renderTabs(setName){
  const b=$('#pltabs');b.innerHTML='';
  pls.forEach((p,i)=>{const x=document.createElement('button');x.type='button';x.className='chip';x.setAttribute('aria-pressed',i===plSel);x.textContent=(i===plAct?'\u25B6 ':'')+p.n;x.onclick=()=>{plSel=i;renderTabs(true);renderList()};b.appendChild(x)});
  if(setName)$('#plname').value=pls[plSel].n;
  $('#plnote').textContent=(plAct===plSel?'This playlist is the one that plays.':'The one that plays now is "'+pls[plAct].n+'".')+' ('+pls[plSel].l.length+' of 10 items)';
}
$('#plname').oninput=e=>{pls[plSel].n=e.target.value.slice(0,11)||('Playlist '+(plSel+1));dirty=true;renderTabs(false)};
function renderList(){
  const box=$('#plist'),L=pls[plSel].l;box.innerHTML='';
  if(!L.length){box.innerHTML='<p class="dim">No items yet. Press Add item.</p>';return}
  L.forEach((it,i)=>{
    const d=document.createElement('div');d.className='pl';
    const fc=FACES[it.p]||[],ao=it.p===5?argOptions(it.s):[];
    d.innerHTML=`<span class="n">${i+1}</span><select class="lp" aria-label="Page">${opts(PGN,it.p)}</select>${fc.length?`<select class="ls" aria-label="Face">${opts(fc,it.s)}</select>`:''}${ao.length?`<select class="la" aria-label="Show">${opts(ao,it.a||0)}</select>`:''}<input class="lt" type="number" min="1" max="600" value="${it.t}" aria-label="Seconds"><span class="dim">sec</span><button class="btn" type="button" data-a="up" aria-label="Move up">&#9650;</button><button class="btn" type="button" data-a="dn" aria-label="Move down">&#9660;</button><button class="btn" type="button" data-a="rm" aria-label="Remove">&#10005;</button>`;
    d.querySelector('.lp').onchange=e=>{it.p=+e.target.value;it.s=0;it.a=0;dirty=true;renderList()};
    const ls=d.querySelector('.ls');if(ls)ls.onchange=e=>{it.s=+e.target.value;it.a=0;dirty=true;if(it.p===5)renderList()};
    const la=d.querySelector('.la');if(la)la.onchange=e=>{it.a=+e.target.value;dirty=true};
    d.querySelector('.lt').oninput=e=>{it.t=Math.max(1,Math.min(600,+e.target.value||1));dirty=true};
    d.querySelectorAll('button').forEach(b=>b.onclick=()=>{const a=b.dataset.a;dirty=true;if(a==='rm')L.splice(i,1);else if(a==='up'&&i>0)[L[i-1],L[i]]=[L[i],L[i-1]];else if(a==='dn'&&i<L.length-1)[L[i+1],L[i]]=[L[i],L[i+1]];renderList();renderTabs(false)});
    box.appendChild(d);
  });
}
$('#padd').onclick=()=>{const L=pls[plSel].l;if(L.length>=10){toast('Up to 10 items per playlist');return}L.push({p:L.length&&L[L.length-1].p===0?1:0,s:0,a:0,t:10});dirty=true;renderList();renderTabs(false)};

/* color swatches */
const PRESET=['#FF6B57','#FF9F0A','#FFD60A','#A3E635','#30D158','#2DD4BF','#64D2FF','#0A84FF','#BF5AF2','#FF375F','#FFFFFF'];
$$('.sw').forEach(box=>{const k=box.dataset.for;PRESET.forEach(c=>{const b=document.createElement('button');b.type='button';b.style.background=c;b.setAttribute('aria-label','Use '+c);b.onclick=()=>{const i=$('[data-k='+k+']');i.value=c.toLowerCase();i.dispatchEvent(new Event('input',{bubbles:true}));if(k==='accent')setAccent(c)};box.appendChild(b)})});

/* accent */
function setAccent(h){document.documentElement.style.setProperty('--accent',h);const n=parseInt(h.slice(1),16),r=n>>16,g=n>>8&255,b=n&255;document.documentElement.style.setProperty('--on-accent',(r*299+g*587+b*114)/1000>150?'#0B1220':'#FFFFFF')}

/* config */
function fill(){
  $$('[data-k]').forEach(e=>{const v=cfg[e.dataset.k];if(v===undefined)return;if(e.type==='checkbox')e.checked=!!v;else if(e.type!=='password')e.value=v});
  $$('[data-bit]').forEach(e=>e.checked=(cfg.pages&e.dataset.bit)>0);
  const o=[...$('#tzpreset').options].find(x=>x.value===cfg.tz);$('#tzpreset').value=o?o.value:'';
  setAccent(cfg.accent||'#2DD4BF');$('#pt').textContent=(cfg.mqttTopic||'smalltv')+'/page/set';
}
async function loadCfg(){cfg=await api('/api/config');fill();initEvs();initPls();syncArg(cfg.progArg||0)}
$('#tzpreset').onchange=e=>{if(e.target.value)$('#tz').value=e.target.value};
$('[data-k=accent]').oninput=e=>setAccent(e.target.value);
$$('[data-k],[data-bit]').forEach(e=>e.addEventListener('input',()=>dirty=true));
async function saveAll(over){
  const body={};
  $$('[data-k]').forEach(e=>{const k=e.dataset.k;body[k]=e.type==='checkbox'?e.checked:(e.hasAttribute('data-num')?Number(e.value):e.value)});
  body.pages=$$('[data-bit]').reduce((a,e)=>a|(e.checked?+e.dataset.bit:0),0)||1;
  body.pl=pls;body.plActive=plAct;body.ev=evs;
  Object.assign(body,over||{});
  if(body.mode===5&&!pls[body.plActive].l.length){toast('Add at least one item to the playlist first');return}
  try{const r=await api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});dirty=false;toast(r.saved===false?'Applied, but not saved: storage unavailable':'Saved');await loadCfg()}catch(e){toast(e.message)}
}
$('#save').onclick=()=>saveAll();
$('#pplay').onclick=()=>{plAct=plSel;saveAll({mode:5,plActive:plSel})};

/* live status + preview */
function preview(){
  const s=$('#screen'),p=st.page;let h='';
  if(p===0)h=`<div class="big">${(st.time||'--:--').slice(0,5)}</div><div class="mid">${st.date||''}</div>`;
  else if(p===1)h=st.temp!==undefined?`<div class="big">${Math.round(st.temp)}°</div><div class="mid">${st.condition}</div><div class="sm">High ${Math.round(st.hi)}  Low ${Math.round(st.lo)}</div>`:'<div class="sm">Fetching weather…</div>';
  else if(p===2)h=`<div class="mid">${st.ip}</div><div class="sm">${st.rssi} dBm · ${Math.round(st.heap/1024)} KB free</div>`;
  else if(p===5)h=`<div class="mid">Progress</div><div class="sm">${st.date||''}</div>`;
  else if(p===4)h=`<div class="mid">Calendar</div><div class="sm">${st.date||''}</div>`;
  else h=`<div class="mid">GIF</div><div class="sm">${cfg.gif||'none selected'}</div>`;
  s.innerHTML=h;
  $$('#chips .chip').forEach(c=>c.setAttribute('aria-pressed',+c.dataset.p===p));
}
function up(s){const d=Math.floor(s/86400),h=Math.floor(s/3600)%24,m=Math.floor(s/60)%60;return`${d}d ${h}h ${m}m`}
async function poll(){
  if(document.hidden)return;
  try{
    st=await api('/api/status');
    $('#fswarn').hidden=st.fs!==false;
    $('#title').firstChild.textContent=st.hostname;$('#sub').textContent=`${st.ip} · v${st.version}`;
    preview();toolStatus();
    $('#stats').innerHTML=[['Address',st.ip],['Signal',st.rssi+' dBm'],['Free memory',Math.round(st.heap/1024)+' KB'],['Uptime',up(st.uptime)],['MQTT',st.mqtt],['Storage',Math.round(st.fsUsed/1024)+' of '+Math.round(st.fsTotal/1024)+' KB'],['Firmware','v'+st.version]].map(r=>`<dt>${r[0]}</dt><dd>${r[1]}</dd>`).join('');
  }catch(e){$('#sub').textContent='Device not responding'}
}
$$('#chips .chip').forEach(c=>c.onclick=async()=>{try{await api('/api/page?n='+c.dataset.p,{method:'POST'});if(cfg.mode!==0)await loadCfg();poll()}catch(e){toast(e.message)}});

/* gifs */
async function loadGifs(){
  try{
    const g=await api('/api/gifs'),ul=$('#files');
    ul.innerHTML=g.files.length?'':'<li class="dim">No GIFs yet. Upload one to get started.</li>';
    g.files.forEach(f=>{
      const li=document.createElement('li'),sel=f.name===g.selected;
      li.innerHTML=`<span class="n${sel?' sel':''}"></span><span class="dim">${Math.round(f.size/1024)} KB</span><button class="btn">${sel?'Show':'Use'}</button><button class="btn danger" aria-label="Delete"></button>`;
      li.querySelector('.n').textContent=f.name+(sel?' (selected)':'');
      const [use,del]=li.querySelectorAll('button');del.textContent='Delete';
      use.onclick=async()=>{try{await api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({gif:f.name,pages:(cfg.pages|8)})});await loadCfg();await api('/api/page?n=3',{method:'POST'});loadGifs();toast('Showing '+f.name)}catch(e){toast(e.message)}};
      del.onclick=async()=>{if(!confirm('Delete '+f.name+'?'))return;try{await api('/api/gif?name='+encodeURIComponent(f.name),{method:'DELETE'});await loadCfg();loadGifs()}catch(e){toast(e.message)}};
      ul.appendChild(li);
    });
    $('#fsbar').style.width=(100*g.used/g.total)+'%';
    $('#fstext').textContent=`${Math.round(g.used/1024)} of ${Math.round(g.total/1024)} KB used`;
  }catch(e){toast(e.message)}
}
$('#upbtn').onclick=()=>$('#gfile').click();
$('#nextgif').onclick=async()=>{try{await api('/api/gifnext',{method:'POST'});await loadCfg();loadGifs();toast('Next GIF')}catch(e){toast(e.message)}};
$('#gfile').onchange=e=>{
  const f=e.target.files[0];if(!f)return;
  const fd=new FormData();fd.append('file',f,f.name);
  const x=new XMLHttpRequest(),b=$('#upbtn');b.disabled=true;
  x.upload.onprogress=ev=>{if(ev.lengthComputable)b.textContent='Uploading '+Math.round(100*ev.loaded/ev.total)+'%'};
  x.onload=()=>{b.disabled=false;b.textContent='Upload a GIF';let m='Uploaded';if(x.status!==200){try{m=JSON.parse(x.responseText).error}catch(_){m='Upload failed'}}toast(m);loadGifs()};
  x.onerror=()=>{b.disabled=false;b.textContent='Upload a GIF';toast('Upload failed')};
  x.open('POST','/api/gif');x.send(fd);e.target.value='';
};

/* timer + stopwatch */
const tool=(cmd,secs)=>api('/api/tool?cmd='+cmd+(secs!==undefined?'&secs='+secs:''),{method:'POST'}).then(poll).catch(e=>toast(e.message));
const fm=s=>String(Math.floor(s/60)).padStart(2,'0')+':'+String(s%60).padStart(2,'0');
[[60,'1 min'],[180,'3 min'],[300,'5 min'],[600,'10 min'],[900,'15 min'],[1800,'30 min']].forEach(([s,l])=>{const b=document.createElement('button');b.type='button';b.className='btn';b.textContent=l;b.onclick=()=>tool('timer_start',s);$('#tpre').appendChild(b)});
$('#tstart').onclick=()=>{const s=(+$('#tmin').value||0)*60+(+$('#tsec').value||0);if(s<1){toast('Set a time first');return}tool('timer_start',s)};
$('#tpause').onclick=()=>tool(st.tmRun?'timer_pause':'timer_resume');
$('#tstop').onclick=()=>tool('close');
$('#swstart').onclick=()=>tool('sw_start');$('#swstop').onclick=()=>tool('sw_stop');$('#swlap').onclick=()=>tool('sw_lap');$('#swreset').onclick=()=>tool('sw_reset');
$('#toolclose').onclick=()=>tool('close');
function toolStatus(){
  $('#tstat').textContent=st.tool===1?(st.tmDone?"Time's up!":(st.tmRun?'Running: ':'Paused: ')+fm(st.tmLeft)):'No timer running';
  $('#tpause').textContent=st.tmRun?'Pause':'Resume';
  $('#swstat').textContent=st.tool===2?(st.swRun?'Running: ':'Stopped: ')+fm(Math.floor(st.swMs/1000))+'.'+Math.floor(st.swMs%1000/100):'Stopwatch is off';
}

/* system */
$('#fmt').onclick=async()=>{if(confirm('Erase ALL stored data on the device?')){try{await api('/api/format',{method:'POST'});toast('Formatted');setTimeout(()=>location.reload(),1200)}catch(e){toast(e.message)}}};
$('#reboot').onclick=async()=>{if(confirm('Restart the device?')){try{await api('/api/reboot',{method:'POST'})}catch(e){}toast('Restarting…')}};
$('#reset').onclick=async()=>{if(confirm('Erase settings and WiFi? The device will start its setup network.')){try{await api('/api/reset',{method:'POST'})}catch(e){}toast('Resetting…')}};

loadCfg().then(poll);setInterval(poll,2000);
addEventListener('beforeunload',e=>{if(dirty){e.preventDefault();e.returnValue=''}});
</script>
</body>
</html>
)HTMLUI";
