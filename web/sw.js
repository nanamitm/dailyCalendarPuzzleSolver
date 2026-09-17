// Offline cache. Bump CACHE when any asset below changes.
const CACHE = 'puzzlesolver-v1';
const ASSETS = [
    './',
    'index.html',
    'styles.css',
    'app.js',
    'board.js',
    'solver-core.js',
    'solver-worker.js',
    'manifest.webmanifest',
    'icons/icon-192.png',
    'icons/icon.svg',
];

self.addEventListener('install', e => {
    e.waitUntil(caches.open(CACHE).then(c => c.addAll(ASSETS)).then(() => self.skipWaiting()));
});

self.addEventListener('activate', e => {
    e.waitUntil(
        caches.keys()
            .then(keys => Promise.all(keys.filter(k => k !== CACHE).map(k => caches.delete(k))))
            .then(() => self.clients.claim()));
});

// Network first, falling back to the cache so a new deploy is picked up quickly.
self.addEventListener('fetch', e => {
    if (e.request.method !== 'GET') return;
    e.respondWith(
        fetch(e.request)
            .then(res => {
                const copy = res.clone();
                caches.open(CACHE).then(c => c.put(e.request, copy)).catch(() => {});
                return res;
            })
            .catch(() => caches.match(e.request).then(r => r || caches.match('index.html'))));
});
