#ifndef HTTP_UI_H
#define HTTP_UI_H

/*
 *  Static HTML/CSS/JS page templates served by http_server.cpp.
 *  Kept here to keep the request handlers readable.
 */

#include <pgmspace.h>
#include "config.h"

static const char HTML_BEGIN[] PROGMEM = R"(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0">
    <title>WiFi File Storage</title>
    <link rel="icon" type="image/svg+xml" href="data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA1NzYgNTEyIj48IS0tISBGb250IEF3ZXNvbWUgRnJlZSA3LjMuMCBieSBAZm9udGF3ZXNvbWUgLSBodHRwczovL2ZvbnRhd2Vzb21lLmNvbSBMaWNlbnNlIC0gaHR0cHM6Ly9mb250YXdlc29tZS5jb20vbGljZW5zZS9mcmVlIChJY29uczogQ0MgQlkgNC4wLCBGb250czogU0lMIE9GTCAxLjEsIENvZGU6IE1JVCBMaWNlbnNlKSBDb3B5cmlnaHQgMjAyNiBGb250aWNvbnMsIEluYy4gLS0+PHBhdGggZmlsbD0iY3VycmVudENvbG9yIiBkPSJNMTQ0IDQ4MGMtNzkuNSAwLTE0NC02NC41LTE0NC0xNDQgMC02My40IDQxLTExNy4yIDk3LjktMTM2LjUtMS4zLTcuNy0xLjktMTUuNS0xLjktMjMuNSAwLTc5LjUgNjQuNS0xNDQgMTQ0LTE0NCA1NS40IDAgMTAzLjUgMzEuMyAxMjcuNiA3Ny4xIDE0LjItOC4zIDMwLjgtMTMuMSA0OC40LTEzLjEgNTMgMCA5NiA0MyA5NiA5NiAwIDE1LjctMy44IDMwLjYtMTAuNSA0My43IDQ0IDIwLjMgNzQuNSA2NC43IDc0LjUgMTE2LjMgMCA3MC43LTU3LjMgMTI4LTEyOCAxMjhsLTMwNCAwek0zMDUgMTkxYy05LjQtOS40LTI0LjYtOS40LTMzLjkgMGwtNzIgNzJjLTkuNCA5LjQtOS40IDI0LjYgMCAzMy45czI0LjYgOS40IDMzLjkgMGwzMS0zMSAwIDEwMi4xYzAgMTMuMyAxMC43IDI0IDI0IDI0czI0LTEwLjcgMjQtMjRsMC0xMDIuMSAzMSAzMWM5LjQgOS40IDI0LjYgOS40IDMzLjkgMHM5LjQtMjQuNiAwLTMzLjlsLTcyLTcyeiIvPjwvc3ZnPg==">
    <style>
      body { background:var(--bg); color:var(--text); font-family: system-ui, Arial, Helvetica, sans-serif; margin:0; }
      a{ color:var(--link); }
      .contain{ width:100%; }
      .center_div{ margin:0 auto; width:92%; max-width:1100px; position:relative; padding:1.2rem 0 4.5rem; }
    </style>
  </head>
  <body>
)";

static const char HTML_END[] PROGMEM = "</body></html>";

// Shared top navigation bar. Inserted right after each page's container opens.
static const char NAV_HTML[] PROGMEM = R"NAV(
<style>
  .nav{ display:flex; align-items:center; gap:.5rem; flex-wrap:wrap; margin:3px 0; }
  .nav a, .nav button{ border:0; cursor:pointer; font-family:inherit; border-radius:.5rem;
    background:var(--bg); color:var(--text); font-size:.85rem; line-height:1;
    padding:.5rem .8rem; text-decoration:none; opacity:1; transition:opacity .15s; }
  .nav a:hover, .nav button:hover{ opacity:.6; }
  .nav a.active{ opacity:.6; }
  .menuwrap{ position:relative; margin-left:auto; }
  .navmenu{ display:none; position:absolute; right:0; top:120%; background:var(--card-bg);
    border:1px solid var(--border); border-radius:.5rem; box-shadow:0 6px 18px rgba(0,0,0,.45);
    min-width:170px; padding:.3rem; z-index:80; }
  .navmenu.open{ display:block; }
  .navmenu button{ display:block; width:100%; text-align:left; border:0; background:none;
    color:var(--text); font-size:.85rem; padding:.5rem .7rem; border-radius:.35rem; cursor:pointer; }
  .navmenu button:hover{ opacity:.6; }
</style>
<div class="nav">
  <a href="/">Home</a>
  <a href="/config">Config</a>
  <a href="/api">API docs</a>
  <div class="menuwrap">
    <button onclick="event.stopPropagation(); document.getElementById('navmenu').classList.toggle('open');">&#9776;</button>
    <div id="navmenu" class="navmenu" onclick="event.stopPropagation();">
      <button onclick="document.getElementById('navmenu').classList.remove('open'); fetch('/theme').then(function(){ location.reload(); });">Toggle Theme</button>
      <button id="menuEject" onclick="document.getElementById('navmenu').classList.remove('open'); ejectSD();">Eject SD Card</button>
      <button id="menuFormat" onclick="document.getElementById('navmenu').classList.remove('open'); formatSD();">Format SD Card</button>
    </div>
  </div>
</div>
<script>
  (function(){var p=location.pathname,l=document.querySelectorAll('.nav a');
    for(var i=0;i<l.length;i++){ if(l[i].getAttribute('href')===p){ l[i].className='active'; } }})();
  document.addEventListener('click', function(){ document.getElementById('navmenu').classList.remove('open'); });
  document.addEventListener('DOMContentLoaded', function(){
    if(typeof ejectSD !== 'function'){ document.getElementById('menuEject').style.display='none'; }
    if(typeof formatSD !== 'function'){ document.getElementById('menuFormat').style.display='none'; }
  });
</script>
)NAV";

