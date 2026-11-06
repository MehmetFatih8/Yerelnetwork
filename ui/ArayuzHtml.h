#pragma once

// Yerelkeşif arayüzü. Derleme anında programa gömülür.
// C++ tarafı window.<ad> fonksiyonlarını çağırarak arayüzü günceller; arayüz de w.bind ile bağlanan
// C++ fonksiyonlarını (cpp...) çağırır.
inline constexpr const char* kArayuzHtml = R"HTML(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Yerelkeşif</title>
<style>
  /* Renkler: Material 3 koyu tonal paleti + cam (frosted) yüzeyler */
  :root {
    --bg: #0c1018;
    --surface: rgba(30, 36, 48, .58);
    --stroke: rgba(255, 255, 255, .09);
    --primary: #a9c9ff;
    --on-primary: #0a2650;
    --primary-soft: rgba(169, 201, 255, .15);
    --accent: #7fe3cf;
    --success: #7ee6a6;
    --danger: #ff9b9b;
    --text: #eef2fa;
    --muted: #9aa6bc;
    --font-display: "Segoe UI Variable Display", "Segoe UI Variable", "Segoe UI", system-ui, sans-serif;
    --font-text: "Segoe UI Variable Text", "Segoe UI Variable", "Segoe UI", system-ui, sans-serif;
    color-scheme: dark;
  }

  * { box-sizing: border-box; margin: 0; }

  html, body {
    height: 100%;
    background: var(--bg);
    color: var(--text);
    font: 500 14px/1.45 var(--font-text);
    text-rendering: optimizeLegibility;
    overflow: hidden;
    user-select: none;
  }

  /* Arka plan: derin lacivert zemin üzerinde yumuşak renk lekeleri */
  body {
    background:
      radial-gradient(900px 620px at 0% 0%, rgba(70, 110, 190, .42), transparent 62%),
      radial-gradient(820px 620px at 100% 100%, rgba(30, 120, 110, .36), transparent 60%),
      radial-gradient(640px 520px at 62% 18%, rgba(120, 80, 170, .20), transparent 62%),
      var(--bg);
  }

  /* Pencerenin üst kenarından boyutlandırma tutamacı */
  .ust-kenar {
    position: fixed; top: 0; left: 0; right: 0; height: 5px;
    z-index: 50; cursor: ns-resize;
  }

  /* Özel başlık çubuğu */
  .titlebar {
    height: 40px; flex-shrink: 0;
    display: flex; align-items: center; justify-content: space-between;
    background: rgba(6, 9, 15, .92);
    border-bottom: 1px solid rgba(255, 255, 255, .06);
    padding-left: 14px;
  }
  .brand {
    flex: 1; height: 100%;
    display: flex; align-items: center; gap: 10px;
    font: 600 13px/1 var(--font-display); color: var(--text); opacity: .92;
  }
  .logo {
    width: 14px; height: 14px; border-radius: 5px;
    background: linear-gradient(135deg, var(--primary), var(--accent));
    box-shadow: 0 0 10px rgba(169, 201, 255, .35);
  }
  .wc { display: flex; height: 100%; }
  .wc-btn {
    width: 46px; height: 100%; padding: 0;
    display: flex; align-items: center; justify-content: center;
    border: 0; border-radius: 0; background: transparent; color: var(--muted);
    cursor: pointer; transition: background .15s ease, color .15s ease;
  }
  .wc-btn:hover { background: rgba(255, 255, 255, .08); color: var(--text); }
  .wc-btn:focus-visible { outline: none; background: rgba(255, 255, 255, .10); }
  .wc-btn svg { width: 12px; height: 12px; }
  .wc-kapat:hover { background: #c42b1c; color: #fff; }
  .ikon-restore { display: none; }
  .buyuk .ikon-restore { display: block; }
  .buyuk .ikon-max { display: none; }

  .app {
    display: flex; gap: 14px; height: calc(100% - 40px); padding: 16px;
    opacity: 0; transform: translateY(6px);
    transition: opacity .35s ease, transform .35s ease;
  }
  .hazir .app { opacity: 1; transform: none; }

  /* Paneller: cam yüzey, ince kenar ışığı, yumuşak gölge */
  .panel {
    background: var(--surface);
    backdrop-filter: blur(28px) saturate(170%);
    border: 1px solid var(--stroke);
    border-radius: 28px;
    box-shadow: 0 18px 50px rgba(0, 0, 0, .35), inset 0 1px 0 rgba(255, 255, 255, .06);
    padding: 20px;
    display: flex; flex-direction: column;
    min-height: 0;
  }
  .left { width: 34%; min-width: 280px; }
  .right { flex: 1; min-width: 0; }

  .head {
    display: flex; align-items: center; justify-content: space-between; gap: 12px;
    margin-bottom: 16px;
  }
  .eyebrow {
    font: 600 11px/1 var(--font-text);
    letter-spacing: .08em; text-transform: uppercase;
    color: var(--muted); margin-bottom: 6px;
  }
  .head h2 {
    font: 650 19px/1.2 var(--font-display);
    letter-spacing: -.01em;
  }

  /* Butonlar: hap (pill) şekli, durum katmanı ile hover/aktif geri bildirimi */
  .btn {
    position: relative; overflow: hidden;
    display: inline-flex; align-items: center; justify-content: center; gap: 8px;
    height: 40px; padding: 0 18px; border: 0; border-radius: 999px;
    font: 600 14px/1 var(--font-display); letter-spacing: .01em;
    color: var(--primary); background: var(--primary-soft);
    cursor: pointer;
    transition: transform .15s ease, background .2s ease, box-shadow .2s ease, opacity .2s ease;
  }
  .btn::after {
    content: ""; position: absolute; inset: 0;
    background: currentColor; opacity: 0; pointer-events: none;
    transition: opacity .2s ease;
  }
  .btn:hover::after { opacity: .08; }
  .btn:active::after { opacity: .16; }
  .btn:active { transform: scale(.97); }
  .btn:focus-visible { outline: 2px solid var(--primary); outline-offset: 3px; }
  .btn:disabled { cursor: not-allowed; opacity: .45; transform: none; }

  .btn-filled {
    background: var(--primary); color: var(--on-primary);
    box-shadow: 0 8px 22px rgba(169, 201, 255, .28);
  }
  .btn-filled:disabled { box-shadow: none; }
  .btn-ghost { background: transparent; color: var(--muted); }
  .btn-sm { height: 34px; padding: 0 14px; font-size: 13px; }
  .btn-icon { width: 36px; height: 36px; padding: 0; }
  .btn-block { width: 100%; }

  /* Cihaz listesi: satıra tıklayınca seçilir */
  .devices { flex: 1; overflow-y: auto; display: flex; flex-direction: column; gap: 10px; padding: 2px; }
  .device {
    display: flex; align-items: center; gap: 12px;
    background: rgba(255, 255, 255, .04);
    border: 1px solid var(--stroke); border-radius: 18px;
    padding: 14px 16px;
    cursor: pointer;
    transition: background .2s ease, border-color .2s ease;
  }
  .device:hover { background: rgba(255, 255, 255, .07); }
  .device.selected { background: var(--primary-soft); border-color: rgba(169, 201, 255, .45); }
  .device .ad { font: 600 15px/1.2 var(--font-display); flex: 1; min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .device .status {
    display: inline-flex; align-items: center; gap: 6px;
    padding: 4px 10px; border-radius: 999px;
    background: rgba(126, 230, 166, .12); color: var(--success);
    font: 600 12px/1 var(--font-text); flex-shrink: 0;
  }
  .device .status::before {
    content: ""; width: 6px; height: 6px; border-radius: 50%;
    background: var(--success); box-shadow: 0 0 8px var(--success);
  }

  /* Boş durum */
  .empty {
    margin: auto; padding: 24px;
    display: flex; flex-direction: column; align-items: center; gap: 6px;
    text-align: center; color: var(--muted);
  }
  .empty strong { font: 650 15px/1.3 var(--font-display); color: var(--text); }
  .empty-icon {
    width: 60px; height: 60px; margin-bottom: 10px;
    display: flex; align-items: center; justify-content: center;
    border-radius: 50%; background: var(--primary-soft); color: var(--primary);
  }
  .empty-icon svg { width: 26px; height: 26px; }

  /* Seçili cihaz paneli: üstte dosya gönderimi (sabit boy), altta sohbet (kalan alan) */
  .sel-panel { display: flex; flex-direction: column; flex: 1; min-height: 0; }
  .gonder { flex: none; }
  .work { height: 190px; flex: none; display: flex; gap: 14px; }

  /* Sürükle-bırak alanı */
  .drop {
    width: 230px; flex-shrink: 0;
    display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 4px;
    padding: 16px;
    border: 1.5px dashed rgba(169, 201, 255, .30); border-radius: 24px;
    background: rgba(255, 255, 255, .03);
    color: var(--muted); text-align: center; cursor: pointer;
    transition: border-color .2s ease, background .2s ease, transform .2s ease;
  }
  .drop:hover { border-color: rgba(169, 201, 255, .55); background: rgba(255, 255, 255, .05); }
  .drop:focus-visible { outline: 2px solid var(--primary); outline-offset: 3px; }
  .drop .plus {
    width: 52px; height: 52px; margin-bottom: 8px;
    display: flex; align-items: center; justify-content: center;
    border-radius: 50%; background: var(--primary-soft);
    color: var(--primary); font: 300 34px/1 var(--font-display);
  }
  .drop-title { font: 650 15px/1.3 var(--font-display); color: var(--text); }
  .drop-sub { font-size: 13px; }
  .drop.over {
    border-color: var(--accent); background: rgba(127, 227, 207, .10);
    transform: scale(1.015);
  }

  /* Dosya listesi */
  .files {
    flex: 1; min-width: 0;
    display: flex; flex-direction: column; gap: 12px;
    background: rgba(255, 255, 255, .03);
    border: 1px solid var(--stroke); border-radius: 24px;
    padding: 14px;
  }
  .file-list { flex: 1; overflow-y: auto; display: flex; flex-direction: column; gap: 8px; }
  .file {
    display: flex; justify-content: space-between; align-items: center; gap: 12px;
    padding: 10px 14px; border-radius: 16px;
    background: rgba(255, 255, 255, .05);
    animation: rise .25s ease;
  }
  .file .name { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; font-weight: 600; }
  .file .size { color: var(--muted); font-size: 13px; flex-shrink: 0; font-variant-numeric: tabular-nums; }
  @keyframes rise { from { opacity: 0; transform: translateY(4px); } to { opacity: 1; transform: none; } }

  /* Aktarım alt çubuğu: ilerleme + bilgi + ana eylem */
  .progress {
    height: 6px; margin-top: 12px; border-radius: 999px; overflow: hidden;
    background: rgba(255, 255, 255, .08); display: none;
  }
  .progress.gorunur { display: block; }
  .progress i {
    display: block; height: 100%; width: 0; border-radius: inherit;
    background: linear-gradient(90deg, var(--primary), var(--accent));
    transition: width .2s ease;
  }
  .foot { display: flex; justify-content: flex-end; align-items: center; gap: 14px; padding-top: 12px; }
  .foot .info { color: var(--muted); font-size: 13px; margin-right: auto; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }

  /* Sohbet paneli: gelen mesajlar solda, giden mesajlar sağda, sistem satırları ortada */
  .sohbet {
    flex: 1; min-height: 0;
    display: flex; flex-direction: column;
    margin-top: 16px;
    background: rgba(255, 255, 255, .03);
    border: 1px solid var(--stroke); border-radius: 24px;
    padding: 14px;
  }
  .sohbet .eyebrow { margin-bottom: 10px; padding-left: 4px; }
  .mesajlar {
    flex: 1; min-height: 0; overflow-y: auto;
    display: flex; flex-direction: column; gap: 8px;
    padding: 4px 4px 4px 2px;
    user-select: text;
  }
  .msg { display: flex; }
  .msg.giden { justify-content: flex-end; }
  .msg.sistem { justify-content: center; }
  .bubble {
    max-width: 72%;
    padding: 9px 14px;
    border-radius: 18px;
    white-space: pre-wrap; word-break: break-word;
    font-size: 14px; line-height: 1.4;
    animation: rise .2s ease;
  }
  .msg.gelen .bubble { background: rgba(255, 255, 255, .08); border-bottom-left-radius: 6px; }
  .msg.giden .bubble { background: var(--primary); color: var(--on-primary); border-bottom-right-radius: 6px; }
  .msg.sistem .bubble {
    max-width: 90%; padding: 4px 12px; border-radius: 999px;
    background: rgba(255, 255, 255, .04); color: var(--muted);
    font-size: 12.5px; text-align: center;
  }

  /* Mesaj yazma kutusu */
  .composer { display: flex; align-items: center; gap: 10px; margin-top: 12px; }
  .composer textarea {
    flex: 1; min-width: 0; height: 42px; max-height: 96px;
    resize: none; outline: none;
    padding: 10px 16px;
    border: 1px solid var(--stroke); border-radius: 21px;
    background: rgba(255, 255, 255, .05);
    color: var(--text); font: 500 14px/1.3 var(--font-text);
    user-select: text;
    transition: border-color .2s ease, background .2s ease;
  }
  .composer textarea::placeholder { color: var(--muted); }
  .composer textarea:focus { border-color: rgba(169, 201, 255, .55); background: rgba(255, 255, 255, .07); }
  .composer .btn { height: 42px; padding: 0 20px; }

  /* IP ekleme penceresi */
  .modal {
    position: fixed; inset: 0; z-index: 60;
    display: flex; align-items: center; justify-content: center;
    background: rgba(4, 7, 12, .55);
    backdrop-filter: blur(6px);
  }
  .modal-card {
    width: 340px; padding: 22px;
    display: flex; flex-direction: column; gap: 12px;
    background: rgba(24, 30, 42, .96);
    border: 1px solid var(--stroke); border-radius: 24px;
    box-shadow: 0 24px 60px rgba(0, 0, 0, .45);
  }
  .modal-card h2 { font: 650 19px/1.2 var(--font-display); }
  .modal-card input {
    height: 42px; padding: 0 16px; outline: none;
    border: 1px solid var(--stroke); border-radius: 21px;
    background: rgba(255, 255, 255, .05);
    color: var(--text); font: 500 14px/1 var(--font-text);
    user-select: text;
    transition: border-color .2s ease;
  }
  .modal-card input::placeholder { color: var(--muted); }
  .modal-card input:focus { border-color: rgba(169, 201, 255, .55); }
  .modal-hata { min-height: 18px; color: var(--danger); font-size: 13px; }
  .modal-foot { display: flex; justify-content: flex-end; gap: 10px; }

  /* Kısa bildirimler */
  .toast {
    position: fixed; left: 50%; bottom: 22px; z-index: 70;
    max-width: 70%; padding: 10px 18px; border-radius: 999px;
    background: rgba(20, 26, 38, .96); border: 1px solid var(--stroke);
    box-shadow: 0 12px 30px rgba(0, 0, 0, .4);
    font-size: 13px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
    opacity: 0; transform: translate(-50%, 12px); pointer-events: none;
    transition: opacity .25s ease, transform .25s ease;
  }
  .toast.goster { opacity: 1; transform: translate(-50%, 0); }

  .hidden { display: none !important; }

  /* Çok ince kaydırma çubukları */
  ::-webkit-scrollbar { width: 6px; }
  ::-webkit-scrollbar-thumb { background: rgba(255, 255, 255, .14); border-radius: 3px; }

  @media (prefers-reduced-motion: reduce) {
    *, *::before, *::after { transition: none !important; animation: none !important; }
  }
</style>
</head>
<body>
<div class="ust-kenar" id="ustKenar"></div>
<header class="titlebar" id="titlebar">
  <div class="brand"><span class="logo"></span><span>Yerelkeşif</span></div>
  <div class="wc">
    <button class="wc-btn" id="wcKucult" aria-label="Küçült" title="Küçült">
      <svg viewBox="0 0 12 12" fill="none" stroke="currentColor" stroke-width="1.2"><path d="M1 6h10"/></svg>
    </button>
    <button class="wc-btn" id="wcBuyult" aria-label="Büyüt" title="Büyüt">
      <svg class="ikon-max" viewBox="0 0 12 12" fill="none" stroke="currentColor" stroke-width="1.2"><rect x="1.5" y="1.5" width="9" height="9" rx="1.5"/></svg>
      <svg class="ikon-restore" viewBox="0 0 12 12" fill="none" stroke="currentColor" stroke-width="1.2"><rect x="3.5" y="0.5" width="8" height="8" rx="1.5"/><path d="M1.5 3.5v6a1 1 0 0 0 1 1h6"/></svg>
    </button>
    <button class="wc-btn wc-kapat" id="wcKapat" aria-label="Kapat" title="Kapat">
      <svg viewBox="0 0 12 12" fill="none" stroke="currentColor" stroke-width="1.2" stroke-linecap="round"><path d="M1.5 1.5l9 9M10.5 1.5l-9 9"/></svg>
    </button>
  </div>
</header>
<div class="app">

  <section class="panel left">
    <div class="head">
      <div>
        <div class="eyebrow">Yerel ağ</div>
        <h2>Cihazlar</h2>
      </div>
      <button class="btn btn-sm" id="ipEkle">+ IP Ekle</button>
    </div>
    <div class="devices" id="devices">
      <div class="empty" id="devEmpty">
        <div class="empty-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"><path d="M2 8.8a15 15 0 0 1 20 0M5 12.6a10 10 0 0 1 14 0M8.5 16.3a5 5 0 0 1 7 0"/><circle cx="12" cy="20" r="1" fill="currentColor"/></svg></div>
        <strong>Henüz cihaz yok</strong>
        <span>Ağdaki cihazlar burada görünecek.</span>
      </div>
    </div>
  </section>

  <section class="panel right">
    <div id="noSel" class="empty">
      <div class="empty-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="4" width="18" height="13" rx="3"/><path d="M8 21h8M12 17v4"/></svg></div>
      <strong>Bir cihaz seçin</strong>
      <span>Dosya göndermek veya mesajlaşmak için soldan bir cihaz seçin.</span>
    </div>

    <div id="sel" class="hidden sel-panel">
      <div class="head">
        <div>
          <div class="eyebrow">Hedef</div>
          <h2 id="selTitle"></h2>
        </div>
        <button class="btn btn-icon btn-ghost" id="kapat" aria-label="Kapat">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M6 6l12 12M18 6L6 18"/></svg>
        </button>
      </div>

      <div class="gonder">
        <div class="work">
          <div class="drop" id="drop" tabindex="0" role="button" aria-label="Dosya seç">
            <div class="plus">+</div>
            <div class="drop-title">Dosya sürükleyin</div>
            <div class="drop-sub">veya tıklayarak seçin</div>
          </div>
          <div class="files">
            <button class="btn btn-block" id="dosyaEkle">+ Dosya Ekle</button>
            <div class="file-list" id="fileList">
              <div class="empty" id="fileEmpty">
                <strong>Henüz dosya seçilmedi</strong>
                <span>Eklediğiniz dosyalar burada listelenir.</span>
              </div>
            </div>
          </div>
        </div>
        <div class="progress" id="prog"><i id="progBar"></i></div>
        <div class="foot">
          <span class="info" id="info"></span>
          <button class="btn btn-filled" id="baslat" disabled>Transferi Başlat</button>
        </div>
      </div>

      <div class="sohbet">
        <div class="eyebrow">Sohbet</div>
        <div class="mesajlar" id="mesajlar">
          <div class="empty" id="mesajBos">
            <strong>Henüz mesaj yok</strong>
            <span>Yazdığınız mesajlar ve gelen yanıtlar burada görünür.</span>
          </div>
        </div>
        <form class="composer" id="composer" autocomplete="off">
          <textarea id="mesajKutusu" rows="1" placeholder="Mesaj yazın… (Enter gönderir, Shift+Enter alt satır)"></textarea>
          <button class="btn btn-filled" id="gonder" type="submit">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 12l16-8-6 16-2-7z"/></svg>
            Gönder
          </button>
        </form>
      </div>
    </div>
  </section>

</div>

<div class="modal hidden" id="modal" role="dialog" aria-modal="true" aria-labelledby="modalBaslik">
  <form class="modal-card" id="ipForm" autocomplete="off">
    <div class="eyebrow">Manuel bağlantı</div>
    <h2 id="modalBaslik">IP adresi ekle</h2>
    <input id="ipGiris" type="text" inputmode="decimal" placeholder="192.168.1.42" spellcheck="false">
    <div class="modal-hata" id="ipHata"></div>
    <div class="modal-foot">
      <button type="button" class="btn btn-ghost btn-sm" id="ipIptal">İptal</button>
      <button type="submit" class="btn btn-filled btn-sm" id="ipOnay">Ekle</button>
    </div>
  </form>
</div>

<div class="toast" id="toast" role="status" aria-live="polite"></div>

<script>
/* ---------- C++ köprüsü ----------
   C++ tarafı w.bind("isim", ...) ile fonksiyonları window.isim olarak sunar ve sonucu Promise ile döndürür.
   C++ de window.cihazEkle gibi fonksiyonları çağırarak arayüzü günceller. */
const cpp = (ad, ...argumanlar) => window[ad](...argumanlar);
const $ = id => document.getElementById(id);

const state = { devices: [], selected: null, files: [], chats: {}, sending: false };

function fmtSize(b) {
  if (b == null) return "";
  const u = ["B", "KB", "MB", "GB"]; let i = 0;
  while (b >= 1024 && i < 3) { b /= 1024; i++; }
  return b.toFixed(i ? 1 : 0) + " " + u[i];
}

function toast(metin) {
  const t = $("toast");
  t.textContent = metin;
  t.classList.add("goster");
  clearTimeout(toast.zamanlayici);
  toast.zamanlayici = setTimeout(() => t.classList.remove("goster"), 3200);
}

/* ---------- Cihazlar ---------- */
function renderDevices() {
  const box = $("devices");
  box.querySelectorAll(".device").forEach(e => e.remove());
  $("devEmpty").classList.toggle("hidden", state.devices.length > 0);
  state.devices.forEach(c => {
    const d = document.createElement("div");
    d.className = "device" + (c.ip === state.selected ? " selected" : "");
    d.title = c.ip;
    d.innerHTML = '<span class="ad"></span><span class="status">Aktif</span>';
    d.querySelector(".ad").textContent = c.ad;
    d.onclick = () => selectDevice(c.ip);
    box.appendChild(d);
  });
}

/* C++ bir cihaz bulduğunda ya da ad değiştiğinde çağırır */
function cihazEkle(ip, ad) {
  const mevcut = state.devices.find(x => x.ip === ip);
  if (mevcut) mevcut.ad = ad; else state.devices.push({ ip, ad });
  renderDevices();
  if (state.selected === ip) $("selTitle").textContent = ad;
}

/* C++ cihazın bağlantısının koptuğunu bildirdiğinde çağırır */
function cihazKayboldu(ip) {
  state.devices = state.devices.filter(x => x.ip !== ip);
  renderDevices();
  if (state.selected === ip) {
    closeDevice();
    toast("Cihazla bağlantı kesildi.");
  }
}

function selectDevice(ip) {
  if (state.sending) { toast("Gönderim sürerken cihaz değiştirilemez."); return; }
  const c = state.devices.find(x => x.ip === ip);
  state.selected = ip;
  state.files = [];
  $("selTitle").textContent = c ? c.ad : ip;
  $("noSel").classList.add("hidden");
  $("sel").classList.remove("hidden");
  hideProgress();
  renderDevices();
  renderFiles();
  renderMesajlar();
}

function closeDevice() {
  state.selected = null;
  $("sel").classList.add("hidden");
  $("noSel").classList.remove("hidden");
  renderDevices();
}

/* IP ekleme penceresi */
function ipModalAc() {
  $("ipGiris").value = "";
  $("ipHata").textContent = "";
  $("modal").classList.remove("hidden");
  $("ipGiris").focus();
}
function ipModalKapat() {
  $("modal").classList.add("hidden");
}
$("ipEkle").onclick = ipModalAc;
$("ipIptal").onclick = ipModalKapat;
$("modal").addEventListener("mousedown", e => { if (e.target === $("modal")) ipModalKapat(); });
document.addEventListener("keydown", e => {
  if (e.key === "Escape" && !$("modal").classList.contains("hidden")) ipModalKapat();
});
$("ipForm").onsubmit = async e => {
  e.preventDefault();
  const ip = $("ipGiris").value.trim();
  const hata = await cpp("cppIpEkle", ip);
  if (hata) { $("ipHata").textContent = hata; return; }
  ipModalKapat();
  toast(ip + " sorgulanıyor…");
};

$("kapat").onclick = () => {
  if (state.sending) toast("Gönderim sürerken panel kapatılamaz.");
  else closeDevice();
};

/* ---------- Dosyalar ---------- */
function renderFiles() {
  const list = $("fileList");
  list.querySelectorAll(".file").forEach(e => e.remove());
  $("fileEmpty").classList.toggle("hidden", state.files.length > 0);
  state.files.forEach(f => {
    const row = document.createElement("div");
    row.className = "file";
    row.title = f.path || f.name;
    row.innerHTML = '<span class="name"></span><span class="size"></span>';
    row.querySelector(".name").textContent = f.name;
    row.querySelector(".size").textContent = fmtSize(f.size);
    list.appendChild(row);
  });
  if (!state.sending) $("info").textContent = state.files.length ? state.files.length + " dosya hazır" : "";
  guncelleDugmeler();
}

function guncelleDugmeler() {
  $("baslat").disabled = state.sending || !state.selected || state.files.length === 0;
  $("dosyaEkle").disabled = state.sending;
}

function dosyaEkle(liste) {
  if (!state.selected) return;
  liste.forEach(f => { if (!state.files.some(x => x.path === f.path)) state.files.push(f); });
  renderFiles();
}

async function dosyaSec() {
  if (state.sending) return;
  const secilen = await cpp("cppDosyaSec");
  if (Array.isArray(secilen)) dosyaEkle(secilen);
}

$("dosyaEkle").onclick = dosyaSec;
$("drop").onclick = dosyaSec;
$("drop").onkeydown = e => { if (e.key === "Enter" || e.key === " ") { e.preventDefault(); dosyaSec(); } };

function showProgress(pct) {
  $("prog").classList.add("gorunur");
  $("progBar").style.width = Math.min(100, Math.max(0, pct)) + "%";
}
function hideProgress() {
  $("prog").classList.remove("gorunur");
  $("progBar").style.width = "0%";
}

async function transferBaslat() {
  if (!state.selected || state.sending || state.files.length === 0) return;
  state.sending = true;
  guncelleDugmeler();
  $("info").textContent = "Gönderim başlatılıyor…";
  showProgress(0);
  await cpp("cppTransferBaslat", state.selected, ...state.files.map(f => f.path));
}
$("baslat").onclick = transferBaslat;

/* C++ aktarım durumunu bildirir.
   asama: "ilerleme" | "tamam" | "hata". sonMu: bu toplu işlemdeki son olay. */
window.transferDurumu = (ip, gelen, ad, yuzde, asama, sira, toplam, detay, sonMu) => {
  const secili = ip === state.selected;

  if (asama === "ilerleme") {
    if (secili) {
      showProgress(yuzde);
      $("info").textContent = (gelen ? "Alınıyor " : "Gönderiliyor ") + sira + "/" + toplam + ": " + ad + "  %" + Math.round(yuzde);
    }
    return;
  }

  if (asama === "tamam") {
    mesajEkle(ip, "sistem", gelen ? "Dosya alındı: " + ad + " (" + detay + ")" : "Dosya gönderildi: " + ad);
    if (gelen) toast("Dosya alındı: " + ad);
  } else {
    mesajEkle(ip, "sistem", "Aktarım başarısız (" + ad + "): " + detay);
    toast("Aktarım başarısız: " + detay);
  }

  if (gelen) {
    if (secili) {
      showProgress(asama === "tamam" ? 100 : 0);
      $("info").textContent = asama === "tamam" ? "Dosya alındı: " + ad : "Dosya alınamadı";
      setTimeout(() => { if (!state.sending) hideProgress(); }, 1500);
    }
    return;
  }

  if (sonMu) {
    state.sending = false;
    if (asama === "tamam") state.files = [];
    if (secili) {
      renderFiles();
      $("info").textContent = asama === "tamam" ? "Transfer tamamlandı" : "Transfer tamamlanamadı";
      setTimeout(() => { if (!state.sending) hideProgress(); }, 1500);
    }
    guncelleDugmeler();
  } else if (secili && asama === "tamam") {
    showProgress(100);
  }
};

/* Sürükle-bırak: pencerenin her yerinde varsayılan davranış kapatılır, yoksa WebView dosyaya gidip sayfayı değiştirir.
   Dosyalar tam yoluyla postMessageWithAdditionalObjects ile C++'a gider; C++ window.dosyalarEklendi ile geri döner. */
const drop = $("drop");
["dragenter", "dragover"].forEach(t => document.addEventListener(t, e => {
  e.preventDefault();
  const kabul = !!state.selected && !state.sending;
  if (e.dataTransfer) e.dataTransfer.dropEffect = kabul ? "copy" : "none";
  if (kabul) drop.classList.add("over");
}));
document.addEventListener("dragleave", e => { if (!e.relatedTarget) drop.classList.remove("over"); });
document.addEventListener("drop", e => {
  e.preventDefault();
  drop.classList.remove("over");
  if (!state.selected || state.sending || !e.dataTransfer || !e.dataTransfer.files.length) return;
  const wv = window.chrome && window.chrome.webview;
  if (wv && wv.postMessageWithAdditionalObjects) {
    wv.postMessageWithAdditionalObjects('{"id":"__drop","method":"__dosyaBirak","params":[]}', [...e.dataTransfer.files]);
  }
});

window.dosyalarEklendi = dosyaEkle;

/* ---------- Sohbet ---------- */
function renderMesajlar() {
  const box = $("mesajlar");
  box.querySelectorAll(".msg").forEach(e => e.remove());
  const liste = state.chats[state.selected] || [];
  $("mesajBos").classList.toggle("hidden", liste.length > 0);
  liste.forEach(m => {
    const satir = document.createElement("div");
    satir.className = "msg " + m.yon;
    const balon = document.createElement("div");
    balon.className = "bubble";
    balon.textContent = m.metin;
    satir.appendChild(balon);
    box.appendChild(satir);
  });
  box.scrollTop = box.scrollHeight;
}

/* yon: "gelen" | "giden" | "sistem". Sohbetler oturum boyunca bellekte tutulur. */
function mesajEkle(ip, yon, metin) {
  (state.chats[ip] = state.chats[ip] || []).push({ yon, metin });
  if (ip === state.selected) renderMesajlar();
}

function mesajGonder() {
  const kutu = $("mesajKutusu");
  const metin = kutu.value.trim();
  if (!metin || !state.selected) return;
  mesajEkle(state.selected, "giden", metin);
  kutu.value = "";
  cpp("cppMesajGonder", state.selected, metin);
}
$("composer").onsubmit = e => { e.preventDefault(); mesajGonder(); };
$("mesajKutusu").onkeydown = e => {
  if (e.key === "Enter" && !e.shiftKey && !e.isComposing) { e.preventDefault(); mesajGonder(); }
};

window.mesajAl = (ip, metin) => {
  mesajEkle(ip, "gelen", metin);
  if (ip !== state.selected) {
    const c = state.devices.find(x => x.ip === ip);
    toast((c ? c.ad : ip) + ": " + metin.slice(0, 60));
  }
};

/* C++ bildirimleri: hata ve bilgi metinleri */
window.bildirim = (ip, metin) => {
  mesajEkle(ip, "sistem", metin);
  toast(metin);
};

/* ---------- Pencere ---------- */
const titlebar = $("titlebar");
titlebar.addEventListener("mousedown", e => {
  if (e.button === 0 && !e.target.closest(".wc")) cpp("pencereSurukle");
});
titlebar.addEventListener("dblclick", e => {
  if (!e.target.closest(".wc")) cpp("pencereBuyult");
});
$("wcKucult").onclick = () => cpp("pencereKucult");
$("wcBuyult").onclick = () => cpp("pencereBuyult");
$("wcKapat").onclick = () => cpp("pencereKapat");
$("ustKenar").addEventListener("mousedown", e => {
  if (e.button === 0) cpp("pencereBoyutlandir", "ust");
});
window.pencereDurumu = buyuk => document.body.classList.toggle("buyuk", !!buyuk);

/* İlk kare çizildikten sonra C++'a "hazırım" de: pencere o zaman gösterilir ve ağ servisi başlar */
let hazirGitti = false;
const hazirDe = () => {
  if (hazirGitti) return;
  hazirGitti = true;
  document.body.classList.add("hazir");
  cpp("uygulamaHazir");
};
window.addEventListener("load", () => {
  renderDevices();
  renderFiles();
  requestAnimationFrame(() => requestAnimationFrame(hazirDe));
  setTimeout(hazirDe, 250);   // Pencere gizliyken tarayıcı kareleri durdurursa yedek
});
</script>
</body>
</html>
)HTML";
