#ifndef PAGE_TICKET_CONFIG_H
#define PAGE_TICKET_CONFIG_H

const char* html_ticket_config = R"html(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Cau Hinh Mau Phieu In</title>
<style>
:root{--ink:#0b0f14;--panel:#11161d;--line:#232b35;--text:#e6edf3;--muted:#6b7785;--accent:#ffb454;--accent-dim:rgba(255,180,84,.16);--ok:#5ec98f;--err:#f85149;--mono:ui-monospace,"SF Mono",Consolas,monospace;--sans:-apple-system,"Segoe UI",Roboto,sans-serif;}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:var(--sans);background:var(--ink);color:var(--text);min-height:100vh;display:flex;flex-direction:column}
a{color:var(--accent);text-decoration:none}
header{background:var(--panel);border-bottom:1px solid var(--line);padding:14px 20px;display:flex;align-items:center;justify-content:space-between}
.logo{font-size:17px;font-weight:700;color:var(--accent)}.subtitle{font-size:12px;color:var(--muted);margin-top:2px}
.nav-links{display:flex;gap:12px;font-size:13px}
.toolbar{background:var(--panel);border-bottom:1px solid var(--line);padding:10px 20px;display:flex;gap:8px;flex-wrap:wrap;align-items:center}
.btn{display:inline-flex;align-items:center;gap:6px;padding:7px 14px;border:1px solid var(--line);border-radius:6px;background:transparent;color:var(--text);font-size:13px;cursor:pointer;transition:all .15s;font-family:inherit}
.btn:hover{border-color:var(--accent);color:var(--accent);background:var(--accent-dim)}
.btn-primary{background:var(--accent);color:#000;border-color:var(--accent);font-weight:600}
.btn-primary:hover{background:#ffca82;color:#000;border-color:#ffca82}
.btn-danger{border-color:#663333;color:#f85149}.btn-danger:hover{background:rgba(248,81,73,.15);border-color:var(--err)}
.status-bar{padding:8px 20px;font-size:13px;display:flex;align-items:center;gap:8px;border-bottom:1px solid var(--line)}
.status-bar.ok{color:var(--ok);background:rgba(94,201,143,.08)}.status-bar.err{color:var(--err);background:rgba(248,81,73,.08)}.status-bar.info{color:var(--accent);background:var(--accent-dim)}
.main{flex:1;display:grid;grid-template-columns:1fr 300px;gap:0;min-height:0}
.editor-pane{display:flex;flex-direction:column;border-right:1px solid var(--line);min-height:0}
.pane-hdr{padding:10px 16px;background:rgba(255,255,255,.03);border-bottom:1px solid var(--line);font-size:12px;color:var(--muted);display:flex;justify-content:space-between;align-items:center;flex-shrink:0}
.pane-hdr span{font-weight:600;color:var(--text)}
#editor{flex:1;resize:none;background:#0d1117;color:#e6edf3;border:none;outline:none;padding:16px;font-family:var(--mono);font-size:13px;line-height:1.6;width:100%;min-height:300px}
#editor.invalid{box-shadow:inset 3px 0 0 var(--err)}#editor.valid{box-shadow:inset 3px 0 0 var(--ok)}
.hint{padding:8px 16px;background:rgba(255,180,84,.05);border-top:1px solid var(--line);font-size:11px;color:var(--muted);flex-shrink:0}
.hint code{color:var(--accent);font-family:var(--mono)}
.preview-pane{display:flex;flex-direction:column;background:var(--panel);overflow:hidden}
.preview-scroll{flex:1;overflow-y:auto;padding:16px;display:flex;flex-direction:column;align-items:center}
.ticket-paper{background:#fff;color:#111;width:256px;padding:12px 8px;font-family:var(--mono);border-radius:4px;box-shadow:0 4px 20px rgba(0,0,0,.5);min-height:160px}
.tr{width:100%;white-space:pre-wrap;word-break:break-all;display:block;line-height:1.4}
.tr.left{text-align:left}.tr.center{text-align:center}.tr.right{text-align:right}
.tr.s1{font-size:8.5px}.tr.s2{font-size:11px}.tr.s3{font-size:14px}.tr.s4{font-size:20px;letter-spacing:2px}
.tr.bold{font-weight:700}.tr.divider{font-size:8px;color:#bbb}
.pvnote{font-size:10px;color:var(--muted);text-align:center;padding:8px 4px}
.badge{font-size:10px;padding:2px 7px;border-radius:10px;background:var(--accent-dim);color:var(--accent);font-weight:600}
.sep{width:1px;height:20px;background:var(--line);margin:0 4px}
input[type=file]{display:none}
@media(max-width:700px){.main{grid-template-columns:1fr}.preview-pane{display:none}}
</style>
</head>
<body>
<header>
<div><div class="logo">Cau Hinh Mau Phieu In</div><div class="subtitle">JSON Template Editor</div></div>
<div class="nav-links"><a href="/">Configuration</a><a href="/gpio">GPIO Mapping</a><a href="/ticket-config" style="color:var(--accent);font-weight:bold">Ticket Template</a><a href="/ota">Update</a><a href="/log">System Logs</a></div>
</header>
<div class="toolbar">
<button class="btn btn-primary" onclick="saveTemplate()">Luu len thiet bi</button>
<button class="btn" onclick="testPrint()">In thu ngay</button>
<div class="sep"></div>
<button class="btn" onclick="downloadTemplate()">Tai mau ve may</button>
<label class="btn" for="fileInput">Tai len tu file</label>
<input type="file" id="fileInput" accept=".json,application/json" onchange="loadFromFile(event)">
<div class="sep"></div>
<button class="btn btn-danger" onclick="resetDefault()">Khoi phuc mac dinh</button>
</div>
<div class="status-bar info" id="statusBar">Dang tai template tu thiet bi...</div>
<div class="main">
<div class="editor-pane">
<div class="pane-hdr"><span>JSON Editor</span><div style="display:flex;gap:8px;align-items:center"><span class="badge" id="rowBadge">0 dong</span><span id="validBadge" style="font-size:11px;color:var(--muted)">Chua kiem tra</span></div></div>
<textarea id="editor" spellcheck="false" placeholder="Dang tai..."></textarea>
<div class="hint">Bien: <code>{unit_name}</code> <code>{service_name}</code> <code>{ticket_number}</code> <code>{calling_ticket}</code> <code>{time}</code> <code>{date}</code> | size: 1-4 | align: left/center/right | type: text/divider</div>
</div>
<div class="preview-pane">
<div class="pane-hdr"><span>Preview phieu in</span><span style="font-size:11px;color:var(--muted)">Kho 80mm</span></div>
<div class="preview-scroll">
<div class="ticket-paper" id="preview"><div class="tr center s1" style="color:#aaa">Dang tai...</div></div>
<div class="pvnote">Preview dung du lieu mau. Gia tri thuc te tu Server.</div>
</div>
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
function setStatus(m,t){sb.textContent=m;sb.className="status-bar "+(t||"info")}
function sv(s){return s.replace(/{unit_name}/g,SAMPLE.unit_name).replace(/{service_name}/g,SAMPLE.service_name).replace(/{ticket_number}/g,SAMPLE.ticket_number).replace(/{calling_ticket}/g,SAMPLE.calling_ticket).replace(/{time}/g,SAMPLE.time).replace(/{date}/g,SAMPLE.date)}
function esc(s){return s.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;")}
function renderPreview(tpl){
  if(!tpl||!Array.isArray(tpl.rows)){pv.innerHTML="<div class=\"tr center s1\" style=\"color:red\">JSON khong hop le</div>";return}
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
  pv.innerHTML=h||"<div class=\"tr center s1\" style=\"color:#aaa\">Khong co noi dung</div>";
  rb.textContent=tpl.rows.length+" dong";
}
function parseEditor(){
  try{var t=JSON.parse(ed.value);ed.className="valid";vb.textContent="JSON hop le";vb.style.color="var(--ok)";return t}
  catch(e){ed.className="invalid";vb.textContent="Loi: "+e.message.slice(0,40);vb.style.color="var(--err)";return null}
}
function onEditorChange(){var t=parseEditor();if(t)renderPreview(t)}
ed.addEventListener("input",onEditorChange);
function loadTemplate(){
  var xhr=new XMLHttpRequest();xhr.open("GET","/api/ticket-template");
  xhr.onload=function(){
    if(xhr.status===200){
      try{var p=JSON.parse(xhr.responseText);ed.value=JSON.stringify(p,null,2);onEditorChange();setStatus("Da tai template tu thiet bi","ok")}
      catch(e){ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();setStatus("Template mac dinh (du lieu NVS bi loi)","info")}
    } else {
      ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();setStatus("Template mac dinh (chua co du lieu luu tren thiet bi)","info");
    }
  };
  xhr.onerror=function(){ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();setStatus("Template mac dinh (loi ket noi)","info")};
  xhr.send();
}
function saveTemplate(cb){
  var t=parseEditor();if(!t){setStatus("JSON khong hop le â€“ vui long sua truoc khi luu","err");if(cb)cb(false);return}
  setStatus("Dang luu len thiet bi...","info");
  var xhr=new XMLHttpRequest();xhr.open("POST","/api/ticket-template");
  xhr.setRequestHeader("Content-Type","application/json");
  xhr.onload=function(){
    if(xhr.status===200){setStatus("Da luu thanh cong! "+xhr.responseText,"ok");if(cb)cb(true)}
    else{setStatus("Loi: "+xhr.responseText,"err");if(cb)cb(false)}
  };
  xhr.onerror=function(){setStatus("Loi ket noi","err");if(cb)cb(false)};
  xhr.send(ed.value);
}
function testPrint(){
  saveTemplate(function(ok){
    if(!ok)return;
    setStatus("Dang gui lenh in thu...","info");
    var xhr=new XMLHttpRequest();xhr.open("GET","/api/test_print");
    xhr.onload=function(){if(xhr.status===200)setStatus("In thu thanh cong!","ok");else setStatus("Loi in thu: HTTP "+xhr.status,"err")};
    xhr.onerror=function(){setStatus("Loi ket noi khi in thu","err")};
    xhr.send();
  });
}
function downloadTemplate(){window.open("/api/ticket-template/download","_blank")}
function loadFromFile(e){
  var f=e.target.files[0];if(!f)return;
  var reader=new FileReader();
  reader.onload=function(ev){
    try{var p=JSON.parse(ev.target.result);ed.value=JSON.stringify(p,null,2);onEditorChange();setStatus("Da tai file: "+f.name,"ok")}
    catch(er){setStatus("File khong hop le: "+er.message,"err")}
    e.target.value="";
  };
  reader.readAsText(f);
}
function resetDefault(){
  if(!confirm("Khoi phuc template mac dinh? Noi dung hien tai se bi xoa."))return;
  ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();
  setStatus("Template mac dinh (chua luu)","info");
}
loadTemplate();
</script>
</body>
</html>
)html";

#endif // PAGE_TICKET_CONFIG_H