static const char INDEX_HTML_0[] PROGMEM = R"(
<style>
  .ip{ color:var(--muted); font-size:.8rem; text-align:center; margin:0;
    position:fixed; left:0; bottom:0; width:100%; z-index:50;
    background:var(--bg); box-sizing:border-box; padding:.5rem 1rem; }
  .card{ background:var(--card-bg); border:1px solid var(--border); border-radius:.9rem; padding:1.1rem; margin-bottom:1.1rem; }
  .btn-row{ display:flex; flex-wrap:wrap; gap:.5rem; margin:3px 0; }
  .btn{ position:relative; z-index:0; border:0; border-radius:.55rem; color:var(--text);
    background:var(--card-bg); font-size:.85rem; padding:.6rem 1rem; cursor:pointer; }
  .btn::before{ content:''; position:absolute; inset:0; z-index:-1; border-radius:inherit;
    background:var(--btn); opacity:0; transition:opacity .15s; }
  .btn:hover::before{ opacity:1; }
  .ghead{ margin:0 0 .8rem; font-size:.95rem; color:var(--text); }
  #crumb{ background:var(--bg); border-radius:.55rem; padding:.6rem .8rem; margin-bottom:.8rem;
    font-size:.85rem; color:var(--muted); }
  table.files{ width:100%; border-collapse:collapse; font-size:.85rem; }
  table.files th, table.files td{ text-align:left; padding:.5rem .4rem; border-bottom:1px solid var(--border); }
  table.files th{ color:var(--muted); font-weight:600; }
  table.files .ficon{ display:inline-block; vertical-align:-2px; margin-right:.4rem; color:#e0b357; }
  table.files td.sz{ color:var(--muted); white-space:nowrap; }
  table.files td.rowmenu{ width:1%; white-space:nowrap; text-align:right; }
  .ellipsisbtn{ border:0; background:none; color:var(--muted); font-size:1rem; line-height:1;
    padding:.3rem .5rem; cursor:pointer; opacity:1; transition:opacity .15s; }
  .ellipsisbtn:hover{ opacity:.6; }
  table.files td.cfgfile{ color:#8a7fd1; font-style:italic; }
  table.files a.pvlink{ color:inherit; text-decoration:none; cursor:pointer; }
  table.files tr.pvrow:hover{ opacity:.7; }
  #ctxmenu{ position:fixed; display:none; background:var(--card-bg); border:1px solid var(--border);
    border-radius:.5rem; box-shadow:0 6px 18px rgba(0,0,0,.45); z-index:60; min-width:140px; padding:.3rem; }
  #ctxmenu button{ display:block; width:100%; text-align:left; border:0; background:none; color:var(--text);
    font-size:.85rem; padding:.5rem .7rem; border-radius:.35rem; cursor:pointer; }
  #ctxmenu button:hover{ background:var(--btn); }
  #ctxmenu button.danger{ color:var(--accent); }
  #ctxmenu button.danger:hover{ background:var(--accent); color:#fff; }
  #movebox{ display:none; position:fixed; inset:0; background:rgba(0,0,0,.5); z-index:70; }
  #movebox .card{ max-width:360px; margin:10vh auto; }
  .mvrow{ padding:.45rem .3rem; cursor:pointer; border-radius:.35rem; font-size:.85rem; }
  .mvrow:hover{ background:var(--btn); }
  .center_div{ display:flex; flex-direction:column; min-height:100vh; box-sizing:border-box; }
  .layout{ display:flex; gap:1rem; align-items:stretch; flex:1; min-height:0; }
  .maincol{ flex:1; min-width:0; display:flex; flex-direction:column; min-height:0; }
  .listcard{ display:flex; flex-direction:column; flex:1; min-height:0; }
  .filewrap{ flex:1; min-height:0; overflow-y:auto; }
  #previewcol{ display:none; width:320px; flex:0 0 320px; position:sticky; top:1.2rem; align-self:flex-start; }
  #previewcol img{ max-width:100%; border-radius:.5rem; display:block; }
  #previewcol:fullscreen, #previewcol:-webkit-full-screen{
    width:100vw; height:100vh; max-width:none; position:static; top:auto;
    display:flex; flex-direction:column; box-sizing:border-box; }
  #previewcol:fullscreen img, #previewcol:-webkit-full-screen img{
    flex:1; min-height:0; max-height:100%; max-width:100%; margin:auto; object-fit:contain; }
  #previewcol:fullscreen pre, #previewcol:-webkit-full-screen pre{ flex:1; max-height:none; }
  .iconbtn{ padding:.35rem .5rem; display:inline-flex; align-items:center; justify-content:center; }
  @media (max-width:700px){
    .layout{ flex-direction:column; }
    #previewcol{ width:100%; flex:none; position:static; }
  }
  .loadoverlay{ position:fixed; inset:0; background:var(--bg); z-index:9999;
    display:flex; align-items:center; justify-content:center; }
  .spinner{ width:40px; height:40px; border:4px solid var(--border);
    border-top-color:var(--accent); border-radius:50%; animation:spin .8s linear infinite; }
  @keyframes spin{ to{ transform:rotate(360deg); } }
</style>
<div class="contain">
  <div class="center_div">
)";

static const char SD_MISSING_HTML[] PROGMEM = R"(
  <div class="card">
    <p class="ghead">SD Card</p>
    <p style="color:#ff8b8b;">No Micro SD card detected. Please insert an SD card to enable file storage.</p>
  </div>
</div>
)";

static const char SD_FAULTY_HTML[] PROGMEM = R"(
  <div class="card">
    <p class="ghead">SD Card</p>
    <p style="color:#ffcf7a;">SD card detected, but its filesystem could not be read. It may be corrupted or unformatted - formatting will erase it and fix this.</p>
    <div class="btn-row">
      <button class="btn" onclick="formatSD();">Format SD card</button>
    </div>
  </div>
</div>
)";

