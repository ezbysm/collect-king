// 수집왕 — 사운드 (Web Audio API로 코드에서 직접 만든 칩튠과 효과음)
'use strict';

const Sound = (() => {
  const PREF_KEY = 'sujipking_sound_v1';
  let ctx = null, master, bgmGain, sfxGain, echo;
  let muted = false;
  let wantBgm = null, curBgm = null;
  let seq = null;            // 재생 중인 배경음 시퀀서
  let noiseBuf = null;
  const waves = {};

  try { muted = localStorage.getItem(PREF_KEY) === 'off'; } catch (e) { /* noop */ }

  // ---------- 준비 ----------
  function ensure() {
    if (ctx) return ctx;
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return null;
    ctx = new AC();
    master = ctx.createGain(); master.gain.value = muted ? 0 : 0.7; master.connect(ctx.destination);
    bgmGain = ctx.createGain(); bgmGain.gain.value = 0.32; bgmGain.connect(master);
    sfxGain = ctx.createGain(); sfxGain.gain.value = 0.6; sfxGain.connect(master);
    // 리드 악기용 메아리
    echo = ctx.createDelay(1); echo.delayTime.value = 0.33;
    const fb = ctx.createGain(); fb.gain.value = 0.28;
    const wet = ctx.createGain(); wet.gain.value = 0.35;
    echo.connect(fb); fb.connect(echo); echo.connect(wet); wet.connect(bgmGain);
    // 노이즈 버퍼
    noiseBuf = ctx.createBuffer(1, ctx.sampleRate, ctx.sampleRate);
    const d = noiseBuf.getChannelData(0);
    for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
    // 펄스파 (듀티 25%, 12.5%)
    for (const duty of [0.25, 0.125]) {
      const n = 32, re = new Float32Array(n), im = new Float32Array(n);
      for (let k = 1; k < n; k++) im[k] = (2 / (k * Math.PI)) * Math.sin(k * Math.PI * duty);
      waves['p' + duty] = ctx.createPeriodicWave(re, im);
    }
    document.addEventListener('visibilitychange', () => {
      if (!ctx) return;
      if (document.hidden) ctx.suspend(); else ctx.resume();
    });
    return ctx;
  }

  // 모바일은 사용자가 화면을 만져야 소리가 난다
  function unlock() {
    if (!ensure()) return;
    if (ctx.state === 'suspended') ctx.resume();
    if (wantBgm && curBgm !== wantBgm) startBgm(wantBgm);
  }

  function setMuted(v) {
    muted = v;
    try { localStorage.setItem(PREF_KEY, v ? 'off' : 'on'); } catch (e) { /* noop */ }
    if (ctx) master.gain.setTargetAtTime(v ? 0 : 0.7, ctx.currentTime, 0.05);
  }
  function isMuted() { return muted; }

  const freq = m => 440 * Math.pow(2, (m - 69) / 12);

  // ---------- 기본 음원 ----------
  function tone(f, t, dur, { type = 'square', vol = 0.2, dest, attack = 0.005, release = 0.06, slideTo, wave } = {}) {
    const o = ctx.createOscillator();
    if (wave) o.setPeriodicWave(wave); else o.type = type;
    o.frequency.setValueAtTime(f, t);
    if (slideTo) o.frequency.exponentialRampToValueAtTime(slideTo, t + dur);
    const g = ctx.createGain();
    g.gain.setValueAtTime(0, t);
    g.gain.linearRampToValueAtTime(vol, t + attack);
    g.gain.setValueAtTime(vol, Math.max(t + attack, t + dur - release));
    g.gain.linearRampToValueAtTime(0, t + dur);
    o.connect(g); g.connect(dest || sfxGain);
    o.start(t); o.stop(t + dur + 0.02);
    return g;
  }
  function noise(t, dur, { vol = 0.2, filter = 'highpass', fq = 6000, q = 1, dest, decay = true } = {}) {
    const src = ctx.createBufferSource();
    src.buffer = noiseBuf; src.loop = true;
    const f = ctx.createBiquadFilter(); f.type = filter; f.frequency.value = fq; f.Q.value = q;
    const g = ctx.createGain();
    g.gain.setValueAtTime(vol, t);
    if (decay) g.gain.exponentialRampToValueAtTime(0.001, t + dur);
    else g.gain.setValueAtTime(0, t + dur);
    src.connect(f); f.connect(g); g.connect(dest || sfxGain);
    src.start(t, Math.random() * 0.5); src.stop(t + dur + 0.02);
  }

  // ---------- 배경음 ----------
  // 음표 문자열: 'E5 . G5 -' (.=이어서, -=쉼표)
  const NOTE = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 };
  function parse(str) {
    return str.trim().split(/\s+/).map(tok => {
      if (tok === '.') return '.';
      if (tok === '-') return null;
      const m = tok.match(/^([A-G])(b|#)?(\d)$/);
      return (+m[3] + 1) * 12 + NOTE[m[1]] + (m[2] === '#' ? 1 : m[2] === 'b' ? -1 : 0);
    });
  }
  function arp(chords, pattern = [0, 7, 12, 7, 0, 7, 12, 7]) {
    // chords: 'C3 A2 F2 G2 ...' 마디마다 근음, 근음-5도-옥타브 아르페지오
    return parse(chords).flatMap(root => pattern.map(iv => (iv === null ? null : root + iv)));
  }

  const TRACKS = {
    // 방: 저녁 햇살 같은 잔잔한 곡
    room: {
      bpm: 88, leadWave: 'p0.25', leadVol: 0.07, bassVol: 0.11, hat: true,
      lead: parse(`
        E5 . G5 . A5 G5 E5 .   C5 . . . A4 . C5 .   D5 . F5 . E5 D5 C5 .   D5 . . . - - G4 .
        E5 . G5 . C6 . B5 .    G5 . . . E5 . D5 .   C5 . D5 . E5 . F5 E5   D5 . . . . . - -
        C5 . E5 . G5 . E5 .    A4 . C5 . E5 . D5 .  F4 . A4 . C5 . A4 .    B4 . D5 . G5 . F5 .
        E5 . . . D5 . C5 .     B4 . . . G4 . B4 .   A4 . C5 . F5 . E5 .    D5 . . . C5 . . .`),
      bass: arp('C3 A2 F2 G2 C3 E2 F2 G2 C3 A2 F2 G2 C3 E2 F2 G2'),
    },
    // 히로인: 조금 느리고 달콤하게
    her: {
      bpm: 76, leadWave: 'triangle', leadVol: 0.16, bassVol: 0.1, hat: false, echo: true,
      lead: parse(`
        A5 . . G5 F5 . C5 .    D5 . F5 . A5 . G5 .   F5 . . D5 Bb4 . D5 .   C5 . . . E5 . G5 .
        A5 . C6 . A5 . F5 .    E5 . . . C5 . E5 .    D5 . F5 . Bb5 . A5 .   G5 . . . . . - -`),
      bass: arp('F2 D2 Bb2 C3 F2 A2 Bb2 C3', [0, 7, 12, 16, 12, 7, 12, 7]),
    },
    // 엔딩: 느리게, 메아리를 걸어서
    ending: {
      bpm: 68, leadWave: 'triangle', leadVol: 0.17, bassVol: 0.1, hat: false, echo: true,
      lead: parse(`
        G5 . . . E5 . G5 .    D6 . . . B5 . G5 .    C6 . . . A5 . E5 .    B5 . . . G5 . . .
        A5 . . . F5 . A5 .    G5 . . . E5 . C5 .    F5 . A5 . C6 . A5 .   G5 . . . . . - -`),
      bass: arp('C3 G2 A2 E2 F2 C3 F2 G2', [0, 7, 12, 7, 0, 7, 12, 7]),
    },
    // 타이틀: 방 테마를 조금 더 밝게
    title: null,
  };
  TRACKS.title = { ...TRACKS.room, bpm: 100, leadWave: 'p0.125', leadVol: 0.06, echo: true };

  function startBgm(name) {
    stopSeq();
    curBgm = name;
    const tr = TRACKS[name];
    if (!tr || !ctx) return;
    const stepDur = 60 / tr.bpm / 2; // 8분음표
    const len = tr.lead.length;
    let step = 0, next = ctx.currentTime + 0.1;
    const fadeIn = ctx.createGain();
    fadeIn.gain.setValueAtTime(0, ctx.currentTime);
    fadeIn.gain.linearRampToValueAtTime(1, ctx.currentTime + 1.2);
    fadeIn.connect(bgmGain);
    const holdLen = (arr, i) => { let n = 1; while (arr[(i + n) % arr.length] === '.' && n < 16) n++; return n; };
    const timer = setInterval(() => {
      if (ctx.state !== 'running') { next = ctx.currentTime + 0.1; return; }
      while (next < ctx.currentTime + 0.25) {
        const i = step % len;
        const ln = tr.lead[i];
        if (typeof ln === 'number') {
          const d = holdLen(tr.lead, i) * stepDur;
          const g = tone(freq(ln), next, d * 0.95, { wave: waves[tr.leadWave], type: tr.leadWave, vol: tr.leadVol, dest: fadeIn, release: Math.min(0.15, d / 3) });
          if (tr.echo) g.connect(echo);
        }
        const bn = tr.bass[i % tr.bass.length];
        if (typeof bn === 'number') tone(freq(bn), next, stepDur * 0.9, { type: 'triangle', vol: tr.bassVol, dest: fadeIn, release: 0.04 });
        if (tr.hat && i % 2 === 1) noise(next, 0.03, { vol: 0.025, dest: fadeIn });
        next += stepDur; step++;
      }
    }, 50);
    seq = { timer, out: fadeIn };
  }
  function stopSeq() {
    if (!seq) return;
    clearInterval(seq.timer);
    const out = seq.out;
    out.gain.setTargetAtTime(0, ctx.currentTime, 0.25);
    setTimeout(() => out.disconnect(), 1500);
    seq = null;
  }

  function bgm(name) {
    if (wantBgm === name) return;
    wantBgm = name;
    if (ctx && ctx.state === 'running') startBgm(name);
  }

  // ---------- 효과음 ----------
  let lastBlip = 0;
  const SFX = {
    click(t) { tone(1300, t, 0.025, { vol: 0.06, release: 0.02 }); },
    blip(t) {
      if (t - lastBlip < 0.05) return;
      lastBlip = t;
      tone(520 + Math.random() * 160, t, 0.02, { type: 'triangle', vol: 0.05, release: 0.015 });
    },
    notice(t) {
      tone(880, t, 0.5, { type: 'sine', vol: 0.12, release: 0.45 });
      tone(1320, t + 0.06, 0.45, { type: 'sine', vol: 0.08, release: 0.4 });
    },
    error(t) {
      tone(440, t, 0.28, { vol: 0.08, release: 0.2 });
      tone(466, t, 0.28, { vol: 0.08, release: 0.2 });
    },
    coin(t) { tone(988, t, 0.08, { vol: 0.08 }); tone(1319, t + 0.08, 0.3, { vol: 0.08, release: 0.25 }); },
    done(t) { [72, 76, 79, 84].forEach((m, i) => tone(freq(m), t + i * 0.07, 0.18, { wave: waves['p0.25'], vol: 0.08, release: 0.1 })); },
    item(t) { [79, 84, 88, 91].forEach((m, i) => tone(freq(m), t + i * 0.05, 0.25, { type: 'triangle', vol: 0.12, release: 0.2 })); },
    startup(t) {
      [56, 63, 68, 72, 75].forEach((m, i) => tone(freq(m), t + i * 0.13, 1.8 - i * 0.13, { type: 'triangle', vol: 0.09, attack: 0.05, release: 1.0 }));
    },
    off(t) { tone(900, t, 0.25, { type: 'sine', vol: 0.12, slideTo: 60 }); noise(t, 0.15, { vol: 0.05 }); },
    thump(t) { tone(140, t, 0.22, { type: 'sine', vol: 0.3, slideTo: 50 }); },
    alarm(t) { for (let i = 0; i < 4; i++) tone(i % 2 ? 660 : 880, t + i * 0.16, 0.14, { vol: 0.07 }); },
    tray(t) { noise(t, 0.12, { vol: 0.15, filter: 'lowpass', fq: 900 }); tone(180, t + 0.1, 0.06, { vol: 0.06 }); },
    burn(t) {
      const o = ctx.createOscillator(); o.type = 'sawtooth';
      o.frequency.setValueAtTime(60, t); o.frequency.exponentialRampToValueAtTime(240, t + 0.8);
      const f = ctx.createBiquadFilter(); f.type = 'lowpass'; f.frequency.value = 500;
      const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.06, t + 0.3); g.gain.setValueAtTime(0.06, t + 1.3); g.gain.linearRampToValueAtTime(0, t + 1.6);
      o.connect(f); f.connect(g); g.connect(sfxGain); o.start(t); o.stop(t + 1.7);
    },
    hdd(t, dur = 2.5) {
      let x = t;
      while (x < t + dur) { noise(x, 0.012, { vol: 0.12, filter: 'bandpass', fq: 2500 + Math.random() * 2000, q: 2 }); x += 0.04 + Math.random() * 0.12; }
    },
    scanner(t) {
      const o = ctx.createOscillator(); o.type = 'square';
      o.frequency.setValueAtTime(110, t); o.frequency.linearRampToValueAtTime(130, t + 1.4);
      const f = ctx.createBiquadFilter(); f.type = 'lowpass'; f.frequency.value = 350;
      const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.05, t + 0.1); g.gain.setValueAtTime(0.05, t + 1.4); g.gain.linearRampToValueAtTime(0, t + 1.6);
      o.connect(f); f.connect(g); g.connect(sfxGain); o.start(t); o.stop(t + 1.7);
    },
    // 56k 모뎀 접속음 (흉내)
    modem(t, short = false) {
      let x = t;
      if (!short) { tone(350, x, 0.6, { type: 'sine', vol: 0.06 }); tone(440, x, 0.6, { type: 'sine', vol: 0.06 }); x += 0.7; }
      const DTMF = [[697, 1209], [770, 1336], [852, 1477], [697, 1336], [941, 1336], [770, 1209], [852, 1336]];
      for (const [a, b] of (short ? DTMF.slice(0, 3) : DTMF)) {
        tone(a, x, 0.08, { type: 'sine', vol: 0.06, release: 0.01 }); tone(b, x, 0.08, { type: 'sine', vol: 0.06, release: 0.01 });
        x += 0.13;
      }
      x += 0.2;
      tone(2100, x, short ? 0.4 : 0.9, { type: 'sine', vol: 0.05 }); x += short ? 0.45 : 0.95;
      const end = x + (short ? 0.8 : 1.6);
      while (x < end) {
        const f1 = [980, 1180, 1650, 1850, 2250, 2400][(Math.random() * 6) | 0];
        tone(f1, x, 0.07, { type: 'sine', vol: 0.05, release: 0.01, slideTo: f1 * (0.9 + Math.random() * 0.3) });
        if (Math.random() < 0.5) noise(x, 0.07, { vol: 0.03, filter: 'bandpass', fq: 1800, q: 0.7 });
        x += 0.06;
      }
      noise(x, short ? 0.5 : 1.0, { vol: 0.05, filter: 'bandpass', fq: 2000, q: 0.5 });
    },
  };

  function sfx(name, ...args) {
    if (muted || !ensure() || ctx.state !== 'running') return;
    try { SFX[name](ctx.currentTime + 0.01, ...args); } catch (e) { /* 소리 실패는 무시 */ }
  }

  // 테스트용: 효과음을 예외 처리 없이 재생하고, 출력 크기를 잴 수 있게 한다
  let analyser = null;
  const debug = {
    sfxRaw(name, ...args) { SFX[name](ctx.currentTime + 0.01, ...args); },
    level() {
      if (!analyser) { analyser = ctx.createAnalyser(); master.connect(analyser); }
      const d = new Float32Array(analyser.fftSize); analyser.getFloatTimeDomainData(d);
      return Math.sqrt(d.reduce((a, v) => a + v * v, 0) / d.length);
    },
    state: () => ({ ctx: ctx && ctx.state, bgm: curBgm }),
  };

  return { unlock, bgm, sfx, setMuted, isMuted, debug };
})();
