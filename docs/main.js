(function () {
  const REPO_API = "https://api.github.com/repos/Little-Sufi/ESP32-R3X";

  const BOARD_CONFIG = {
    v3: {
      name: "ESP32-R3X v3.0 Powerhouse",
      manifestFile: "manifest_v3.json",
      file: "ESP32-R3X-v3.0-merged.bin",
      binPath: "firmware/ESP32-R3X-v3.0-merged.bin",
      chip: "ESP32-S3",
      chipFamily: "ESP32-S3",
      version: "v3.0",
      tagText: "v3.0 (Powerhouse)",
      meta: "ESP32-S3 · 2.0 MB · CAN Bus + Sub-GHz Waterfall + Tools Next/Prev Page"
    },
    v2: {
      name: "ESP32-R3X v2.0 Stable",
      manifestFile: "manifest_v2.json",
      file: "ESP32-R3X-v2.0-merged.bin",
      binPath: "firmware/ESP32-R3X-v2.0-merged.bin",
      chip: "ESP32-S3",
      chipFamily: "ESP32-S3",
      version: "v2.0",
      tagText: "v2.0 (Stable)",
      meta: "ESP32-S3 · 2.0 MB · Multi-band RF Baseline + PCAP + WiGLE Wardriving"
    }
  };

  let repoVersion = "3.0";
  let firmwareReady = true;

  function formatCount(n) {
    if (n >= 10000) return `${Math.round(n / 1000)}k`;
    if (n >= 1000) return `${(n / 1000).toFixed(1).replace(/\.0$/, "")}k`;
    return String(n);
  }

  function applyVersionLabels() {
    document.querySelectorAll("[data-repo-version]").forEach((el) => {
      el.textContent = `v${repoVersion}`;
    });
  }

  function applyRepoStats(stars, forks) {
    const starsEl = document.getElementById("repo-stars");
    const forksEl = document.getElementById("repo-forks");
    if (starsEl) starsEl.textContent = formatCount(stars);
    if (forksEl) forksEl.textContent = formatCount(forks);
  }

  function updateInstallVisibility() {
    if (!installBtn) return;
    const serialOk = "serial" in navigator;
    installBtn.style.display = serialOk ? "" : "none";
  }

  /* —— Flasher board picker —— */
  const picker = document.getElementById("board-picker");
  const installBtn = document.getElementById("install-btn");
  const dlBtn = document.getElementById("dl-btn");
  const fwName = document.getElementById("fw-name");
  const fwMeta = document.getElementById("fw-meta");
  const esptoolCmd = document.getElementById("esptool-cmd");
  const startEsptoolCmd = document.getElementById("start-esptool-cmd");

  function updateEsptoolCommands(fileName) {
    const cmd =
      `python -m esptool --chip esp32s3 --baud 921600 write-flash 0x0 ${fileName}`;
    if (esptoolCmd) esptoolCmd.textContent = cmd;
    if (startEsptoolCmd) startEsptoolCmd.textContent = cmd;
  }

  function selectBoard(board) {
    const boardKey = board.dataset.board;
    const cfg = BOARD_CONFIG[boardKey];
    if (!cfg) return;

    picker?.querySelectorAll(".board").forEach((b) => b.classList.remove("is-active"));
    board.classList.add("is-active");

    const fileName = cfg.file;
    const binPath = cfg.binPath;

    if (installBtn) {
      installBtn.setAttribute("manifest", cfg.manifestFile);
    }

    if (dlBtn) {
      dlBtn.href = binPath;
      dlBtn.setAttribute("download", fileName);
      dlBtn.removeAttribute("target");
      dlBtn.removeAttribute("rel");
    }

    if (fwName) fwName.textContent = fileName;
    if (fwMeta) fwMeta.textContent = cfg.meta;

    const fwTag = document.getElementById("fw-tag");
    if (fwTag) fwTag.textContent = cfg.version;

    const flasherVerTag = document.getElementById("flasher-ver-tag");
    if (flasherVerTag) flasherVerTag.textContent = cfg.tagText;

    updateEsptoolCommands(fileName);
    updateInstallVisibility();
  }

  picker?.addEventListener("click", (e) => {
    const board = e.target.closest(".board");
    if (board) selectBoard(board);
  });

  const defaultBoard = picker?.querySelector(".board.is-active");
  if (defaultBoard) selectBoard(defaultBoard);

  /* —— Copy esptool command —— */
  const copyCmd = document.getElementById("copy-cmd");
  copyCmd?.addEventListener("click", async () => {
    try {
      await navigator.clipboard.writeText(esptoolCmd?.textContent || "");
      copyCmd.innerHTML = '<i class="ti ti-check"></i>';
      setTimeout(() => (copyCmd.innerHTML = '<i class="ti ti-copy"></i>'), 1600);
    } catch {
      copyCmd.innerHTML = '<i class="ti ti-x"></i>';
      setTimeout(() => (copyCmd.innerHTML = '<i class="ti ti-copy"></i>'), 1600);
    }
  });

  const startCopyCmd = document.getElementById("start-copy-cmd");
  startCopyCmd?.addEventListener("click", async () => {
    try {
      await navigator.clipboard.writeText(startEsptoolCmd?.textContent || "");
      startCopyCmd.innerHTML = '<i class="hn hn-check"></i>';
      setTimeout(() => (startCopyCmd.innerHTML = '<i class="hn hn-copy"></i>'), 1600);
    } catch {
      startCopyCmd.innerHTML = '<i class="hn hn-times-circle"></i>';
      setTimeout(() => (startCopyCmd.innerHTML = '<i class="hn hn-copy"></i>'), 1600);
    }
  });

  /* —— Get started tabs —— */
  const startTabs = document.getElementById("start-tabs");

  function activateStartTab(id) {
    if (!startTabs || !id) return;
    const tab = startTabs.querySelector(`.start-tab[data-tab="${id}"]`);
    if (!tab) return;
    startTabs.querySelectorAll(".start-tab").forEach((t) => {
      const active = t === tab;
      t.classList.toggle("is-active", active);
      t.setAttribute("aria-selected", active ? "true" : "false");
    });
    startTabs.querySelectorAll(".start-panel").forEach((panel) => {
      const active = panel.dataset.panel === id;
      panel.classList.toggle("is-active", active);
      panel.hidden = !active;
    });
    tab.focus({ preventScroll: true });
  }

  startTabs?.addEventListener("click", (e) => {
    const tab = e.target.closest(".start-tab");
    if (!tab || tab.classList.contains("is-active")) return;
    activateStartTab(tab.dataset.tab);
  });

  document.querySelectorAll("[data-start-tab]").forEach((link) => {
    link.addEventListener("click", (e) => {
      e.preventDefault();
      const id = link.dataset.startTab;
      document.getElementById("start")?.scrollIntoView({ behavior: "smooth" });
      activateStartTab(id);
    });
  });

  /* —— Web Serial support warning —— */
  if (!("serial" in navigator)) {
    document.getElementById("browser-warn")?.classList.add("is-visible");
  }
  updateInstallVisibility();

  /* —— GitHub repo sync —— */
  async function syncRepo() {
    try {
      const [repoRes, releaseRes] = await Promise.all([
        fetch(REPO_API),
        fetch(`${REPO_API}/releases/latest`),
      ]);

      if (repoRes.ok) {
        const repo = await repoRes.json();
        applyRepoStats(repo.stargazers_count, repo.forks_count);
      }

      if (releaseRes.ok) {
        const release = await releaseRes.json();
        repoVersion = release.tag_name.replace(/^v/i, "");
        releaseAssets = {};
        for (const asset of release.assets || []) {
          if (asset.name.endsWith(".bin")) releaseAssets[asset.name] = asset.browser_download_url;
        }
        applyVersionLabels();
        const releasesLink = document.getElementById("releases-link");
        if (releasesLink && release.html_url) releasesLink.href = release.html_url;
      }
    } catch {
      /* keep static fallbacks */
    }

    const activeBoard = picker?.querySelector(".board.is-active") || picker?.querySelector(".board");
    if (activeBoard) selectBoard(activeBoard);
  }

  syncRepo();

  /* —— Video playlist grid —— */
  const PLAYLIST_VIDEOS = [
    { id: "2YumcTh3O7s", title: "ESP32-R3X Full Guide | The Open Source Flipper Zero Alternative" },
    { id: "wuQRoT1DsFM", title: "ESP32-R3X | Passive Wi-Fi Monitoring" },
    { id: "BlE0nNkenzE", title: "I Built Something Better Than Flipper | ESP32-R3X v2" },
    { id: "B7bdqAX_2_g", title: "Your Garage Door is NOT Safe!!" },
    { id: "HJAaVReDNLY", title: "I can trick my phone using the ESP32-R3X" },
    { id: "vEVc0dKLNsk", title: "It's pure chaos, DON'T DO THIS!!" },
    { id: "Yanvy01yz60", title: "I Fixed ESP32-R3X. and It's Better Than Ever | Part 2" },
    { id: "GqUX6TgbHEU", title: "Why I Hated My ESP32-R3X. and How I'm Fixing It | Part 1" },
    { id: "jjO6Zj0ANJY", title: "Sub-GHz Replay Attack in action with ESP32-R3X" },
    { id: "fTTjPQxpWMk", title: "ESP32-R3X Deauther" },
    { id: "yBsFysaAPms", title: "ESP32-R3X" },
    { id: "VxIagLUt2NU", title: "It shouldn't be this easy!" },
    { id: "0ckOLswa9kI", title: "ESP32-R3X in Action" },
  ];

  const videoPlayer = document.getElementById("video-player");
  const videoGrid = document.getElementById("video-grid");

  function setActiveVideo(id) {
    if (videoPlayer) {
      videoPlayer.src = `https://www.youtube.com/embed/${id}?rel=0&autoplay=1`;
    }
    videoGrid?.querySelectorAll(".video-card").forEach((card) => {
      card.classList.toggle("is-active", card.dataset.id === id);
    });
  }

  if (videoGrid && PLAYLIST_VIDEOS.length) {
    videoGrid.innerHTML = PLAYLIST_VIDEOS.map(
      (v, i) => `
        <button type="button" class="video-card${i === 0 ? " is-active" : ""}" data-id="${v.id}" aria-label="${v.title}">
          <span class="video-card__thumb">
            <img src="https://i.ytimg.com/vi/${v.id}/mqdefault.jpg" alt="" loading="lazy" width="320" height="180" />
            <span class="video-card__play" aria-hidden="true">▶</span>
          </span>
          <span class="video-card__title">${v.title}</span>
        </button>`
    ).join("");

    videoGrid.addEventListener("click", (e) => {
      const card = e.target.closest(".video-card");
      if (card?.dataset.id) setActiveVideo(card.dataset.id);
    });

    syncVideoLayout();
    requestAnimationFrame(syncVideoLayout);
    window.addEventListener("resize", syncVideoLayout);
    window.addEventListener("load", syncVideoLayout);
    if ("ResizeObserver" in window) {
      const frame = document.querySelector(".video__frame");
      if (frame) new ResizeObserver(syncVideoLayout).observe(frame);
    }
  }

  function syncVideoLayout() {
    const frame = document.querySelector(".video__frame");
    const grid = document.getElementById("video-grid");
    const side = document.querySelector(".video__side");
    if (!frame || !grid || !side || window.innerWidth <= 900) {
      grid.style.removeProperty("height");
      side?.style.removeProperty("height");
      return;
    }
    const h = frame.offsetHeight;
    side.style.height = `${h}px`;
    grid.style.height = `${h}px`;
  }
})();