// Always emitted at the end of the start page (both with and without a card),
// so the status div/functions and the format button exist regardless of
// whether the ready/faulty/missing block was rendered above. Also polls SD
// status so the page reloads itself as soon as it changes (inserted, ejected,
// or turns out to be faulty).
static const char SD_POLL_HTML[] PROGMEM = R"(
<script>
  var __statusDefault = document.getElementById('status').textContent;
  var __statusTimer = null;
  function showStatus(msg, kind){
    var s=document.getElementById('status');
    if(__statusTimer){ clearTimeout(__statusTimer); }
    s.textContent=msg;
    __statusTimer=setTimeout(function(){
      s.textContent=__statusDefault;
      s.className='ip';
      __statusTimer=null;
    }, 3000);
  }
  function formatSD(){
    if(!confirm('This will ERASE ALL DATA on the SD card. Continue?')){ return; }
    showStatus('Formatting SD card...', 'info');
    fetch('/format').then(function(r){
      return r.text().then(function(t){
        showStatus(t, r.ok ? 'ok' : 'err');
        setTimeout(function(){ location.reload(); }, 1200);
      });
    }).catch(function(e){ showStatus('Format error: ' + e.message, 'err'); });
  }
  var __sdKnown = null;
  function __pollSd(){
    fetch('/api/sdstatus').then(function(r){return r.text();}).then(function(t){
      t = t.trim();
      if(__sdKnown===null){ __sdKnown=t; return; }
      if(t!==__sdKnown){ location.reload(); }
    }).catch(function(){});
  }
  setInterval(__pollSd, 3000);
</script>
)";

static const char INDEX_HTML_1[] PROGMEM = R"(
<div class="layout">
  <div class="maincol">
    <div class="card listcard">
      <div class="btn-row">
        <label class="btn accent" for="up">Upload file</label>
        <input type="file" id="up" multiple style="display:none" onchange="uploadFiles(this.files);">
        <button class="btn" onclick="newFolder();">New folder</button>
        <span id="sdspace" style="margin-left:auto;align-self:center;color:var(--muted);font-size:.8rem;white-space:nowrap;"></span>
      </div>
      <div id="crumb"></div>
      <div class="filewrap">
        <table class="files"><tbody id="flist"></tbody></table>
      </div>
    </div>
  </div>
  <div id="previewcol" class="card">
    <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:.6rem;gap:.3rem;">
      <span id="previewName" style="font-size:.85rem;color:var(--text);word-break:break-all;flex:1;min-width:0;"></span>
      <button class="btn iconbtn" id="prevBtn" onclick="prevPreview();" title="Previous"></button>
      <button class="btn iconbtn" id="nextBtn" onclick="nextPreview();" title="Next"></button>
      <button class="btn iconbtn" id="fsBtn" onclick="toggleFullscreen();" title="Fullscreen"></button>
      <button class="btn iconbtn" style="padding:.35rem .6rem;" onclick="closePreview();" title="Close">&times;</button>
    </div>
    <div id="previewSpinner" class="spinner" style="display:none;margin:2rem auto;"></div>
    <img id="previewImg" alt="">
    <pre id="previewText" style="display:none;max-height:60vh;overflow:auto;white-space:pre-wrap;word-break:break-word;font-size:.75rem;color:var(--text);background:var(--bg);border-radius:.5rem;padding:.6rem;margin:0;"></pre>
  </div>
</div>
</div>
<div id="ctxmenu"></div>
<div id="movebox" onclick="if(event.target===this){ closeMoveDialog(); }">
  <div class="card">
    <p id="movetitle" style="margin:0 0 .6rem;font-size:.9rem;"></p>
    <div id="movepath" style="color:var(--muted);font-size:.8rem;margin:0 0 .6rem;"></div>
    <div id="movelist" style="max-height:40vh;overflow-y:auto;margin-bottom:.8rem;"></div>
    <div class="btn-row">
      <button class="btn accent" onclick="doMove();">Move here</button>
      <button class="btn" onclick="closeMoveDialog();">Cancel</button>
    </div>
  </div>
