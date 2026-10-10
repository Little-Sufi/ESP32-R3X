// ESP32-R3X Mobile Companion Service Worker (v3.1.0 Network-First)
const CACHE_NAME = 'r3x-mobile-v3.1.0';
const ASSETS_TO_CACHE = [
  './',
  './index.html',
  './app.css',
  './app.js',
  './manifest.json',
  './icons/icon.svg'
];

self.addEventListener('install', (event) => {
  self.skipWaiting();
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => cache.addAll(ASSETS_TO_CACHE))
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) => {
      return Promise.all(
        keys.map((key) => {
          if (key !== CACHE_NAME) {
            return caches.delete(key);
          }
        })
      );
    }).then(() => self.clients.claim())
  );
});

self.addEventListener('fetch', (event) => {
  const url = event.request.url;

  // Never cache API, hardware telemetry, or live commands
  if (url.includes('/api/') || 
      url.includes('/status') || 
      url.includes('/cmd') || 
      url.includes('/ping') ||
      url.includes('192.168.4.1')) {
    return;
  }

  // Network-First for scripts, styles, and HTML
  event.respondWith(
    fetch(event.request)
      .then((networkResponse) => {
        if (networkResponse && networkResponse.status === 200 && event.request.method === 'GET') {
          const clone = networkResponse.clone();
          caches.open(CACHE_NAME).then((cache) => cache.put(event.request, clone));
        }
        return networkResponse;
      })
      .catch(() => {
        return caches.match(event.request).then((cached) => cached || caches.match('./index.html'));
      })
  );
});
