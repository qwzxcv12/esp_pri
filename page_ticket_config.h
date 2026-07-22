#ifndef PAGE_TICKET_CONFIG_H
#define PAGE_TICKET_CONFIG_H

const char* html_ticket_config = Rhtml(
<!DOCTYPE html>
<html lang=vi>
<head>
<meta charset=UTF-8>
<meta name=viewport content=width=device-width,initial-scale=1.0>
<title>Cau Hinh Mau Phieu In</title>
<style>
:root{--ink:#0b0f14;--panel:#11161d;--line:#232b35;--text:#e6edf3;--muted:#6b7785;--accent:#ffb454;--accent-dim:rgba(255,180,84,.16);--ok:#5ec98f;--err:#f85149;--mono:ui-monospace,SF Mono,Consolas,monospace;--sans:-apple-system,Segoe UI,Roboto,sans-serif;}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:var(--sans);background:var(--ink);color:var(--text);min-height:100vh;display:flex;flex-direction:column}
a{color:var(--accent);text-decoration:none}
header{background:var(--panel);border-bottom:1px solid var(--line);padding:14px 20px;display:flex;align-items:center;justify-content:space-between}
.logo{font-size:17px;font-weight:700;color:var(--accent)}.subtitle{font-size:12px;color:var(--muted);margin-top:2px}
.nav-links{display:flex;gap:12px;font-size:13px}.nav-links a:hover{text-decoration:underline}
.toolbar{background:var(--panel);border-bottom:1px solid var(--line);padding:10px 20px;display:flex;gap:8px;flex-wrap:wrap;align-items:center}
.btn{display:inline-flex;align-items:center;gap:6px;padding:7px 14px;border:1px solid var(--line);border-radius:6px;background:transparent;color:var(--text);font-size:13px;cursor:pointer;transition:all .15s;font-family:inherit}
.btn:hover{border-color:var(--accent);color:var(--accent);background:var(--accent-dim)}
.btn-primary{background:var(--accent);color:#000;border-color:var(--accent);font-weight:600}
.btn-primary:hover{background:#ffca82;color:#000;border-color:#ffca82}
.btn-danger{border-color:#663333;color:#f85149}.btn-danger:hover{background:rgba(248,81,73,.15);border-color:var(--err)}
.status-bar{padding:8px 20px;font-size:12px;display:flex;align-items:center;gap:8px;border-bottom:1px solid var(--line)}
.status-bar.ok{color:var(--ok);background:rgba(94,201,143,.08)}.status-bar.err{color:var(--err);background:rgba(248,81,73,.08)}.status-bar.info{color:var(--accent);background:var(--accent-dim)}
.main{flex:1;display:grid;grid-template-columns:1fr 300px;gap:0;overflow:hidden;height:calc(100vh - 130px)}
.editor-pane{display:flex;flex-direction:column;border-right:1px solid var(--line)}
.pane-header{padding:10px 16px;background:rgba(255,255,255,.03);border-bottom:1px solid var(--line);font-size:12px;color:var(--muted);display:flex;justify-content:space-between;align-items:center}
.pane-header span{font-weight:600;color:var(--text)}
#editor{flex:1;resize:none;background:#0d1117;color:#e6edf3;border:none;outline:none;padding:16px;font-family:var(--mono);font-size:13px;line-height:1.6;width:100%;height:100%}
#editor.invalid{box-shadow:inset 3px 0 0 var(--err)}#editor.valid{box-shadow:inset 3px 0 0 var(--ok)}
.preview-pane{display:flex;flex-direction:column;background:var(--panel);overflow:hidden}
.preview-scroll{flex:1;overflow-y:auto;padding:16px;display:flex;flex-direction:column;align-items:center}
.ticket-paper{background:#fff;color:#111;width:256px;padding:12px 8px;font-family:var(--mono);border-radius:4px;box-shadow:0 4px 20px rgba(0,0,0,.5);min-height:160px}
.tr{width:100%;white-space:pre-wrap;word-break:break-all;display:block;line-height:1.4}
.tr.left{text-align:left}.tr.center{text-align:center}.tr.right{text-align:right}
.tr.s1{font-size:8.5px}.tr.s2{font-size:11px}.tr.s3{font-size:14px}.tr.s4{font-size:20px;letter-spacing:2px}
.tr.bold{font-weight:700}.tr.divider{font-size:8px;color:#bbb}
.preview-note{font-size:10px;color:var(--muted);text-align:center;padding:8px 4px}
.badge{font-size:10px;padding:2px 7px;border-radius:10px;background:var(--accent-dim);color:var(--accent);font-weight:600}
.sep{width:1px;height:20px;background:var(--line);margin:0 4px}
input[type=file]{display:none}
@media(max-width:700px){.main{grid-template-columns:1fr}.preview-pane{display:none}}
.schema-hint{padding:12px 16px;background:rgba(255,180,84,.05);border-top:1px solid var(--line);font-size:11px;color:var(--muted)}
.schema-hint code{color:var(--accent);font-family:var(--mono)}
</style>
</head>
<body>
<header>
<div><div class=logo>🖨 Cau Hinh Mau Phieu In</div><div class=subtitle>JSON Template Editor</div></div>
<div class=nav-links><a href=/>⚙ Cau hinh</a><a href=/log>📋 Log</a></div>
</header>
<div class=toolbar>
<button class=btn btn-primary onclick=saveTemplate()>✅ Luu len thiet bi</button>
<button class=btn onclick=testPrint()>🖨 In thu ngay</button>
<div class=sep></div>
<button class=btn onclick=downloadTemplate()>📥 Tai mau ve may</button>
<label class=btn for=fileInput>📤 Tai len tu file</label>
<input type=file id=fileInput accept=.json,application/json onchange=loadFromFile(event)>
<div class=sep></div>
<button class=btn btn-danger onclick=resetDefault()>♻ Khoi phuc mac dinh</button>
</div>
<div class=status-bar info id=statusBar>Dang tai template tu thiet bi...</div>
<div class=main>
<div class=editor-pane>
<div class=pane-header><span>📝 JSON Editor</span><div style=display:flex;gap:8px;align-items:center><span class=badge id=rowBadge>0 dong</span><span id=validBadge style=font-size:11px;color:var(--muted)>Chua kiem tra</span></div></div>
<textarea id=editor spellcheck=false placeholder=Dang tai... oninput=onEditorChange()></textarea>
<div class=schema-hint>Bien dong: <code>{unit_name}</code> <code>{service_name}</code> <code>{ticket_number}</code> <code>{calling_ticket}</code> <code>{time}</code> <code>{date}</code> &nbsp;|&nbsp; size: 1-4 &nbsp;|&nbsp; align: left/center/right &nbsp;|&nbsp; type: text / divider</div>
</div>
<div class=preview-pane><div class=pane-header><span>👁 Preview</span><span style=font-size:11px;color:var(--muted)>Kho 80mm</span></div>
<div class=preview-scroll><div class=ticket-paper id=preview></div>
<div class=preview-note>* Preview dung du lieu mau. Gia tri thuc te tu Server.</div>
</div></div>
</div>
<script>
const SAMPLE={unit_name:BV NGUYEN TRAI,service_name:KHAM RANG - HAM - MAT,ticket_number:1003,calling_ticket:0998,time:13:30:00,date:22/07/2026};
const DEFAULT_TPL={version:1,paper_width:80,rows:[{type:text,content:{unit_name},align:center,size:2,bold:true,line_spacing:50},{type:text,content:{service_name},align:center,size:2,bold:true,line_spacing:45},{type:divider,line_spacing:30},{type:text,content:SO THU TU,align:center,size:2,bold:true,line_spacing:45},{type:text,content:{ticket_number},align:center,size:4,bold:true,line_spacing:80},{type:divider,line_spacing:30},{type:text,content:STT dang goi: {calling_ticket},align:center,size:2,bold:true,line_spacing:45},{type:divider,line_spacing:30},{type:text,content:QUY KHACH vui long cho duoc phuc vu theo,align:center,size:1,bold:false,line_spacing:45},{type:text,content:STT hien tren bang dien tu. Xin cam on!,align:center,size:1,bold:false,line_spacing:45},{type:text,content:STT duoc in luc: {time} ngay {date},align:center,size:1,bold:false,line_spacing:40}]};
const ed=document.getElementById(editor),sb=document.getElementById(statusBar),pv=document.getElementById(preview),vb=document.getElementById(validBadge),rb=document.getElementById(rowBadge);
function setStatus(m,t=info){sb.textContent=m;sb.className=status-bar +t}
function sv(s){return s.replace(/{unit_name}/g,SAMPLE.unit_name).replace(/{service_name}/g,SAMPLE.service_name).replace(/{ticket_number}/g,SAMPLE.ticket_number).replace(/{calling_ticket}/g,SAMPLE.calling_ticket).replace(/{time}/g,SAMPLE.time).replace(/{date}/g,SAMPLE.date)}
function esc(s){return s.replace(/&/g,&amp;).replace(/</g,&lt;).replace(/>/g,&gt;)}
function renderPreview(tpl){if(!tpl||!Array.isArray(tpl.rows)){pv.innerHTML=<div class=tr center s1 style=color:red>JSON khong hop le</div>;return}let h=;tpl.rows.forEach(r=>{let al=r.align||center,sz=s+(r.size||1),bd=r.bold? bold:;if(r.type===divider){h+=<div class="tr center s1 divider">------------------------------------------</div>}else{let ct=sv(r.content||);h+=<div class="tr +al+ +sz+bd+" style="margin-bottom:+(Math.max((r.line_spacing||40)-30,1))+px">+esc(ct)+</div>}});pv.innerHTML=h||<div class=tr center s1 style=color:#aaa>Khong co noi dung</div>;rb.textContent=tpl.rows.length+ dong}
function parseEditor(){try{let t=JSON.parse(ed.value);ed.className=valid;vb.textContent=✅ JSON hop le;vb.style.color=var(--ok);return t}catch(e){ed.className=invalid;vb.textContent=❌ +e.message.slice(0,40);vb.style.color=var(--err);return null}}
function onEditorChange(){const t=parseEditor();if(t)renderPreview(t)}
async function loadTemplate(){try{const r=await fetch(/api/ticket-template);if(!r.ok)throw new Error(HTTP +r.status);const j=await r.text();ed.value=JSON.stringify(JSON.parse(j),null,2);onEditorChange();setStatus(✅ Da tai template tu thiet bi,ok)}catch(e){ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();setStatus(ℹ Template mac dinh (chua co du lieu luu tren thiet bi),info)}}
async function saveTemplate(){const t=parseEditor();if(!t){setStatus(❌ JSON khong hop le,err);return false}setStatus(Dang luu...,info);try{const r=await fetch(/api/ticket-template,{method:POST,headers:{Content-Type:application/json},body:ed.value});const tx=await r.text();if(r.ok){setStatus(✅ Da luu thanh cong! +tx,ok);return true}else{setStatus(❌ Loi: +tx,err);return false}}catch(e){setStatus(❌ Loi ket noi: +e.message,err);return false}}
async function testPrint(){if(!await saveTemplate())return;setStatus(🖨 Dang gui lenh in thu...,info);try{const r=await fetch(/api/test_print);if(r.ok)setStatus(✅ In thu thanh cong!,ok);else setStatus(❌ Loi in thu: HTTP +r.status,err)}catch(e){setStatus(❌ Loi: +e.message,err)}}
function downloadTemplate(){window.open(/api/ticket-template/download,_blank)}
function loadFromFile(e){const f=e.target.files[0];if(!f)return;const r=new FileReader();r.onload=ev=>{try{const p=JSON.parse(ev.target.result);ed.value=JSON.stringify(p,null,2);onEditorChange();setStatus(✅ Da tai file: +f.name,ok)}catch(er){setStatus(❌ File khong hop le: +er.message,err)}; e.target.value=};r.readAsText(f)}
function resetDefault(){if(!confirm(Khoi phuc template mac dinh?))return;ed.value=JSON.stringify(DEFAULT_TPL,null,2);onEditorChange();setStatus(ℹ Template mac dinh (chua luu),info)}
loadTemplate();
</script>
</body>
</html>
)html;

#endif // PAGE_TICKET_CONFIG_H
