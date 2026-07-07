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
  #status{ position:fixed; left:50%; transform:translateX(-50%); bottom:1rem; z-index:50; max-width:90%;
    padding:.65rem 1.1rem; border-radius:.6rem; font-size:.85rem; display:none;
    box-shadow:0 6px 18px rgba(0,0,0,.45); }
  #status.ok{ display:block; background:#16361f; color:#7ee29a; border:1px solid #2f6b41; }
  #status.err{ display:block; background:#3a1a1d; color:#ff8b8b; border:1px solid #7a2d33; }
  #status.info{ display:block; background:#1e2733; color:#9db4d0; border:1px solid #33485f; }
  table.files{ width:100%; border-collapse:collapse; font-size:.85rem; }
  table.files th, table.files td{ text-align:left; padding:.5rem .4rem; border-bottom:1px solid #232b37; }
  table.files th{ color:#8b93a3; font-weight:600; }
  table.files td.sz{ color:#8b93a3; white-space:nowrap; }
  table.files td.act{ white-space:nowrap; text-align:right; }
  table.files a.dl{ margin-right:.6rem; }
  .del{ border:1px solid #ff5a3c; border-radius:.4rem; background:none; color:#ff5a3c;
    font-size:.8rem; padding:.2rem .5rem; cursor:pointer; }
  .del:hover{ background:#ff5a3c; color:#fff; }
</style>
<div class="contain">
  <div class="center_div">
)";

static const char INDEX_HTML_1[] PROGMEM = R"(
  <div class="card">
    <div class="btn-row">
      <label class="btn accent" for="up">Upload file</label>
      <input type="file" id="up" multiple style="display:none" onchange="uploadFiles(this.files);">
    </div>
  </div>
  <div class="card">
    <p class="ghead">Files on SD card</p>
    <table class="files"><tbody id="flist"></tbody></table>
  </div>
</div>
<div id="status"></div>
<script>
  function showStatus(msg, kind){
    var s=document.getElementById('status');
    s.textContent=msg;
    s.className=kind||'info';
  }
  function fmtSize(n){
    if(n<1024){ return n+' B'; }
    if(n<1024*1024){ return (n/1024).toFixed(1)+' KB'; }
    return (n/1024/1024).toFixed(1)+' MB';
  }
  function deleteFile(name){
    if(!confirm('Delete ' + name + '?')){ return; }
    fetch('/delete?name=' + encodeURIComponent(name)).then(function(r){
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
  function loadFiles(){
    fetch('/api/filelist').then(function(r){return r.text();}).then(function(t){
      var entries=t.split('|').filter(function(s){return s.length>0;});
      var tbody=document.getElementById('flist');
      tbody.innerHTML='';
      entries.forEach(function(e){
        var parts=e.split(':');
        var name=parts[0], size=parseInt(parts[1],10)||0;
        var tr=document.createElement('tr');
        var tdName=document.createElement('td');
        tdName.textContent=name;
        var tdSize=document.createElement('td');
        tdSize.className='sz';
        tdSize.textContent=fmtSize(size);
        var tdAct=document.createElement('td');
        tdAct.className='act';
        var dl=document.createElement('a');
        dl.className='dl'; dl.textContent='Download';
        dl.href='/download?name=' + encodeURIComponent(name);
        var del=document.createElement('button');
        del.className='del'; del.textContent='Delete';
        del.onclick=function(){ deleteFile(name); };
        tdAct.appendChild(dl); tdAct.appendChild(del);
        tr.appendChild(tdName); tr.appendChild(tdSize); tr.appendChild(tdAct);
        tbody.appendChild(tr);
      });
      if(!entries.length){
        tbody.innerHTML='<tr><td colspan="3" style="color:#8b93a3;">No files.</td></tr>';
      }
    });
  }
  function uploadOne(file, label){
    return new Promise(function(resolve){
      var fd=new FormData();
      fd.append('f', file, file.name);
      showStatus(label + 'uploading ' + file.name + '...', 'info');
      fetch('/upload', {method:'POST', body:fd}).then(function(rsp){
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
  <table>
    <tr><th>Method</th><th>Endpoint</th><th>Description</th></tr>
    <tr><td class="m">GET</td><td><code>/api/filelist</code></td><td>List files as <code>name:size</code> (| separated)</td></tr>
    <tr><td class="m">GET</td><td><code>/download?name=NAME</code></td><td>Download a file from the SD card</td></tr>
    <tr><td class="m">POST</td><td><code>/upload</code></td><td>Upload a file (multipart form field <code>f</code>)</td></tr>
    <tr><td class="m">GET</td><td><code>/delete?name=NAME</code></td><td>Delete a file from the SD card</td></tr>
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
  </table>
  <p class="sub">The page reads the file list on load and after each action.</p>
</div></div>
)";

#endif
