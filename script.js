(() => {
  // ---------- CONFIG ----------
  const MUTATION_RATE = 0.16;        // ±16%
  const LIPO_SPAWN_MS = 100;         // 0.1s
  const SAVE_MS = 5000;              // autosave interval
  const DEX_REPRO_EAT = 3;           // dex reproduces after eating 3 lipos
  const REPRO_HUNGER = 90;           // other species reproduce when hunger > this
  const POP_PAUSE_LIMIT = 1000;
  const TAB_H = 44;
  const INITIAL = { lipo: 20, dex: 10, carnivore: 5, omnivore: 5, cannibal: 5 };
  const MAX_SAVE = 2000;

  // colors 
  const COLORS = {
    lipo: '#27ae60',      // green
    dex: '#ff69b4',       // pink
    carnivore: '#e74c3c', // red
    omnivore: '#f1c40f',  // yellow
    cannibal: '#9b59b6'   // purple
  };

    // Diet rules:
  // carnivore eats all other species except cannibals
  const DIET = {
    dex: ['lipo'],
    carnivore: ['lipo','dex','omnivore','cannibal'], // not cannibal
    omnivore: ['lipo','dex'],
    cannibal: ['dex','carnivore','omnivore','cannibal'] // cannibal eats other animals (not lipos)
  };
  
  // ---------- DOM refs & state ----------
  const area = document.getElementById('area');
  const countsEl = document.getElementById('counts');
  const pauseBtn = document.getElementById('pauseBtn');
  const clearBtn = document.getElementById('clearBtn');

  let paused = false;
  pauseBtn.onclick = () => { paused = !paused; pauseBtn.textContent = paused ? 'Resume' : 'Pause'; };
  clearBtn.onclick = () => { if (confirm('Clear saved simulation data?')) { localStorage.removeItem('dex_lipo_state'); location.reload(); } };

  let animals = []; // moving creatures: dex, carnivore, omnivore, cannibal
  let lipos = [];   // stationary Lipo objects
  let lastLipoSpawn = performance.now();
  let lastSave = performance.now();

  // ---------- helpers ----------
  const uid = () => Math.random().toString(36).slice(2,9);
  const rand = (a,b) => Math.random()*(b-a)+a;
  const clamp = (v,a,b) => Math.max(a,Math.min(b,v));
  function hexToRgba(hex,alpha){
    const n = parseInt(hex.slice(1),16);
    return `rgba(${(n>>16)&255},${(n>>8)&255},${n&255},${alpha})`;
                           }

                 // ---------- spawn / DOM creation ----------
  const DEFAULT_LIPO_W = 30, DEFAULT_LIPO_H = 30;
  const DEFAULT_ANIMAL_W = 36, DEFAULT_ANIMAL_H = 36;

  function spawnLipo(saved){
    const w = saved?.width ?? DEFAULT_LIPO_W;
    const h = saved?.height ?? DEFAULT_LIPO_H;
    const x = saved?.x ?? rand(w/2, area.clientWidth - w/2);
    const y = saved?.y ?? rand(h/2, area.clientHeight - h/2);
    const el = document.createElement('div');
    el.className = 'entity lipo';
    el.style.width = w + 'px';
    el.style.height = h + 'px';
    el.style.background = COLORS.lipo;
    el.style.borderRadius = '6px';
    el.style.zIndex = 4;
    area.appendChild(el);
    const L = { id: uid(), x, y, width: w, height: h, el };
    positionLipo(L);
    lipos.push(L);
    return L;
  }
  function positionLipo(l){ if (l.el) { l.el.style.left = l.x + 'px'; l.el.style.top = l.y + 'px'; } }
  
