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

  // colors as requested
  const COLORS = {
    lipo: '#27ae60',      // green
    dex: '#ff69b4',       // pink
    carnivore: '#e74c3c', // red
    omnivore: '#f1c40f',  // yellow
    cannibal: '#9b59b6'   // purple
  };
  
