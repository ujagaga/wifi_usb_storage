#line 1 "/home/rada/Projects/wifi_usb_storage/http_ui.h"
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
    <style>
      body { background:#11151c; color:#e6e9ef; font-family: system-ui, Arial, Helvetica, sans-serif; margin:0; }
      a{ color:#ff7a5c; }
      .contain{ width:100%; }
      .center_div{ margin:0 auto; width:92%; max-width:800px; position:relative; padding:1.2rem 0 4.5rem; }
    </style>
  </head>
  <body>
)";

static const char HTML_END[] PROGMEM = "</body></html>";

// Shared top navigation bar. Inserted right after each page's container opens.
static const char NAV_HTML[] PROGMEM = R"NAV(
<style>
  .nav{ display:flex; align-items:center; gap:.5rem; flex-wrap:wrap; margin-bottom:1.2rem; }
  .nav a{ border-radius:.5rem; background:#2a3340; color:#e6e9ef;
    font-size:.85rem; line-height:1; padding:.5rem .8rem; text-decoration:none; }
  .nav a:hover{ background:#37424f; }
  .nav a.active{ background:#ff5a3c; color:#fff; }
</style>
<div class="nav">
  <a href="/">Home</a>
  <a href="/selectap">WiFi</a>
  <a href="/api">API docs</a>
</div>
<script>(function(){var p=location.pathname,l=document.querySelectorAll('.nav a');
  for(var i=0;i<l.length;i++){ if(l[i].getAttribute('href')===p){ l[i].className='active'; } }})();</script>
)NAV";

static const char INDEX_HTML_0[] PROGMEM = R"(
<style>
  h1{ font-weight:600; font-size:1.7rem; letter-spacing:.5px; margin:0 0 .2rem; text-align:center; }
  .ip{ color:#8b93a3; font-size:.8rem; text-align:center; margin:0 0 1.2rem; }
  .card{ background:#1b212b; border:1px solid #232b37; border-radius:.9rem; padding:1.1rem; margin-bottom:1.1rem; }
  .btn-row{ display:flex; flex-wrap:wrap; gap:.5rem; margin-top:.6rem; }
  .btn{ border:0; border-radius:.55rem; color:#fff; background:#2a3340; font-size:.85rem;
    padding:.6rem 1rem; cursor:pointer; transition:background .15s; }
  .btn:hover{ background:#37424f; }
  .btn.accent{ background:#ff5a3c; }
  .btn.accent:hover{ background:#ff6f55; }
  .ghead{ margin:0 0 .8rem; font-size:.95rem; color:#cfd4de; }
  #crumb{ background:#11151c; border-radius:.55rem; padding:.6rem .8rem; margin-bottom:.8rem;
    font-size:.85rem; color:#9db4d0; }
  #status{ position:fixed; left:50%; transform:translateX(-50%); bottom:1rem; z-index:50; max-width:90%;
    padding:.65rem 1.1rem; border-radius:.6rem; font-size:.85rem; display:none;
    box-shadow:0 6px 18px rgba(0,0,0,.45); }
  #status.ok{ display:block; background:#16361f; color:#7ee29a; border:1px solid #2f6b41; }
  #status.err{ display:block; background:#3a1a1d; color:#ff8b8b; border:1px solid #7a2d33; }
  #status.info{ display:block; background:#1e2733; color:#9db4d0; border:1px solid #33485f; }
  table.files{ width:100%; border-collapse:collapse; font-size:.85rem; }
  table.files th, table.files td{ text-align:left; padding:.5rem .4rem; border-bottom:1px solid #232b37; }
  table.files th{ color:#8b93a3; font-weight:600; }
  table.files .ficon{ display:inline-block; vertical-align:-2px; margin-right:.4rem; color:#e0b357; }
  table.files td.sz{ color:#8b93a3; white-space:nowrap; }
  #ctxmenu{ position:fixed; display:none; background:#1b212b; border:1px solid #232b37;
    border-radius:.5rem; box-shadow:0 6px 18px rgba(0,0,0,.45); z-index:60; min-width:140px; padding:.3rem; }
  #ctxmenu button{ display:block; width:100%; text-align:left; border:0; background:none; color:#e6e9ef;
    font-size:.85rem; padding:.5rem .7rem; border-radius:.35rem; cursor:pointer; }
  #ctxmenu button:hover{ background:#2a3340; }
  #ctxmenu button.danger{ color:#ff5a3c; }
  #ctxmenu button.danger:hover{ background:#ff5a3c; color:#fff; }
  #movebox{ display:none; position:fixed; inset:0; background:rgba(0,0,0,.5); z-index:70; }
  #movebox .card{ max-width:360px; margin:10vh auto; }
  .mvrow{ padding:.45rem .3rem; cursor:pointer; border-radius:.35rem; font-size:.85rem; }
  .mvrow:hover{ background:#2a3340; }
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
<div id="status"></div>
<script>
  function showStatus(msg, kind){
    var s=document.getElementById('status');
    s.textContent=msg;
    s.className=kind||'info';
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
  <div class="card">
    <div class="btn-row">
      <label class="btn accent" for="up">Upload file</label>
      <input type="file" id="up" multiple style="display:none" onchange="uploadFiles(this.files);">
      <button class="btn" onclick="newFolder();">New folder</button>
      <button class="btn" onclick="ejectSD();">Eject SD card</button>
      <button class="btn" onclick="formatSD();">Format SD card</button>
    </div>
  </div>
  <div class="card">
    <div id="crumb"></div>
    <table class="files"><tbody id="flist"></tbody></table>
  </div>
</div>
<div id="ctxmenu"></div>
<div id="movebox" onclick="if(event.target===this){ closeMoveDialog(); }">
  <div class="card">
    <p id="movetitle" style="margin:0 0 .6rem;font-size:.9rem;"></p>
    <div id="movepath" style="color:#9db4d0;font-size:.8rem;margin:0 0 .6rem;"></div>
    <div id="movelist" style="max-height:40vh;overflow-y:auto;margin-bottom:.8rem;"></div>
    <div class="btn-row">
      <button class="btn accent" onclick="doMove();">Move here</button>
      <button class="btn" onclick="closeMoveDialog();">Cancel</button>
    </div>
  </div>
</div>
<script>
  var currentDir = '';
  var FOLDER_ICON_SVG = '<svg viewBox="0 0 512 512" width="14" height="14"><path fill="currentColor" d="M64 448l384 0c35.3 0 64-28.7 64-64l0-240c0-35.3-28.7-64-64-64L298.7 80c-6.9 0-13.7-2.2-19.2-6.4L241.1 44.8C230 36.5 216.5 32 202.7 32L64 32C28.7 32 0 60.7 0 96L0 384c0 35.3 28.7 64 64 64z"/></svg>';
  function fmtSize(n){
    if(n<1024){ return n+' B'; }
    if(n<1024*1024){ return (n/1024).toFixed(1)+' KB'; }
    return (n/1024/1024).toFixed(1)+' MB';
  }
  function navTo(dir){
    currentDir = dir;
    loadFiles();
  }
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
        return a.name.localeCompare(b.name, undefined, {sensitivity:'base'});
      });
      var tbody=document.getElementById('flist');
      tbody.innerHTML='';
      entries.forEach(function(entry){
        var name=entry.name, size=entry.size, isDir=entry.isDir;
        var tr=document.createElement('tr');
        var tdName=document.createElement('td');
        var tdSize=document.createElement('td');
        tdSize.className='sz';
        if(isDir){
          var icon=document.createElement('span');
          icon.className='ficon';
          icon.innerHTML=FOLDER_ICON_SVG;
          tdName.appendChild(icon);
          var fl=document.createElement('a');
          fl.href='#'; fl.textContent=name;
          fl.onclick=function(e){ e.preventDefault(); navTo(currentDir ? currentDir+'/'+name : name); };
          tdName.appendChild(fl);
          tdSize.textContent='Folder';
        }else{
          tdName.textContent=name;
          tdSize.textContent=fmtSize(size);
        }
        tr.oncontextmenu=function(e){ e.preventDefault(); showCtxMenu(e.clientX, e.clientY, name, isDir); };
        tr.appendChild(tdName); tr.appendChild(tdSize);
        tbody.appendChild(tr);
      });
      if(!entries.length){
        tbody.innerHTML='<tr><td colspan="2" style="color:#8b93a3;">No files.</td></tr>';
      }
    });
  }
  function downloadFile(name){
    window.location = '/download?dir=' + encodeURIComponent(currentDir) + '&name=' + encodeURIComponent(name);
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
        list.innerHTML='<div style="color:#8b93a3;font-size:.8rem;padding:.4rem;">No subfolders here.</div>';
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
  function uploadOne(file, label){
    return new Promise(function(resolve){
      var fd=new FormData();
      fd.append('f', file, file.name);
      showStatus(label + 'uploading ' + file.name + '...', 'info');
      fetch('/upload?dir=' + encodeURIComponent(currentDir), {method:'POST', body:fd}).then(function(rsp){
        return rsp.text().then(function(t){
          if(rsp.ok){ resolve(true); }
          else{ showStatus('Upload failed: ' + t, 'err'); resolve(false); }
        });
      }).catch(function(e){ showStatus('Upload error: ' + e.message, 'err'); resolve(false); });
    });
  }
  function uploadFiles(list){
    if(!list || !list.length){ return; }
    var files=Array.prototype.slice.call(list);
    var ok=0, idx=0;
    function next(){
      if(idx>=files.length){
        showStatus('Uploaded ' + ok + ' of ' + files.length + ' file(s)', ok ? 'ok' : 'err');
        loadFiles();
        return;
      }
      var f=files[idx++];
      var label=files.length>1 ? ('['+idx+'/'+files.length+'] ') : '';
      uploadOne(f, label).then(function(good){ if(good){ ok++; } next(); });
    }
    next();
  }
  loadFiles();
</script>
)";

static const char APLIST_HTML_0[] PROGMEM = R"(
<style>
  .c{text-align: center;}
  input{width:95%;padding:5px;font-size:1em;text-align:center;}
  body{text-align: left;}
  button{display:block;margin:1rem auto 0;border:0;border-radius:0.55rem;color:#fff;line-height:1;font-size:1rem;padding:0.6rem 1.6rem;background-color:#ff5a3c;}
  button:hover{background-color:#ff6f55;}
  .q{float: right;width: 64px;text-align: right;}
  .radio{width:2em;}
  #vm{width:100%;height:50vh;overflow-y:auto;margin-bottom:1em;}
  #pbar{width:100%;height:10px;background:#2a3340;border-radius:5px;overflow:hidden;margin-bottom:1em;}
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
  h2{ font-size:1rem; color:#cfd4de; margin:1.4rem 0 .6rem; }
  .sub{ color:#8b93a3; font-size:.8rem; text-align:center; margin:0 0 1.2rem; }
  table{ width:100%; border-collapse:collapse; font-size:.82rem; }
  th,td{ text-align:left; padding:.5rem .6rem; border-bottom:1px solid #232b37; vertical-align:top; }
  th{ color:#8b93a3; font-weight:600; }
  code{ background:#1b212b; border:1px solid #232b37; border-radius:.3rem; padding:.05rem .35rem;
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
    <tr><td class="m">GET</td><td><code>/selectap</code></td><td>WiFi config page (starts an async scan)</td></tr>
    <tr><td class="m">GET</td><td><code>/api</code></td><td>This documentation page</td></tr>
    <tr><td class="m">GET</td><td><code>/api/sdstatus</code></td><td>Returns <code>ready</code>, <code>faulty</code> (present, unreadable filesystem) or <code>missing</code></td></tr>
    <tr><td class="m">GET</td><td><code>/eject</code></td><td>Finish pending writes and unmount the SD card for safe removal</td></tr>
    <tr><td class="m">GET</td><td><code>/format</code></td><td>Erase the SD card and lay down a fresh filesystem</td></tr>
  </table>
  <p class="sub">The page reads the file list on load and after each action.</p>
</div></div>
)";

#endif
