# Herbivore & Herbs Evolution Simulator

A browser UI (HTML/CSS/JavaScript) served by a C++17 backend. The C++ process owns simulation state and runs movement, perception, food seeking, predator avoidance, feeding, mutation, reproduction, hunger, lifespan, population counts and autosaving. JavaScript polls `/api/state` and renders the returned data; it does not calculate AI decisions or entity positions.

## Requirements
- Linux (uses POSIX sockets)
- C++17 compiler, CMake, pthreads

## Quick Run
From this directory:

```bash
cmake -S . -B build
cmake --build build -j
./build/herbivore_herbs_server
```

Open http://localhost:8091. Keep the process running while using the page. Stop with Ctrl+C.

## API
- `GET /api/state` - complete entity positions, traits, counts and simulation time
- `POST /api/pause` - toggle pause/resume
- `POST /api/reset` - reset to initial population
- `POST /api/save` - save current state

The server autosaves to `herbivore_herbs_state.json` every five seconds. It currently starts a fresh simulation on launch; autosave writing is implemented, but loading that file at startup is not yet implemented.

## Notes
The server is a compact POSIX HTTP server, so this starter targets Linux/Fedora. It intentionally avoids external C++ libraries. The source is a functional architecture prototype, not a hardened public-facing web server. Keep it on localhost unless you add proper security and request handling.