</div>
<script>
  function dirFromUrl(){
    var m = location.search.match(/[?&]dir=([^&]*)/);
    return m ? decodeURIComponent(m[1]) : '';
  }
  var currentDir = dirFromUrl();
  var FOLDER_ICON_SVG = '<svg viewBox="0 0 512 512" width="14" height="14"><path fill="currentColor" d="M64 448l384 0c35.3 0 64-28.7 64-64l0-240c0-35.3-28.7-64-64-64L298.7 80c-6.9 0-13.7-2.2-19.2-6.4L241.1 44.8C230 36.5 216.5 32 202.7 32L64 32C28.7 32 0 60.7 0 96L0 384c0 35.3 28.7 64 64 64z"/></svg>';
  var XMARK_CIRCLE_SVG = '<svg viewBox="0 0 512 512" width="14" height="14"><path fill="currentColor" d="M256 512a256 256 0 1 0 0-512 256 256 0 1 0 0 512zM167 167c9.4-9.4 24.6-9.4 33.9 0l55 55 55-55c9.4-9.4 24.6-9.4 33.9 0s9.4 24.6 0 33.9l-55 55 55 55c9.4 9.4 9.4 24.6 0 33.9s-24.6 9.4-33.9 0l-55-55-55 55c-9.4 9.4-24.6 9.4-33.9 0s-9.4-24.6 0-33.9l55-55-55-55c-9.4-9.4-9.4-24.6 0-33.9z"/></svg>';
  var CHEVRON_LEFT_SVG = '<svg viewBox="0 0 320 512" width="12" height="12"><path fill="currentColor" d="M9.4 233.4c-12.5 12.5-12.5 32.8 0 45.3l192 192c12.5 12.5 32.8 12.5 45.3 0s12.5-32.8 0-45.3L77.3 256 246.6 86.6c12.5-12.5 12.5-32.8 0-45.3s-32.8-12.5-45.3 0l-192 192z"/></svg>';
  var CHEVRON_RIGHT_SVG = '<svg viewBox="0 0 320 512" width="12" height="12"><path fill="currentColor" d="M311.1 233.4c12.5 12.5 12.5 32.8 0 45.3l-192 192c-12.5 12.5-32.8 12.5-45.3 0s-12.5-32.8 0-45.3L243.2 256 73.9 86.6c-12.5-12.5-12.5-32.8 0-45.3s32.8-12.5 45.3 0l192 192z"/></svg>';
  var EXPAND_SVG = '<svg viewBox="0 0 448 512" width="12" height="12"><path fill="currentColor" d="M32 32C14.3 32 0 46.3 0 64l0 96c0 17.7 14.3 32 32 32s32-14.3 32-32l0-64 64 0c17.7 0 32-14.3 32-32s-14.3-32-32-32L32 32zM64 352c0-17.7-14.3-32-32-32S0 334.3 0 352l0 96c0 17.7 14.3 32 32 32l96 0c17.7 0 32-14.3 32-32s-14.3-32-32-32l-64 0 0-64zM320 32c-17.7 0-32 14.3-32 32s14.3 32 32 32l64 0 0 64c0 17.7 14.3 32 32 32s32-14.3 32-32l0-96c0-17.7-14.3-32-32-32l-96 0zM448 352c0-17.7-14.3-32-32-32s-32 14.3-32 32l0 64-64 0c-17.7 0-32 14.3-32 32s14.3 32 32 32l96 0c17.7 0 32-14.3 32-32l0-96z"/></svg>';
  var COMPRESS_SVG = '<svg viewBox="0 0 448 512" width="12" height="12"><path fill="currentColor" d="M160 64c0-17.7-14.3-32-32-32S96 46.3 96 64l0 64-64 0c-17.7 0-32 14.3-32 32s14.3 32 32 32l96 0c17.7 0 32-14.3 32-32l0-96zM32 320c-17.7 0-32 14.3-32 32s14.3 32 32 32l64 0 0 64c0 17.7 14.3 32 32 32s32-14.3 32-32l0-96c0-17.7-14.3-32-32-32l-96 0zM352 64c0-17.7-14.3-32-32-32s-32 14.3-32 32l0 96c0 17.7 14.3 32 32 32l96 0c17.7 0 32-14.3 32-32s-14.3-32-32-32l-64 0 0-64zM320 320c-17.7 0-32 14.3-32 32l0 96c0 17.7 14.3 32 32 32s32-14.3 32-32l0-64 64 0c17.7 0 32-14.3 32-32s-14.3-32-32-32l-96 0z"/></svg>';
  document.getElementById('prevBtn').innerHTML = CHEVRON_LEFT_SVG;
  document.getElementById('nextBtn').innerHTML = CHEVRON_RIGHT_SVG;
  document.getElementById('fsBtn').innerHTML = EXPAND_SVG;
  document.addEventListener('fullscreenchange', function(){
    document.getElementById('fsBtn').innerHTML = document.fullscreenElement ? COMPRESS_SVG : EXPAND_SVG;
  });
  var previewList = [];
  var previewIndex = -1;
  function fmtSize(n){
    if(n<1024){ return n+' B'; }
    if(n<1024*1024){ return (n/1024).toFixed(1)+' KB'; }
    if(n<1024*1024*1024){ return (n/1024/1024).toFixed(1)+' MB'; }
    return (n/1024/1024/1024).toFixed(2)+' GB';
  }
  function refreshSpace(){
    fetch('/api/sdspace').then(function(r){ return r.text(); }).then(function(t){
      var el=document.getElementById('sdspace');
      if(!el){ return; }
      t=t.trim();
      if(!t){ el.textContent=''; return; }
      var parts=t.split(':');
      var total=parseInt(parts[0],10), free=parseInt(parts[1],10);
      el.textContent = fmtSize(free) + ' free of ' + fmtSize(total);
    }).catch(function(){});
  }
  function navTo(dir){
    currentDir = dir;
    var url = dir ? ('/?dir=' + encodeURIComponent(dir)) : '/';
    history.pushState({dir: dir}, '', url);
    loadFiles();
  }
  window.addEventListener('popstate', function(){
    currentDir = dirFromUrl();
    loadFiles();
  });
  function renderCrumb(){
    var crumb=document.getElementById('crumb');
    crumb.innerHTML='';
    var home=document.createElement('a');
    home.href='#'; home.textContent='Home';
    home.onclick=function(e){ e.preventDefault(); navTo(''); };
    crumb.appendChild(home);
    if(currentDir){
      var acc='';
      currentDir.split('/').forEach(function(p){
        acc = acc ? acc+'/'+p : p;
        crumb.appendChild(document.createTextNode(' / '));
        var a=document.createElement('a');
        a.href='#'; a.textContent=p;
        a.onclick=function(e){ e.preventDefault(); navTo(acc); };
        crumb.appendChild(a);
      });
    }
  }
  function deleteFile(name){
    if(!confirm('Delete ' + name + '?')){ return; }
    fetch('/delete?dir=' + encodeURIComponent(currentDir) + '&name=' + encodeURIComponent(name)).then(function(r){
      return r.text().then(function(t){
        if(r.ok){
          showStatus('Deleted ' + name, 'ok');
          loadFiles();
        }else{
          showStatus('Delete failed: ' + t, 'err');
        }
      });
    }).catch(function(e){ showStatus('Delete error: ' + e.message, 'err'); });
  }
  function renamePrompt(name){
    var newName=prompt('New name:', name);
    if(!newName || newName===name){ return; }
    fetch('/rename?dir=' + encodeURIComponent(currentDir) + '&name=' + encodeURIComponent(name) + '&newName=' + encodeURIComponent(newName)).then(function(r){
      return r.text().then(function(t){
        if(r.ok){
          showStatus('Renamed to ' + newName, 'ok');
          loadFiles();
        }else{
          showStatus('Rename failed: ' + t, 'err');
        }
      });
    }).catch(function(e){ showStatus('Rename error: ' + e.message, 'err'); });
  }
  function newFolder(){
    var name=prompt('Folder name:');
    if(!name){ return; }
    fetch('/mkdir?dir=' + encodeURIComponent(currentDir) + '&name=' + encodeURIComponent(name)).then(function(r){
      return r.text().then(function(t){
        if(r.ok){
          showStatus('Folder created', 'ok');
          loadFiles();
        }else{
          showStatus('Create folder failed: ' + t, 'err');
        }
      });
    }).catch(function(e){ showStatus('Create folder error: ' + e.message, 'err'); });
  }
  function ejectSD(){
    if(!confirm('Eject SD card? Make sure no upload is in progress.')){ return; }
    fetch('/eject').then(function(r){return r.text();}).then(function(t){
      showStatus(t, 'ok');
      setTimeout(function(){ location.reload(); }, 800);
    }).catch(function(e){ showStatus('Eject error: ' + e.message, 'err'); });
  }
  function loadFiles(){
    renderCrumb();
    fetch('/api/filelist?dir=' + encodeURIComponent(currentDir)).then(function(r){return r.text();}).then(function(t){
      var entries=t.split('|').filter(function(s){return s.length>0;}).map(function(e){
        var parts=e.split(':');
        var name=parts[0], size=parseInt(parts[1],10)||0;
        var isDir=false;
        if(name.length && name.charAt(name.length-1)==='/'){
          isDir=true;
          name=name.slice(0,-1);
        }
        return {name:name, size:size, isDir:isDir};
      });
      entries.sort(function(a, b){
        if(a.isDir !== b.isDir){ return a.isDir ? -1 : 1; }
        var aCfg = !a.isDir && a.name === 'config.txt';
        var bCfg = !b.isDir && b.name === 'config.txt';
        if(aCfg !== bCfg){ return aCfg ? 1 : -1; }
        return a.name.localeCompare(b.name, undefined, {sensitivity:'base'});
      });
      previewList = entries.filter(function(e){ return !e.isDir && isPreviewable(e.name); }).map(function(e){ return e.name; });
      var tbody=document.getElementById('flist');
      tbody.innerHTML='';
      entries.forEach(function(entry){
        var name=entry.name, size=entry.size, isDir=entry.isDir;
        var tr=document.createElement('tr');
        var tdName=document.createElement('td');
        var tdSize=document.createElement('td');
        tdSize.className='sz';
        if(!isDir && name === 'config.txt'){
          tdName.className='cfgfile';
        }
        if(isDir){
          var icon=document.createElement('span');
          icon.className='ficon';
          icon.innerHTML=FOLDER_ICON_SVG;
          tdName.appendChild(icon);
          var fl=document.createElement('a');
          fl.className='pvlink'; fl.href='#'; fl.textContent=name;
          fl.onclick=function(e){ e.preventDefault(); navTo(currentDir ? currentDir+'/'+name : name); };
          tdName.appendChild(fl);
          tdSize.textContent='Folder';
          tr.className='pvrow';
        }else{
          if(isPreviewable(name)){
            var pv=document.createElement('a');
            pv.className='pvlink'; pv.href='#'; pv.textContent=name;
            pv.onclick=function(e){ e.preventDefault(); showPreview(name); };
            tdName.appendChild(pv);
            tr.className='pvrow';
          }else{
            tdName.textContent=name;
          }
          tdSize.textContent=fmtSize(size);
        }
        tr.oncontextmenu=function(e){ e.preventDefault(); showCtxMenu(e.clientX, e.clientY, name, isDir); };
        var tdMenu=document.createElement('td');
        tdMenu.className='rowmenu';
        var mb=document.createElement('button');
        mb.className='ellipsisbtn'; mb.textContent='...'; mb.title='Menu';
        mb.onclick=function(e){
          e.preventDefault(); e.stopPropagation();
          var r=mb.getBoundingClientRect();
          showCtxMenu(r.left, r.bottom, name, isDir);
        };
        tdMenu.appendChild(mb);
        tr.appendChild(tdName); tr.appendChild(tdSize); tr.appendChild(tdMenu);
        tbody.appendChild(tr);
      });
      if(!entries.length){
        tbody.innerHTML='<tr><td colspan="3" style="color:var(--muted);">No files.</td></tr>';
      }
    });
  }
  function downloadFile(name){
    window.location = '/download?dir=' + encodeURIComponent(currentDir) + '&name=' + encodeURIComponent(name);
  }
  var IMAGE_EXTS = ['jpg','jpeg','png','gif','svg','webp','bmp','ico'];
  var TEXT_EXTS = ['txt','log','csv','json','md','ini','cfg','yaml','yml','xml'];
  function extOf(name){
    var dot = name.lastIndexOf('.');
    return dot < 0 ? '' : name.slice(dot+1).toLowerCase();
  }
  function previewKind(name){
    var ext = extOf(name);
    if(IMAGE_EXTS.indexOf(ext) >= 0){ return 'image'; }
    if(TEXT_EXTS.indexOf(ext) >= 0){ return 'text'; }
    return null;
  }
  function isPreviewable(name){
    return previewKind(name) !== null;
  }
  function showPreview(name){
    previewIndex = previewList.indexOf(name);
    renderPreview();
  }
  function nextPreview(){
    if(!previewList.length){ return; }
    previewIndex = (previewIndex + 1) % previewList.length;
    renderPreview();
  }
  function prevPreview(){
    if(!previewList.length){ return; }
    previewIndex = (previewIndex - 1 + previewList.length) % previewList.length;
    renderPreview();
  }
  function renderPreview(){
    if(previewIndex < 0 || previewIndex >= previewList.length){ return; }
    var name = previewList[previewIndex];
    var img = document.getElementById('previewImg');
    var txt = document.getElementById('previewText');
    document.getElementById('previewName').textContent = name;
    document.getElementById('previewcol').style.display = 'block';
    if(previewKind(name) === 'image'){
      txt.style.display = 'none';
      txt.textContent = '';
      img.style.display = 'none';
      var spinner = document.getElementById('previewSpinner');
      spinner.style.display = 'block';
      img.onload = function(){ spinner.style.display = 'none'; img.style.display = 'block'; };
      img.onerror = function(){ spinner.style.display = 'none'; img.style.display = 'block'; };
      img.src = '/download?dir=' + encodeURIComponent(currentDir) + '&name=' + encodeURIComponent(name);
    }else{
      img.style.display = 'none';
      img.src = '';
      txt.style.display = 'block';
      txt.textContent = 'Loading...';
      fetch('/preview?dir=' + encodeURIComponent(currentDir) + '&name=' + encodeURIComponent(name)).then(function(r){return r.text();}).then(function(t){
        txt.textContent = t;
      }).catch(function(e){ txt.textContent = 'Preview error: ' + e.message; });
    }
  }
  function closePreview(){
    document.getElementById('previewcol').style.display = 'none';
    document.getElementById('previewImg').src = '';
  }
  function toggleFullscreen(){
    if(document.fullscreenElement){
      document.exitFullscreen();
      return;
    }
    document.getElementById('previewcol').requestFullscreen();
  }
  function hideCtxMenu(){
    document.getElementById('ctxmenu').style.display='none';
  }
  function showCtxMenu(x, y, name, isDir){
    var menu=document.getElementById('ctxmenu');
    menu.innerHTML='';
    if(!isDir){
      var dl=document.createElement('button');
      dl.textContent='Download';
      dl.onclick=function(){ hideCtxMenu(); downloadFile(name); };
      menu.appendChild(dl);
      if(isPreviewable(name)){
        var pv=document.createElement('button');
        pv.textContent='Preview';
        pv.onclick=function(){ hideCtxMenu(); showPreview(name); };
        menu.appendChild(pv);
      }
      var mv=document.createElement('button');
      mv.textContent='Move';
      mv.onclick=function(){ hideCtxMenu(); showMoveDialog(name); };
      menu.appendChild(mv);
    }
    var rn=document.createElement('button');
    rn.textContent='Rename';
    rn.onclick=function(){ hideCtxMenu(); renamePrompt(name); };
    menu.appendChild(rn);
    var del=document.createElement('button');
    del.className='danger'; del.textContent='Delete';
    del.onclick=function(){ hideCtxMenu(); deleteFile(name); };
    menu.appendChild(del);
    menu.style.left=x+'px';
    menu.style.top=y+'px';
    menu.style.display='block';
    var rect=menu.getBoundingClientRect();
    if(rect.right>window.innerWidth){ menu.style.left=Math.max(0, window.innerWidth-rect.width)+'px'; }
    if(rect.bottom>window.innerHeight){ menu.style.top=Math.max(0, window.innerHeight-rect.height)+'px'; }
  }
  document.addEventListener('click', hideCtxMenu);
  var moveSrcDir='', moveName='', browseDir='';
  function showMoveDialog(name){
    moveSrcDir = currentDir;
    moveName = name;
    browseDir = currentDir;
    document.getElementById('movetitle').textContent = "Move '" + name + "' to:";
    document.getElementById('movebox').style.display = 'block';
    loadMoveDirList();
  }
  function closeMoveDialog(){
    document.getElementById('movebox').style.display = 'none';
  }
  function renderMoveCrumb(){
    var el=document.getElementById('movepath');
    el.innerHTML='';
    var home=document.createElement('a');
    home.href='#'; home.textContent='Home';
    home.onclick=function(e){ e.preventDefault(); browseDir=''; loadMoveDirList(); };
    el.appendChild(home);
    if(browseDir){
      var acc='';
      browseDir.split('/').forEach(function(p){
        acc = acc ? acc+'/'+p : p;
        el.appendChild(document.createTextNode(' / '));
        var a=document.createElement('a');
        a.href='#'; a.textContent=p;
        a.onclick=function(e){ e.preventDefault(); browseDir=acc; loadMoveDirList(); };
        el.appendChild(a);
      });
    }
  }
  function loadMoveDirList(){
    renderMoveCrumb();
    fetch('/api/filelist?dir=' + encodeURIComponent(browseDir)).then(function(r){return r.text();}).then(function(t){
      var entries=t.split('|').filter(function(s){return s.length>0;});
      var list=document.getElementById('movelist');
      list.innerHTML='';
      var folders=entries.filter(function(e){
        var n=e.split(':')[0];
        return n.length && n.charAt(n.length-1)==='/';
      }).map(function(e){ return e.split(':')[0].slice(0,-1); });
      if(!folders.length){
        list.innerHTML='<div style="color:var(--muted);font-size:.8rem;padding:.4rem;">No subfolders here.</div>';
        return;
      }
      folders.forEach(function(n){
        var row=document.createElement('div');
        row.className='mvrow'; row.textContent=n;
        row.onclick=function(){ browseDir = browseDir ? browseDir+'/'+n : n; loadMoveDirList(); };
        list.appendChild(row);
      });
    });
  }
  function doMove(){
    fetch('/move?dir=' + encodeURIComponent(moveSrcDir) + '&name=' + encodeURIComponent(moveName) + '&destDir=' + encodeURIComponent(browseDir)).then(function(r){
      return r.text().then(function(t){
        closeMoveDialog();
        if(r.ok){
          showStatus('Moved ' + moveName, 'ok');
          loadFiles();
        }else{
          showStatus('Move failed: ' + t, 'err');
        }
      });
    }).catch(function(e){ closeMoveDialog(); showStatus('Move error: ' + e.message, 'err'); });
  }
  function fmtSpeed(bps){
    if(bps<1024){ return bps.toFixed(0)+' B/s'; }
    if(bps<1024*1024){ return (bps/1024).toFixed(1)+' KB/s'; }
    return (bps/1024/1024).toFixed(1)+' MB/s';
  }
  function fmtEta(sec){
    if(!isFinite(sec) || sec <= 0){ return ''; }
    if(sec < 60){ return '<1 min left'; }
    return Math.ceil(sec/60) + ' min left';
  }
  var currentUploadXhr = null;
  var uploadStopped = false;
  function stopUpload(){
    uploadStopped = true;
    if(currentUploadXhr){ currentUploadXhr.abort(); }
    var ov=document.createElement('div');
    ov.className='loadoverlay';
    ov.innerHTML='<div class="spinner"></div>';
    document.body.appendChild(ov);
    // A background fetch to refresh the list can race the device still
    // noticing/cleaning up the aborted upload; a full reload just waits
    // naturally for the server instead of silently failing.
    location.reload();
  }
  function uploadOne(file, label, row, batch){
    return new Promise(function(resolve){
      showStatus(label + 'uploading ' + file.name + '...', 'info');
      var xhr=new XMLHttpRequest();
      currentUploadXhr = xhr;
      xhr.open('POST', '/upload');
      xhr.setRequestHeader('Content-Type', 'application/octet-stream');
      xhr.setRequestHeader('X-Dir', encodeURIComponent(currentDir));
      xhr.setRequestHeader('X-Name', encodeURIComponent(file.name));
      var lastLoaded=0, lastTime=Date.now();
      xhr.upload.onprogress=function(e){
        if(!e.lengthComputable){ return; }
        var now=Date.now(), dt=(now-lastTime)/1000;
        if(dt<0.15){ return; }
        var speed=(e.loaded-lastLoaded)/dt;
        lastLoaded=e.loaded; lastTime=now;
        var pct=Math.floor(e.loaded/e.total*100);
        var remaining=batch.totalBytes - (batch.doneBytes + e.loaded);
        var eta=speed>0 ? fmtEta(remaining/speed) : '';
        if(row){ row.sizeCell.textContent = pct + '%' + (eta ? ' - ' + eta : ''); }
        showStatus(label + file.name + ': ' + pct + '% @ ' + fmtSpeed(speed) + (eta ? ', ' + eta : ''), 'info');
      };
      xhr.onload=function(){
        currentUploadXhr = null;
        if(xhr.status>=200 && xhr.status<300){ resolve('ok'); }
        else{ showStatus('Upload failed: ' + xhr.responseText, 'err'); resolve('err'); }
      };
      xhr.onerror=function(){ currentUploadXhr = null; showStatus('Upload error', 'err'); resolve('err'); };
      xhr.onabort=function(){ currentUploadXhr = null; resolve('stopped'); };
      xhr.send(file);
    });
  }
  function uploadFiles(list){
    if(!list || !list.length){ return; }
    var files=Array.prototype.slice.call(list);
    uploadStopped = false;
    var batch = {totalBytes: files.reduce(function(s,f){ return s+f.size; }, 0), doneBytes: 0};
    var ok=0, idx=0;
    var tbody=document.getElementById('flist');
    var rows=files.map(function(f){
      var tr=document.createElement('tr');
      var tdName=document.createElement('td'); tdName.textContent=f.name;
      var tdSize=document.createElement('td'); tdSize.className='sz'; tdSize.textContent='Queued';
      var tdMenu=document.createElement('td'); tdMenu.className='rowmenu';
      var ab=document.createElement('button');
      ab.className='ellipsisbtn'; ab.innerHTML=XMARK_CIRCLE_SVG; ab.title='Abort upload';
      ab.onclick=function(e){ e.preventDefault(); e.stopPropagation(); stopUpload(); };
      tdMenu.appendChild(ab);
      tr.appendChild(tdName); tr.appendChild(tdSize); tr.appendChild(tdMenu);
      tbody.insertBefore(tr, tbody.firstChild);
      return {sizeCell: tdSize};
    });
    function next(){
      if(uploadStopped || idx>=files.length){
        showStatus(uploadStopped ? 'Upload stopped' : ('Uploaded ' + ok + ' of ' + files.length + ' file(s)'),
          uploadStopped ? 'info' : (ok ? 'ok' : 'err'));
        loadFiles();
        refreshSpace();
        return;
      }
      var f=files[idx], row=rows[idx];
      idx++;
      var label=files.length>1 ? ('['+idx+'/'+files.length+'] ') : '';
      uploadOne(f, label, row, batch).then(function(result){
        if(result==='ok'){ ok++; batch.doneBytes += f.size; }
        next();
      });
    }
    next();
  }
  loadFiles();
  refreshSpace();
