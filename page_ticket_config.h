#ifndef PAGE_TICKET_CONFIG_H
#define PAGE_TICKET_CONFIG_H

const char* html_ticket_config = R"html(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Ticket Template</title>
    <style>
        :root {
            --ink: #0b0f14;
            --panel: #11161d;
            --line: #232b35;
            --text: #e6edf3;
            --muted: #6b7785;
            --accent: #ffb454;
            --accent-dim: rgba(255, 180, 84, 0.16);
            --ok: #5ec98f;
            --err: #f85149;
            --mono: ui-monospace, 'SF Mono', 'Cascadia Code', 'Consolas', 'Courier New', monospace;
            --sans: -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
        }
        * { box-sizing: border-box; }
        body {
            font-family: var(--sans);
            background-color: var(--ink);
            background-image:
                linear-gradient(90deg, rgba(255, 180, 84, 0.04) 1px, transparent 1px),
                linear-gradient(rgba(255, 180, 84, 0.04) 1px, transparent 1px),
                radial-gradient(circle, rgba(255, 180, 84, 0.12) 1.4px, transparent 1.4px);
            background-size: 56px 56px, 56px 56px, 56px 56px;
            background-position: 0 0, 0 0, 28px 28px;
            color: var(--text);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 24px 16px;
            overflow-y: scroll;
        }
        .panel {
            width: 580px;
            max-width: 95%;
            background: var(--panel);
            border: 1px solid var(--line);
            border-top: 2px solid var(--accent);
            border-radius: 8px;
            box-shadow: 0 20px 50px rgba(0, 0, 0, 0.45);
            overflow: hidden;
        }
        .panel__header {
            display: flex;
            align-items: center;
            gap: 12px;
            height: 85px;
            flex: none;
            padding: 0 24px;
            border-bottom: 1px solid var(--line);
        }
        .logo {
            width: 40px;
            height: 40px;
            border-radius: 10px;
            background: linear-gradient(135deg, var(--accent), #ff8f00);
            padding: 8px;
            color: var(--ink);
            box-shadow: 0 4px 12px var(--accent-dim);
        }
        .panel__heading {
            flex: 1;
            min-width: 0;
        }
        .device-id {
            font-family: var(--mono);
            font-size: 11px;
            letter-spacing: 1.5px;
            color: var(--muted);
            text-transform: uppercase;
        }
        h2 {
            margin: 2px 0 6px;
            font-size: 19px;
            font-weight: 600;
            color: var(--text);
        }
        .subtitle {
            font-size: 12.5px;
            line-height: 1.5;
            color: var(--muted);
            margin: 0;
        }
        .status {
            flex: none;
            display: flex;
            align-items: center;
            gap: 6px;
            font-family: var(--mono);
            font-size: 10.5px;
            letter-spacing: 1px;
            color: var(--ok);
            white-space: nowrap;
            padding-top: 3px;
        }
        .status-dot {
            width: 6px;
            height: 6px;
            border-radius: 50%;
            background: var(--ok);
            box-shadow: 0 0 6px rgba(94, 201, 143, 0.8);
            animation: pulse 2s ease-in-out infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.35; }
        }
        
        /* Navigation Bar */
        .panel__nav {
            display: flex;
            border-bottom: 1px solid var(--line);
            background: rgba(0, 0, 0, 0.25);
            flex: none;
        }
        .nav-item {
            flex: 1;
            text-align: center;
            padding: 14px;
            font-family: var(--mono);
            font-size: 11px;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 1.5px;
            color: var(--muted);
            text-decoration: none;
            transition: all 0.15s ease;
            border-bottom: 2px solid transparent;
        }
        .nav-item:hover {
            color: var(--text);
            background: rgba(255, 255, 255, 0.02);
        }
        .nav-item.active {
            color: var(--accent);
            border-bottom-color: var(--accent);
            background: rgba(255, 255, 255, 0.04);
        }

        .container {
            padding: 20px 24px;
        }

        /* Sub-tab buttons */
        .subtabs {
            display: flex;
            gap: 8px;
            margin-bottom: 14px;
        }
        .subtab-btn {
            flex: 1;
            padding: 9px;
            font-family: var(--mono);
            font-size: 11.5px;
            font-weight: 700;
            color: var(--muted);
            background: rgba(0, 0, 0, 0.3);
            border: 1px solid var(--line);
            border-radius: 6px;
            cursor: pointer;
            transition: all 0.15s;
            text-align: center;
        }
        .subtab-btn:hover {
            color: var(--text);
            border-color: var(--muted);
        }
        .subtab-btn.active {
            color: var(--accent);
            border-color: var(--accent);
            background: var(--accent-dim);
        }

        .status-bar {
            padding: 8px 12px;
            font-size: 12px;
            border-radius: 6px;
            margin-bottom: 14px;
            font-family: var(--mono);
        }
        .status-bar.ok { color: var(--ok); background: rgba(94,201,143,.1); border: 1px solid var(--ok); }
        .status-bar.err { color: var(--err); background: rgba(248,81,73,.1); border: 1px solid var(--err); }
        .status-bar.info { color: var(--accent); background: var(--accent-dim); border: 1px solid var(--accent); }

        #editor {
            width: 100%;
            height: 320px;
            font-family: var(--mono);
            font-size: 12.5px;
            background: rgba(0, 0, 0, 0.35);
            border: 1px solid var(--line);
            color: var(--text);
            padding: 12px;
            border-radius: 6px;
            outline: none;
            line-height: 1.5;
            resize: vertical;
        }
        #editor.invalid { border-color: var(--err); box-shadow: 0 0 0 2px rgba(248,81,73,.2); }
        #editor.valid { border-color: var(--ok); }

        .hint {
            margin-top: 8px;
            font-size: 10.5px;
            color: var(--muted);
            background: rgba(255, 180, 84, 0.05);
            padding: 8px 12px;
            border-radius: 6px;
            border: 1px solid var(--line);
            line-height: 1.6;
        }
        .hint code { color: var(--accent); font-family: var(--mono); }

        .ticket-paper {
            background: #fff;
            color: #111;
            width: 256px;
            padding: 14px 10px;
            font-family: var(--mono);
            border-radius: 4px;
            box-shadow: 0 4px 20px rgba(0,0,0,.5);
            min-height: 160px;
        }
        .tr { width: 100%; white-space: pre-wrap; word-break: break-all; display: block; line-height: 1.4; }
        .tr.left { text-align: left; } .tr.center { text-align: center; } .tr.right { text-align: right; }
        .tr.s1 { font-size: 8.5px; } .tr.s2 { font-size: 11px; } .tr.s3 { font-size: 14px; } .tr.s4 { font-size: 20px; letter-spacing: 2px; }
        .tr.bold { font-weight: 700; } .tr.divider { font-size: 8px; color: #bbb; }

        .btn {
            padding: 10px 14px;
            font-family: var(--mono);
            font-size: 11.5px;
            font-weight: 700;
            letter-spacing: 0.5px;
            color: var(--text);
            background: transparent;
            border: 1px solid var(--line);
            border-radius: 5px;
            cursor: pointer;
            transition: all 0.15s ease;
        }
        .btn:hover { background: rgba(255,255,255,0.05); border-color: var(--accent); color: var(--accent); }
        
        .submit {
            width: 100%;
            margin-top: 14px;
            padding: 13px;
            font-family: var(--mono);
            font-size: 13px;
            font-weight: 700;
            letter-spacing: 1.5px;
            text-transform: uppercase;
            color: var(--ink);
            background: var(--accent);
            border: none;
            border-radius: 6px;
            cursor: pointer;
            transition: background 0.15s ease, transform 0.05s ease;
        }
        .submit:hover { background: #ffc578; }
        .submit:active { transform: translateY(1px); }

        .btn--danger {
            background: transparent;
            color: var(--err);
            border-color: #663333;
        }
        .btn--danger:hover {
            background: rgba(248,81,73,.15);
            border-color: var(--err);
        }

        .badge {
            font-size: 10px;
            padding: 2px 7px;
            border-radius: 10px;
            background: var(--accent-dim);
            color: var(--accent);
            font-weight: 600;
        }

        input[type=file] { display: none; }

        /* Mobile Responsive Adjustments */
        @media (max-width: 480px) {
            body { padding: 0; }
            .panel {
                max-width: 100%;
                border-radius: 0;
                border-left: none;
                border-right: none;
                box-shadow: none;
                min-height: 100vh;
            }
            .panel__header {
                padding: 0 16px;
                height: 75px;
            }
            .chip-icon { display: none; }
            .panel__nav {
                overflow-x: auto;
                -webkit-overflow-scrolling: touch;
            }
            .panel__nav::-webkit-scrollbar { display: none; }
            .nav-item {
                padding: 12px 16px;
                font-size: 10px;
                white-space: nowrap;
                flex: none;
            }
            .container { padding: 16px; }
            #editor { height: 260px; font-size: 13px !important; }
            .action-grid { flex-direction: column; }
        }
    </style>
</head>
<body>
    <div class="panel">
        <div class="panel__header">
            <svg class="logo chip-icon" style="flex: none;" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
                <rect x="9" y="9" width="14" height="14" rx="1.5" stroke="currentColor" stroke-width="1.6"/>
                <rect x="13" y="13" width="6" height="6" rx="0.5" stroke="currentColor" stroke-width="1.4"/>
                <path d="M9 13H4M9 19H4M28 13H23M28 19H23M13 9V4M19 9V4M13 28V23M19 28V23" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/>
            </svg>
            <div class="panel__heading">
                <div class="device-id">ESP32 &middot; SoC</div>
                <h2>Ticket Template</h2>
                <p class="subtitle">JSON receipt layout editor.</p>
            </div>
            <div class="status"><span class="status-dot"></span>Active</div>
        </div>

        <div class="panel__nav">
            <a href="/" class="nav-item">Configuration</a>
            <a href="/gpio" class="nav-item">GPIO Mapping</a>
            <a href="/ticket-config" class="nav-item active">Ticket Template</a>
            <a href="/ota" class="nav-item">Update</a>
            <a href="/log" class="nav-item">System Logs</a>
        </div>

        <div class="container">
            <!-- Subtab View Switcher -->
            <div class="subtabs">
                <button class="subtab-btn active" id="tabEdit" onclick="switchTab('edit')">ðŸ“ JSON Editor</button>
                <button class="subtab-btn" id="tabPreview" onclick="switchTab('preview')">ðŸ‘ Receipt Preview</button>
            </div>

            <!-- Status Notification Bar -->
            <div class="status-bar info" id="statusBar">Loading template from device...</div>

            <!-- View 1: JSON Editor -->
            <div id="viewEdit">
                <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
                    <span class="badge" id="rowBadge">0 rows</span>
                    <span id="validBadge" style="font-size: 11px; color: var(--muted);">Unchecked</span>
                </div>
                <textarea id="editor" spellcheck="false" placeholder="Loading..."></textarea>
                <div class="hint">
                    Variables: <code>{unit_name}</code> <code>{service_name}</code> <code>{ticket_number}</code> <code>{calling_ticket}</code> <code>{time}</code> <code>{date}</code> | size: 1-4 | align: left/center/right | type: text/divider
                </div>
            </div>

            <!-- View 2: Live Preview -->
            <div id="viewPreview" style="display: none;">
                <div style="display: flex; flex-direction: column; align-items: center; background: rgba(0,0,0,0.25); padding: 18px 12px; border-radius: 6px; border: 1px solid var(--line);">
                    <div class="ticket-paper" id="preview">
                        <div class="tr center s1" style="color:#aaa">Loading...</div>
                    </div>
                    <div style="font-size: 10px; color: var(--muted); margin-top: 12px; text-align: center;">* Preview uses sample data. Real values supplied by server.</div>
                </div>
            </div>

            <!-- Action Buttons -->
            <button class="submit" onclick="saveTemplate()">SAVE CONFIGURATION</button>

            <div class="action-grid" style="display: flex; gap: 8px; margin-top: 10px; flex-wrap: wrap;">
                <button class="btn" style="flex: 1;" onclick="testPrint()">ðŸ–¨ Test Print</button>
                <button class="btn" style="flex: 1;" onclick="downloadTemplate()">ðŸ“¥ Download</button>
                <label class="btn" style="flex: 1; text-align: center;" for="fileInput">ðŸ“¤ Upload</label>
                <input type="file" id="fileInput" accept=".json,application/json" onchange="loadFromFile(event)">
            </div>

            <button class="btn btn--danger" style="width: 100%; margin-top: 10px;" onclick="resetDefault()">â™» Reset to Default</button>
        </div>
    </div>

<script>
var SAMPLE = {unit_name:"BV NGUYEN TRAI",service_name:"KHAM RANG - HAM - MAT",ticket_number:"1003",calling_ticket:"0998",time:"13:30:00",date:"22/07/2026"};
var DEFAULT_TPL = {version:1,paper_width:80,rows:[
  {type:"text",content:"{unit_name}",align:"center",size:2,bold:true,line_spacing:50},
  {type:"text",content:"{service_name}",align:"center",size:2,bold:true,line_spacing:45},
  {type:"divider",line_spacing:30},
  {type:"text",content:"SO THU TU",align:"center",size:2,bold:true,line_spacing:45},
  {type:"text",content:"{ticket_number}",align:"center",size:4,bold:true,line_spacing:80},
  {type:"divider",line_spacing:30},
  {type:"text",content:"STT dang goi: {calling_ticket}",align:"center",size:2,bold:true,line_spacing:45},
  {type:"divider",line_spacing:30},
  {type:"text",content:"QUY KHACH vui long cho duoc phuc vu theo",align:"center",size:1,bold:false,line_spacing:45},
  {type:"text",content:"STT hien tren bang dien tu. Xin cam on!",align:"center",size:1,bold:false,line_spacing:45},
  {type:"text",content:"STT duoc in luc: {time} ngay {date}",align:"center",size:1,bold:false,line_spacing:40}
]};
var ed=document.getElementById("editor"),sb=document.getElementById("statusBar"),pv=document.getElementById("preview"),vb=document.getElementById("validBadge"),rb=document.getElementById("rowBadge");

function switchTab(t){
  document.getElementById("tabEdit").className="subtab-btn"+(t==="edit"?" active":"");
  document.getElementById("tabPreview").className="subtab-btn"+(t==="preview"?" active":"");
  document.getElementById("viewEdit").style.display=t==="edit"?"block":"none";
  document.getElementById("viewPreview").style.display=t==="preview"?"block":"none";
  if(t==="preview"){renderPreview(parseEditor());}
}

function setStatus(m,t){sb.textContent=m;sb.className="status-bar "+(t||"info")}
function sv(s){return s.replace(/{unit_name}/g,SAMPLE.unit_name).replace(/{service_name}/g,SAMPLE.service_name).replace(/{ticket_number}/g,SAMPLE.ticket_number).replace(/{calling_ticket}/g,SAMPLE.calling_ticket).replace(/{time}/g,SAMPLE.time).replace(/{date}/g,SAMPLE.date)}
function esc(s){return s.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;")}
function renderPreview(tpl){
  if(!tpl||!Array.isArray(tpl.rows)){pv.innerHTML="<div class=\"tr center s1\" style=\"color:red\">Invalid JSON format</div>";return}
  var h="";
  tpl.rows.forEach(function(r){
    var al=r.align||"center",sz="s"+(r.size||1),bd=r.bold?" bold":"",gap=Math.max((r.line_spacing||40)-30,1);
    if(r.type==="divider"){
      h+="<div class=\"tr center s1 divider\">------------------------------------------</div>";
    } else {
      var ct=sv(r.content||"");
      h+="<div class=\"tr "+al+" "+sz+bd+"\" style=\"margin-bottom:"+gap+"px\">"+esc(ct)+"</div>";
    }
  });
  pv.innerHTML=h||"<div class=\"tr center s1\" style=\"color:#aaa\">No content</div>";
  rb.textContent=tpl.rows.length+" rows";
}
function parseEditor(){
  try{var t=JSON.parse(ed.value);ed.className="valid";vb.textContent="Valid JSON";vb.style.color="var(--ok)";return t}
  catch(e){ed.className="invalid";vb.textContent="Error: "+e.message.slice(0,40);vb.style.color="var(--err)";return null}
}
function onEditorChange(){var t=parseEditor();if(t)renderPreview(t)}
ed.addEventListener("input",onEditorChange);
function loadTemplate(){
  var xhr=new XMLHttpRequest();xhr.open("GET","/api/ticket-template");
  xhr.onload=function(){
    if(xhr.status===200){
      try{var p=JSON.parse(xhr.responseText);ed.value=JSON.stringify(p,null,2);onEditorChange();setStatus("Template loaded successfully","ok")}
      catch(e){ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();setStatus("Default template loaded (Corrupted NVS data)","info")}
    } else {
      ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();setStatus("Default template loaded (No NVS config stored)","info");
    }
  };
  xhr.onerror=function(){ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();setStatus("Default template loaded (Connection error)","info")};
  xhr.send();
}
function saveTemplate(cb){
  var t=parseEditor();if(!t){setStatus("Invalid JSON format â€“ please fix before saving","err");if(cb)cb(false);return}
  setStatus("Saving template to device...","info");
  var xhr=new XMLHttpRequest();xhr.open("POST","/api/ticket-template");
  xhr.setRequestHeader("Content-Type","application/json");
  xhr.onload=function(){
    if(xhr.status===200){setStatus("Template saved successfully! "+xhr.responseText,"ok");if(cb)cb(true)}
    else{setStatus("Error: "+xhr.responseText,"err");if(cb)cb(false)}
  };
  xhr.onerror=function(){setStatus("Connection error","err");if(cb)cb(false)};
  xhr.send(ed.value);
}
function testPrint(){
  saveTemplate(function(ok){
    if(!ok)return;
    setStatus("Sending test print request...","info");
    var xhr=new XMLHttpRequest();xhr.open("GET","/api/test_print");
    xhr.onload=function(){if(xhr.status===200)setStatus("Test print successful!","ok");else setStatus("Test print error: HTTP "+xhr.status,"err")};
    xhr.onerror=function(){setStatus("Connection error during test print","err")};
    xhr.send();
  });
}
function downloadTemplate(){window.open("/api/ticket-template/download","_blank")}
function loadFromFile(e){
  var f=e.target.files[0];if(!f)return;
  var reader=new FileReader();
  reader.onload=function(ev){
    try{var p=JSON.parse(ev.target.result);ed.value=JSON.stringify(p,null,2);onEditorChange();setStatus("Loaded file: "+f.name,"ok")}
    catch(er){setStatus("Invalid file: "+er.message,"err")}
    e.target.value="";
  };
  reader.readAsText(f);
}
function resetDefault(){
  if(!confirm("Reset to default template? Unsaved changes will be lost."))return;
  ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();
  setStatus("Default template loaded (Unsaved)","info");
}
loadTemplate();
</script>
</body>
</html>
)html";

#endif // PAGE_TICKET_CONFIG_H