</script>
)";

static const char APLIST_HTML_0[] PROGMEM = R"(
<style>
  .c{text-align: center;}
  input{width:95%;padding:5px;font-size:1em;text-align:center;}
  body{text-align: left;}
  button{display:block;margin:1rem auto 0;border:0;border-radius:0.55rem;color:#fff;line-height:1;font-size:1rem;padding:0.6rem 1.6rem;background-color:var(--accent);}
  button:hover{background-color:var(--accent-hover);}
  .q{float: right;width: 64px;text-align: right;}
  .radio{width:2em;}
  #vm{width:100%;height:50vh;overflow-y:auto;margin-bottom:1em;}
  #pbar{width:100%;height:10px;background:var(--btn);border-radius:5px;overflow:hidden;margin-bottom:1em;}
  #pfill{height:100%;width:0;background:#1fa3ec;transition:width 10s linear;}
</style>
  <div class="contain">
    <div class="center_div">
)";

static const char APLIST_HTML_1[] PROGMEM = R"(
      <h1 id='ttl'>Scanning...</h1>
      <div id='pbar'><div id='pfill'></div></div>
      <div id='vm'>
)";

static const char APLIST_HTML_2[] PROGMEM = R"(
      </div>
      <form method='get' action='wifisave'>
        <input id='s' name='s' length=32 placeholder='SSID (Leave blank for AP mode)'><br>
        <input id='p' name='p' length=32 placeholder='Password'><br>
        <br><button type='submit'>Save</button>
      </form>
      <div style="margin-top:2rem;color:var(--muted);font-size:.85rem;">
        WiFi signal: <span id="rssi">-</span> dBm
      </div>
      <div style="margin-top:1rem;">
        <label for="bl" style="display:block;margin-bottom:.4rem;">Screen illumination: <span id="blval"></span>%</label>
        <input type="range" id="bl" min="0" max="100" step="1" value=")";

static const char APLIST_HTML_3[] PROGMEM = R"(" oninput="document.getElementById('blval').textContent=this.value;" onchange="setBrightness(this.value);">
      </div>
     </div>
  </div>
<script>
  // The device starts a WiFi scan when this page is served; the progress bar
  // approximates the ~10s scan, then results are fetched (no live connection).
  function c(l){
    document.getElementById('s').value=l.innerText||l.textContent;
    document.getElementById('p').focus();
  }
  function done(){ document.getElementById('pbar').style.display='none'; }
  function load(tries){
    fetch('/aplist').then(function(r){return r.text();}).then(function(t){
      var list=t.split('|').filter(function(s){return s.length>0;});
      var vm=document.getElementById('vm');
      if(!list.length){
        if(tries>0){ setTimeout(function(){ load(tries-1); }, 2000); return; }
        done(); document.getElementById('ttl').innerHTML='No networks found.'; vm.innerHTML='';
        return;
      }
      done(); document.getElementById('ttl').innerHTML='Networks found:';
      var html='';
      for(var i=0;i<list.length;i++){
        html+='<span>'+(i+1)+": </span><a href='#p' onclick='c(this)'>" + list[i] + '</a><br>';
      }
      vm.innerHTML=html;
    }).catch(function(){
      if(tries>0){ setTimeout(function(){ load(tries-1); }, 2000); } else { done(); }
    });
  }
  function setBrightness(v){
    fetch('/brightness?value=' + encodeURIComponent(v));
  }
  function pollRssi(){
    fetch('/api/rssi').then(function(r){ return r.text(); }).then(function(t){
      document.getElementById('rssi').textContent = t.trim();
    }).catch(function(){});
  }
  pollRssi();
  setInterval(pollRssi, 2000);
  document.getElementById('blval').textContent = document.getElementById('bl').value;
  setTimeout(function(){ document.getElementById('pfill').style.width='100%'; }, 50); // animate bar
  setTimeout(function(){ load(3); }, 10000);   // fetch after the bar fills, then retry
</script>
)";

static const char REDIRECT_HTML[] PROGMEM = R"(
<p id="tmr"></p>
<script>
  var c=6;
  function count(){
    var tmr=document.getElementById('tmr');
    if(c>0){
      c--;
      tmr.innerHTML="You will be redirected to home page in "+c+" seconds.";
      setTimeout('count()',1000);
    }else{
      window.location.href="/";
    }
  }
  count();
</script>
)";

static const char API_HTML_0[] PROGMEM = R"(
<style>
  h1{ font-size:1.6rem; margin:0 0 .2rem; text-align:center; }
  h2{ font-size:1rem; color:var(--text); margin:1.4rem 0 .6rem; }
  .sub{ color:var(--muted); font-size:.8rem; text-align:center; margin:0 0 1.2rem; }
  table{ width:100%; border-collapse:collapse; font-size:.82rem; }
  th,td{ text-align:left; padding:.5rem .6rem; border-bottom:1px solid var(--border); vertical-align:top; }
  th{ color:var(--muted); font-weight:600; }
  code{ background:var(--card-bg); border:1px solid var(--border); border-radius:.3rem; padding:.05rem .35rem;
    font-size:.78rem; color:#ff9c85; white-space:nowrap; }
  .m{ color:#7ee29a; font-weight:600; }
</style>
<div class="contain"><div class="center_div">
)";

static const char API_HTML_1[] PROGMEM = R"(
  <h1>HTTP API</h1>
  <p class="sub">Device IP, port 80. Action endpoints return <code>OK</code> or <code>400</code>.</p>

  <h2>File storage</h2>
  <p class="sub">All endpoints below take an optional <code>dir</code> query param (relative folder path, e.g. <code>Photos/2024</code>; omit or leave empty for root).</p>
  <table>
    <tr><th>Method</th><th>Endpoint</th><th>Description</th></tr>
    <tr><td class="m">GET</td><td><code>/api/filelist?dir=DIR</code></td><td>List files as <code>name:size</code> and folders as <code>name/:0</code> (| separated)</td></tr>
    <tr><td class="m">GET</td><td><code>/download?dir=DIR&amp;name=NAME</code></td><td>Download a file from the SD card</td></tr>
    <tr><td class="m">GET</td><td><code>/preview?dir=DIR&amp;name=NAME</code></td><td>Read up to the first 8KB of a file as plain text (used by the UI's text preview)</td></tr>
    <tr><td class="m">POST</td><td><code>/upload?dir=DIR</code></td><td>Upload a file (multipart form field <code>f</code>)</td></tr>
    <tr><td class="m">GET</td><td><code>/delete?dir=DIR&amp;name=NAME</code></td><td>Delete a file, or a folder and everything inside it</td></tr>
    <tr><td class="m">GET</td><td><code>/mkdir?dir=DIR&amp;name=NAME</code></td><td>Create a subfolder</td></tr>
    <tr><td class="m">GET</td><td><code>/move?dir=DIR&amp;name=NAME&amp;destDir=DEST</code></td><td>Move a file into another folder; fails if a same-named file already exists there</td></tr>
    <tr><td class="m">GET</td><td><code>/rename?dir=DIR&amp;name=NAME&amp;newName=NEW</code></td><td>Rename a file or folder; fails if NEW is already taken</td></tr>
  </table>

  <h2>Web UI &amp; config</h2>
  <p class="sub">Used by the built-in web pages; not part of the automation surface.</p>
  <table>
    <tr><th>Method</th><th>Endpoint</th><th>Description</th></tr>
    <tr><td class="m">GET</td><td><code>/aplist</code></td><td>Last WiFi scan result (empty until ready)</td></tr>
    <tr><td class="m">GET</td><td><code>/wifisave?s=SSID&amp;p=PASS</code></td><td>Save WiFi credentials, switch to station mode</td></tr>
    <tr><td class="m">GET</td><td><code>/</code></td><td>Main web page</td></tr>
    <tr><td class="m">GET</td><td><code>/config</code></td><td>WiFi &amp; screen brightness config page (starts an async scan)</td></tr>
    <tr><td class="m">GET</td><td><code>/brightness?value=PERCENT</code></td><td>Set screen illumination (0-100; actual duty is capped at 50% in hardware)</td></tr>
    <tr><td class="m">GET</td><td><code>/api</code></td><td>This documentation page</td></tr>
    <tr><td class="m">GET</td><td><code>/api/sdstatus</code></td><td>Returns <code>ready</code>, <code>faulty</code> (present, unreadable filesystem) or <code>missing</code></td></tr>
    <tr><td class="m">GET</td><td><code>/eject</code></td><td>Finish pending writes and unmount the SD card for safe removal</td></tr>
    <tr><td class="m">GET</td><td><code>/format</code></td><td>Erase the SD card and lay down a fresh filesystem</td></tr>
  </table>
  <p class="sub">The page reads the file list on load and after each action.</p>
</div></div>
)";

#endif
