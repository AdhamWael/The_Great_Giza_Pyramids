document.addEventListener('DOMContentLoaded', () => {
    const navbar = document.querySelector('.navbar');
    const navProgress = document.querySelector('.nav-progress');
    const navLinks = document.querySelectorAll('.nav-links a');
    const heroTitle = document.getElementById('hero-title');
    const heroSubtitle = document.getElementById('hero-subtitle');
    const pyramidImage = document.getElementById('pyramid-image');
    const cursorGlow = document.getElementById('cursor-glow');

    // Initialize Studio Freight Lenis
    const lenis = new Lenis({
        duration: 2.0,
        easing: (t) => Math.min(1, 1.001 - Math.pow(2, -10 * t)),
        direction: 'vertical',
        gestureDirection: 'vertical',
        smooth: true,
        mouseMultiplier: 0.8,
        wheelMultiplier: 1.2,
        normalizeWheel: true,
        smoothTouch: false,
        touchMultiplier: 2,
    });

    function raf(time) {
        lenis.raf(time);
        requestAnimationFrame(raf);
    }
    requestAnimationFrame(raf);
    // Static Floating Navbar Progress
    window.addEventListener('scroll', () => {
        const scrollY = window.scrollY;
        const progressPercent = (scrollY / (document.documentElement.scrollHeight - window.innerHeight)) * 100;
        if (navProgress) navProgress.style.width = `${progressPercent}%`;
    });

    // Cinematic Parallax Engine
    let currentScroll = 0;
    const ease = 0.1;

    function updateParallax() {
        currentScroll += (window.pageYOffset - currentScroll) * ease;
        const scrolled = currentScroll;

        if (heroTitle) heroTitle.style.transform = `translateY(${scrolled * 0.3}px)`;
        if (pyramidImage) pyramidImage.style.transform = `translateY(${scrolled * 0.1}px)`;

        document.querySelectorAll('.visual-accent').forEach((accent) => {
            const rect = accent.getBoundingClientRect();
            if (rect.top < window.innerHeight && rect.bottom > 0) {
                const move = (window.innerHeight - rect.top) * 0.05;
                accent.style.backgroundPosition = `center ${move}px`;
            }
        });

        if (heroSubtitle) heroSubtitle.style.opacity = Math.max(1 - (scrolled / 400), 0);
        if (heroTitle) heroTitle.style.opacity = Math.max(1 - (scrolled / 800), 0);

        requestAnimationFrame(updateParallax);
    }
    updateParallax();

    // Mouse Interaction
    document.addEventListener('mousemove', (e) => {
        const x = e.clientX;
        const y = e.clientY;
        const xAxis = (window.innerWidth / 2 - x) / 40;
        const yAxis = (window.innerHeight / 2 - y) / 40;

        if (cursorGlow) {
            cursorGlow.style.left = `${x}px`;
            cursorGlow.style.top = `${y}px`;
        }

        if (pyramidImage) {
            pyramidImage.style.transform = `translateX(${xAxis}px) translateY(${yAxis * 0.5 + (currentScroll * 0.1)}px)`;
        }
    });

    // Observers
    const observerOptions = { threshold: 0.1 };
    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) entry.target.classList.add('reveal');
        });
    }, observerOptions);

    document.querySelectorAll('.stack-item, .details, .container').forEach(el => observer.observe(el));

    const sectionObserver = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                const id = entry.target.getAttribute('id');
                navLinks.forEach(link => {
                    link.classList.toggle('active', link.getAttribute('href') === `#${id}`);
                });
            }
        });
    }, { threshold: 0.5 });

    document.querySelectorAll('section').forEach(section => sectionObserver.observe(section));

    // Scroll for navbar links
    document.querySelectorAll('.nav-links a').forEach(link => {
        link.addEventListener('click', (e) => {
            const href = link.getAttribute('href');
            if (href.startsWith('#')) {
                e.preventDefault();
                const target = document.querySelector(href);
                if (target && lenis) {
                    lenis.scrollTo(target, {
                        offset: -120, // Enough space for sticky nav
                        duration: 1.8,
                        easing: (t) => Math.min(1, 1.001 - Math.pow(2, -10 * t))
                    });
                }
            }
        });
    });

    // Side Navigation (Optional Side Bar items)
    document.querySelectorAll('.nav-item').forEach(item => {
        item.addEventListener('click', () => {
            const targetId = item.getAttribute('data-target');
            document.getElementById(targetId)?.scrollIntoView({ behavior: 'smooth' });
        });
    });



    // ─── 3D Book Flip Logic (Not Slider) ──────────────────────────────────────────
    const spreadCards = document.querySelectorAll('.spread-card');
    const TOTAL_SPREADS = spreadCards.length;
    const FLIP_DURATION = 1200; // Matches CSS transition

    const btnPrev = document.getElementById('book-prev');
    const btnNext = document.getElementById('book-next');
    const pageLabel = document.getElementById('book-page-label');
    const dotsContainer = document.getElementById('book-dots');

    let currentIndex = 0;
    let isFlipping = false;

    // Initialize spread indices for CSS z-stacking
    spreadCards.forEach((card, i) => {
        card.style.setProperty('--index', i);
    });

    const dots = [];
    if (dotsContainer) {
        dotsContainer.innerHTML = '';
        for (let i = 0; i < TOTAL_SPREADS; i++) {
            const dot = document.createElement('button');
            dot.className = 'book-dot';
            dot.setAttribute('aria-label', `Go to page ${i + 1}`);
            dot.addEventListener('click', (e) => {
                e.preventDefault();
                e.stopPropagation();
                goToSpread(i);
            });
            dotsContainer.appendChild(dot);
            dots.push(dot);
        }
    }

    function goToSpread(target) {
        if (isFlipping || target === currentIndex || target < 0 || target >= TOTAL_SPREADS) return;

        isFlipping = true;
        currentIndex = target;

        spreadCards.forEach((card, i) => {
            // Flip logic: all cards before target are 'flipped' (-180deg)
            if (i < target) {
                card.classList.add('flipped');
            } else {
                card.classList.remove('flipped');
            }
            // Active state
            card.classList.toggle('active', i === currentIndex);
        });

        // UI Updates
        if (btnPrev) btnPrev.setAttribute('data-disabled', currentIndex <= 0 ? 'true' : 'false');
        if (btnNext) btnNext.setAttribute('data-disabled', currentIndex >= TOTAL_SPREADS - 1 ? 'true' : 'false');
        if (pageLabel) pageLabel.textContent = `Page ${currentIndex + 1} of ${TOTAL_SPREADS}`;
        dots.forEach((d, i) => d.classList.toggle('active', i === currentIndex));

        setTimeout(() => { isFlipping = false; }, FLIP_DURATION);
    }

    if (btnPrev) {
        btnPrev.onclick = (e) => {
            e.preventDefault();
            e.stopPropagation();
            goToSpread(currentIndex - 1);
        };
    }
    if (btnNext) {
        btnNext.onclick = (e) => {
            e.preventDefault();
            e.stopPropagation();
            goToSpread(currentIndex + 1);
        };
    }

    // Spine/Book Area Click Navigation
    const bookWrapper = document.querySelector('.book-wrapper');
    const pyramidModal = document.getElementById('pyramid-modal');

    if (bookWrapper) {
        bookWrapper.addEventListener('click', (e) => {
            if (pyramidModal && pyramidModal.classList.contains('active')) return;
            if (e.target.closest('.pyramid-trigger') || e.target.closest('.book-arrow') || e.target.closest('.book-dot')) return;

            const rect = bookWrapper.getBoundingClientRect();
            const relX = e.clientX - rect.left;

            if (relX > rect.width / 2) {
                goToSpread(currentIndex + 1);
            } else {
                goToSpread(currentIndex - 1);
            }
        });
    }

    // Touch Swiping (Converted to Flip)
    let touchStartX = 0;
    let touchEndX = 0;

    if (bookWrapper) {
        bookWrapper.addEventListener('touchstart', (e) => { touchStartX = e.changedTouches[0].screenX; }, { passive: true });
        bookWrapper.addEventListener('touchend', (e) => {
            touchEndX = e.changedTouches[0].screenX;
            const deltaX = touchEndX - touchStartX;
            if (deltaX < -50) goToSpread(currentIndex + 1);
            if (deltaX > 50) goToSpread(currentIndex - 1);
        }, { passive: true });
    }

    // Keyboard bindings
    document.addEventListener('keydown', (e) => {
        if (pyramidModal && pyramidModal.classList.contains('active')) return;
        const bookSection = document.querySelector('.eng-readout');
        if (!bookSection) return;
        const sRect = bookSection.getBoundingClientRect();
        if (sRect.bottom < 0 || sRect.top > window.innerHeight) return;

        if (e.key === 'ArrowRight') goToSpread(currentIndex + 1);
        if (e.key === 'ArrowLeft') goToSpread(currentIndex - 1);
    });

    // ─── 3D Book Tilt (Global Container) ─────────────────────────────────────
    const readoutSection = document.querySelector('.eng-readout');
    const bookContainer = document.querySelector('.book-container');

    if (readoutSection && bookContainer) {
        let tiltRaf = null;

        readoutSection.addEventListener('mousemove', (e) => {
            if (tiltRaf) cancelAnimationFrame(tiltRaf);
            tiltRaf = requestAnimationFrame(() => {
                const rect = bookContainer.getBoundingClientRect();
                const xNorm = (e.clientX - rect.left) / rect.width;
                const yNorm = (e.clientY - rect.top) / rect.height;

                const rx = (yNorm - 0.5) * -4;
                const ry = (xNorm - 0.5) * 4;
                const px = (xNorm - 0.5) * 60;
                const py = (yNorm - 0.5) * 60;

                bookContainer.style.transform = `rotateX(${rx}deg) rotateY(${ry}deg)`;
                bookContainer.style.setProperty('--px', `${px}px`);
                bookContainer.style.setProperty('--py', `${py}px`);
            });
        });

        readoutSection.addEventListener('mouseleave', () => {
            if (tiltRaf) cancelAnimationFrame(tiltRaf);
            bookContainer.style.transition = 'transform 0.6s cubic-bezier(0.175, 0.885, 0.32, 1.275)';
            bookContainer.style.transform = 'rotateX(0deg) rotateY(0deg)';
            bookContainer.style.setProperty('--px', '0px');
            bookContainer.style.setProperty('--py', '0px');
        });
    }

    // Engineering Doctrine Shatter Animation
    window.addEventListener('scroll', () => {
        const shatterWrapper = document.querySelector('.shatter-wrapper');
        const shatterItems = document.querySelectorAll('.shatter-item');
        if (!shatterWrapper || shatterItems.length === 0) return;

        const rect = shatterWrapper.getBoundingClientRect();
        const winHeight = window.innerHeight;
        const totalHeight = shatterWrapper.offsetHeight;

        // 1. Appearance logic: Fade in as the wrapper enters the screen
        let appearanceProgress = (winHeight - rect.top) / 500;
        appearanceProgress = Math.max(0, Math.min(1, appearanceProgress));

        // 2. Shatter trigger: Start animation only after established on screen
        const totalScrollArea = totalHeight - winHeight;
        let progress = -rect.top / totalScrollArea;
        progress = Math.max(0, Math.min(1, progress));

        // Apply a smooth cubic easing for a more premium feel
        const easedProgress = 1 - Math.pow(1 - progress, 2.5);

        shatterItems.forEach((item, index) => {
            const spread = 1 - easedProgress; // 1 = fully stacked, 0 = in grid

            // Staggering: each card reacts slightly differently
            const individualSpread = Math.max(0, Math.min(1, (1 - progress * 1.2) + (index * 0.1)));

            // Standardized focal points (user preferred 55/55)
            let tx = 0, ty = 0;
            if (index === 0) { tx = 55; ty = 55; }
            if (index === 1) { tx = -55; ty = 55; }
            if (index === 2) { tx = 55; ty = -55; }
            if (index === 3) { tx = -55; ty = -55; }

            const currentTx = tx * individualSpread;
            const currentTy = ty * individualSpread;
            const currentScale = 1 - (individualSpread * 0.05 * index);

            // Add subtle 3D rotation based on spread
            const rotX = (1 - individualSpread) * (index % 2 === 0 ? 2 : -2);
            const rotY = (1 - individualSpread) * (index < 2 ? -2 : 2);

            item.style.opacity = appearanceProgress;
            item.style.transform = `
                translate(${currentTx}%, ${currentTy}%) 
                scale(${currentScale})
                rotateX(${rotX}deg)
                rotateY(${rotY}deg)
            `;
            item.style.zIndex = index;
        });

        // Navigation Highlight Sync
        let cur = "";
        const anchors = ['seked', 'geometric-perfection', 'astronomy', 'standardization', 'scale-engineering'];
        anchors.forEach(id => {
            const el = document.getElementById(id);
            if (el && window.scrollY >= el.offsetTop - 300) cur = id;
        });
        document.querySelectorAll('.nav-item').forEach(itm => {
            itm.classList.toggle('active', itm.getAttribute('data-target') === cur);
        });
    });

    /* ─── Pyramid Modal Logic ───────────────────────────────────────────────── */
    const modalTitle = document.getElementById('modal-title');
    const modalHeight = document.getElementById('modal-height');
    const modalBuilt = document.getElementById('modal-built');
    const modalFact = document.getElementById('modal-fact');
    const modalClose = document.getElementById('modal-close');
    const modalOverlay = document.getElementById('modal-overlay');

    const pyramidData = {
        khufu: {
            title: "Khufu — The Great Pyramid",
            height: "146.6 Meters",
            built: "c. 2560 BC",
            fact: "The largest and oldest of the three. Originally towering at 146.6 meters, it consists of an estimated 2.3 million stone blocks.",
            illustration: "khufu.svg",
            video: `<iframe width="100%" height="100%" src="https://www.youtube.com/embed/dWq0iC4YqbE?si=FlrPhRN7AKslhbL3" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>`,
            table: `<tr><td>Base Side Length</td><td>230.3</td><td>23.0</td></tr><tr><td>Vertical Height</td><td>146.6</td><td>14.7</td></tr><tr><td>Slant Height (Face)</td><td>186.4</td><td>18.6</td></tr><tr><td>Lateral Edge (Corner)</td><td>219.1</td><td>21.9</td></tr><tr><td>Base Area</td><td>53,038</td><td>529.0</td></tr><tr><td>Lateral Surface Area</td><td>85,844</td><td>857.4</td></tr><tr><td>Total Surface Area</td><td>138,882</td><td>1,386.4</td></tr><tr><td>Volume</td><td>2,583,283</td><td>2,583.3</td></tr>`
        },
        khafre: {
            title: "Khafre — The Horizon",
            height: "143.5 Meters",
            built: "c. 2570 BC",
            fact: "Built by Khufu's son. Known for retaining some of its original casing stones at the peak. Although slightly shorter than the Great Pyramid, it appears taller because it sits on elevated bedrock.",
            illustration: "khafraa.svg",
            video: `<iframe width="100%" height="100%" src="https://www.youtube.com/embed/_bfHmR8F9hY?si=cfhmjkduHIgWbzu0" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>`,
            table: `<tr><td>Base Side Length</td><td>215.3</td><td>21.5</td></tr><tr><td>Vertical Height</td><td>143.5</td><td>14.4</td></tr><tr><td>Slant Height (Face)</td><td>178.8</td><td>17.9</td></tr><tr><td>Lateral Edge (Corner)</td><td>208.7</td><td>20.9</td></tr><tr><td>Base Area</td><td>46,354</td><td>462.3</td></tr><tr><td>Lateral Surface Area</td><td>76,991</td><td>769.7</td></tr><tr><td>Total Surface Area</td><td>123,345</td><td>1,232.0</td></tr><tr><td>Volume</td><td>2,217,268</td><td>2,217.3</td></tr>`
        },
        menkaure: {
            title: "Menkaure — The Divine",
            height: "65.5 Meters",
            built: "c. 2510 BC",
            fact: "The smallest of the main three, originally cased in red granite. Showcases a distinct shift in scale and material selection at the culmination of the 4th Dynasty plateau expansion.",
            illustration: "manqaraa.svg",
            video: `<iframe width="100%" height="100%" src="https://www.youtube.com/embed/IEhuw_6-4eQ?si=757wCnCYzGhRijwD" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>`,
            table: `<tr><td>Base Side Length</td><td>103.3</td><td>10.3</td></tr><tr><td>Vertical Height</td><td>65.5</td><td>6.6</td></tr><tr><td>Slant Height (Face)</td><td>83.5</td><td>8.4</td></tr><tr><td>Lateral Edge (Corner)</td><td>97.4</td><td>9.7</td></tr><tr><td>Base Area</td><td>10,671</td><td>106.7</td></tr><tr><td>Lateral Surface Area</td><td>17,251</td><td>172.5</td></tr><tr><td>Total Surface Area</td><td>27,922</td><td>279.2</td></tr><tr><td>Volume</td><td>232,977</td><td>233.0</td></tr>`
        }
    };

    function openPyramidModal(id) {
        console.log('Opening modal for:', id);
        const data = pyramidData[id];
        if (!data) {
            console.error('No data found for pyramid ID:', id);
            return;
        }

        const titleEl = document.getElementById('modal-title');
        const heightEl = document.getElementById('modal-height');
        const builtEl = document.getElementById('modal-built');
        const factEl = document.getElementById('modal-fact');
        const tableBodyEl = document.getElementById('modal-table-body');
        const illustrationEl = document.getElementById('modal-illustration');
        const modalEl = document.getElementById('pyramid-modal');

        if (titleEl) titleEl.textContent = data.title;
        if (heightEl) heightEl.textContent = data.height;
        if (builtEl) builtEl.textContent = data.built;
        if (factEl) factEl.textContent = data.fact;
        if (tableBodyEl) tableBodyEl.innerHTML = data.table;

        const videoContainer = document.getElementById('modal-video-container');
        if (videoContainer) videoContainer.innerHTML = data.video;

        if (modalEl) {
            modalEl.classList.remove('theme-khufu', 'theme-khafre', 'theme-menkaure');
            modalEl.classList.add(`theme-${id}`);
            modalEl.classList.add('active');
            modalEl.setAttribute('aria-hidden', 'false');

            if (typeof lenis !== 'undefined') lenis.stop();
            document.body.style.overflow = 'hidden';

            const content = modalEl.querySelector('.modal-content');
            if (content) content.scrollTop = 0;
            console.log('Modal activated for', id);

            // Animate Scan Progress
            const progressFill = document.getElementById('modal-scan-progress');
            const percentText = document.getElementById('scan-percent-v2');
            if (progressFill && percentText) {
                progressFill.style.width = '0%';
                percentText.textContent = '0%';

                let progress = 0;
                const interval = setInterval(() => {
                    progress += Math.floor(Math.random() * 8) + 2;
                    if (progress >= 100) {
                        progress = 100;
                        clearInterval(interval);
                    }
                    progressFill.style.width = `${progress}%`;
                    percentText.textContent = `${progress}%`;
                }, 40);
            }
        }
    }

    function closePyramidModal() {
        const modalEl = document.getElementById('pyramid-modal');
        if (modalEl) {
            modalEl.classList.remove('active');
            modalEl.setAttribute('aria-hidden', 'true');
            document.body.style.overflow = '';

            if (typeof lenis !== 'undefined') {
                lenis.start();
                const bookSection = document.getElementById('scale-engineering');
                if (bookSection) {
                    lenis.scrollTo(bookSection, {
                        offset: -50,
                        duration: 1.5,
                        easing: (t) => Math.min(1, 1.001 - Math.pow(2, -10 * t))
                    });
                }
            }
        }
    }

    if (modalClose) modalClose.addEventListener('click', closePyramidModal);
    if (modalOverlay) modalOverlay.addEventListener('click', closePyramidModal);

    const modalElement = document.getElementById('pyramid-modal');
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && modalElement && modalElement.classList.contains('active')) {
            closePyramidModal();
        }
    });

    document.querySelectorAll('.pyramid-trigger').forEach(trigger => {
        trigger.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            const parts = trigger.id.split('-');
            const id = parts[1];
            openPyramidModal(id);
        });
    });

    // ─── Drag to Scroll Logic for Modal ───
    const modalContent = document.querySelector('.modal-content');
    if (modalContent) {
        let isDown = false;
        let startY;
        let scrollTop;

        const startDragging = (e) => {
            isDown = true;
            modalContent.classList.add('grabbing');
            startY = (e.pageY || (e.touches && e.touches[0].pageY)) - modalContent.offsetTop;
            scrollTop = modalContent.scrollTop;
            modalContent.style.scrollBehavior = 'auto';
        };

        const stopDragging = () => {
            isDown = false;
            modalContent.classList.remove('grabbing');
            modalContent.style.scrollBehavior = '';
        };

        const moveDragging = (e) => {
            if (!isDown) return;
            const y = (e.pageY || (e.touches && e.touches[0].pageY)) - modalContent.offsetTop;
            const walk = (y - startY) * 1.5;
            modalContent.scrollTop = scrollTop - walk;
        };

        modalContent.addEventListener('mousedown', startDragging);
        modalContent.addEventListener('touchstart', startDragging, { passive: true });

        window.addEventListener('mouseup', stopDragging);
        window.addEventListener('touchend', stopDragging);

        modalContent.addEventListener('mousemove', moveDragging);
        modalContent.addEventListener('touchmove', (e) => {
            if (!isDown) return;
            const y = e.touches[0].pageY - modalContent.offsetTop;
            const walk = (y - startY) * 1.5;
            modalContent.scrollTop = scrollTop - walk;
            if (Math.abs(walk) > 10) e.preventDefault(); // Prevent page scroll if dragging modal
        }, { passive: false });
    }

    // ─── Ambient / Prophecy Audio Engine ───
    const audioBtn = document.getElementById('audio-toggle-btn');
    const bgAudio = document.getElementById('bg-audio');
    const prophecySection = document.getElementById('prophecy');
    const prophecyAudio = document.getElementById('prophecy-audio');
    const prophecyTranscript = document.getElementById('prophecy-transcript');

    const prophecyScript = [
        { text: "I have watched… before your time had a name.<br>Before memory… before history… before you." },
        { text: "They came… not as men… but as <b>kings of eternity.</b>" },
        { text: "<b>Khufu…</b><br>The one who reached for the heavens… and carved his will into stone." },
        { text: "<b>Khafre…</b><br>The guardian of power… whose gaze still binds the horizon." },
        { text: "<b>Menkaure…</b><br>The silent legacy… the final echo of a divine ambition." },
        { text: "They did not build… for life.<br>They built… to defeat death itself." },
        { text: "Stone… upon stone… upon stone…<br>Not placed by hands alone…<br>…but by belief… by fear… by eternity." },
        { text: "The pyramids rose… like mountains against time.<br>Each block… a heartbeat.<br>Each shadow… a secret." },
        { text: "And then… time began its work." },
        { text: "Empires… faded.<br>Names… forgotten.<br>Voices… lost to <b>الرمال.</b>" },
        { text: "Yet I remained.<br>Watching…" },
        { text: "Thousands of years have passed…<br>and still… they stand." },
        { text: "Unbroken. Unanswered. Unclaimed by time." },
        { text: "You walk beside them now…<br>thinking you understand." },
        { text: "…but you do not." },
        { text: "For they were not built…<br>to be remembered." },
        { text: "They were built…<br>to outlive eternity itself." }
    ];

    let currentProphecyIdx = -1;
    let touchAttempts = 0;
    let isProphecyMode = false;
    let hasProphecyPlayed = false;

    const hudStatus = document.getElementById('hud-status');
    const progressBar = document.getElementById('prophecy-progress');

    function typeLine(lineData, duration) {
        if (!prophecyTranscript) return;

        // Transitions: Fade out old lines immediately
        const oldLines = prophecyTranscript.querySelectorAll('.transcript-line');
        oldLines.forEach(l => {
            l.classList.add('fade-out');
            l.classList.remove('active');
            setTimeout(() => l.remove(), 1200);
        });

        // Hide init prompt
        const initPrompt = prophecyTranscript.querySelector('.init-prompt');
        if (initPrompt) initPrompt.style.display = 'none';

        const lineEl = document.createElement('h2');
        lineEl.className = 'transcript-line';
        lineEl.innerHTML = lineData.text;
        prophecyTranscript.appendChild(lineEl);

        // Force reflow for animation
        lineEl.offsetHeight;
        lineEl.classList.add('active');

        // Progress bar sync
        if (progressBar) {
            progressBar.style.transition = 'none';
            progressBar.style.width = '0%';
            lineEl.offsetHeight; // reflow
            progressBar.style.transition = `width ${duration}s linear`;
            progressBar.style.width = '100%';
        }
    }

    function playNarratorSequence(index) {
        if (index >= prophecyScript.length) {
            hasProphecyPlayed = true;
            if (audioBtn) {
                audioBtn.textContent = "CHRONICLE COMPLETE";
                audioBtn.classList.remove('audio-active');
                audioBtn.style.opacity = "0.5";
            }
            if (hudStatus) hudStatus.textContent = "DECRYPTED";
            if (progressBar) progressBar.style.width = '100%';
            return;
        }

        currentProphecyIdx = index;
        let fileName = (index + 1) + ".m4a";
        if (index === 0) fileName = "1.mp4";
        if (index === 11) fileName = "12 (1).m4a";

        if (hudStatus) hudStatus.textContent = `SYNCING SECTOR_${index + 1}...`;

        prophecyAudio.src = fileName;
        prophecyAudio.load();

        const startPlayback = () => {
            const duration = prophecyAudio.duration || 6;
            typeLine(prophecyScript[index], duration);
            prophecyAudio.play().catch(() => { });

            if (audioBtn) {
                audioBtn.textContent = "PLAYING NARRATION";
                audioBtn.classList.add('audio-active');
            }
            if (hudStatus) hudStatus.textContent = "STREAMING_DATA";
        };

        prophecyAudio.onloadedmetadata = startPlayback;
        prophecyAudio.oncanplay = () => {
            if (prophecyAudio.paused && !prophecyAudio.ended) startPlayback();
        };

        prophecyAudio.onerror = () => {
            typeLine(prophecyScript[index], 6);
            setTimeout(() => playNarratorSequence(index + 1), 7000);
        };

        prophecyAudio.onended = () => {
            setTimeout(() => {
                playNarratorSequence(index + 1);
            }, 500);
        };
    }

    if (audioBtn) {
        const pObserver = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                isProphecyMode = entry.isIntersecting;
                if (hasProphecyPlayed) return;

                if (isProphecyMode) {
                    if (currentProphecyIdx === -1) {
                        audioBtn.textContent = "PLAY CHRONICLE";
                        audioBtn.classList.add('ready-trigger');
                    } else {
                        // Keep current state text if playing
                    }
                    audioBtn.style.borderColor = "var(--primary-blue)";
                } else {
                    audioBtn.textContent = "Don't Touch Until The End";
                    audioBtn.classList.remove('ready-trigger');
                    audioBtn.style.borderColor = "var(--gold)";
                }
            });
        }, { threshold: 0.3 });

        if (prophecySection) pObserver.observe(prophecySection);

        audioBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            if (hasProphecyPlayed) return;

            if (!isProphecyMode) {
                touchAttempts++;
                const originalText = audioBtn.textContent;
                const warnings = ["I TOLD YOU DON'T TOUCH!", "STOP. IT. NOW.", "YOU ARE DISTURBING THE ARCHIVE.", "CURSE OF THE SCRIBE IMMINENT...", "ACCESS REVOKED."];
                audioBtn.textContent = warnings[Math.min(touchAttempts - 1, warnings.length - 1)];
                audioBtn.classList.add('forbidden-shake');
                setTimeout(() => {
                    audioBtn.textContent = originalText;
                    audioBtn.classList.remove('forbidden-shake');
                }, 2000);
                return;
            }

            if (currentProphecyIdx === -1) {
                if (!serialWriter) {
                    await initSerial();
                }
                sendSerialCommand('G');
                playNarratorSequence(0);
            } else {
                if (prophecyAudio.paused) {
                    prophecyAudio.play();
                    audioBtn.textContent = "PAUSING DECRYPTION";
                    audioBtn.classList.add('audio-active');
                } else {
                    prophecyAudio.pause();
                    audioBtn.textContent = "RESUME DECRYPTION";
                    audioBtn.classList.remove('audio-active');
                }
            }
        });

        window.addEventListener('scroll', () => {
            if (isProphecyMode) return;
            if (prophecyAudio && !prophecyAudio.paused && !prophecyAudio.ended) {
                prophecyAudio.pause();
                audioBtn.textContent = "SIGNAL LOST...";
                audioBtn.classList.remove('audio-active');
            }
        });
    }



    // ─── Direct Zoom Prevention Logic ───
    document.addEventListener('touchstart', (e) => {
        if (e.touches.length > 1) e.preventDefault();
    }, { passive: false });

    let lastTouchEnd = 0;
    document.addEventListener('touchend', (e) => {
        const now = (new Date()).getTime();
        if (now - lastTouchEnd <= 300) e.preventDefault();
        lastTouchEnd = now;
    }, false);

    const modalEl = document.getElementById('pyramid-modal');
    if (modalEl) {
        modalEl.addEventListener('wheel', (e) => {
            if (modalEl.classList.contains('active')) {
                // Let modal-content handle it
            }
        }, { passive: true });
    }




    // ─── Web Serial Arduino Connection ───
    let serialPort = null;
    let serialWriter = null;

    async function initSerial() {
        if (!navigator.serial) {
            console.error("Web Serial API not supported in this browser.");
            return false;
        }
        if (!serialWriter) {
            try {
                serialPort = await navigator.serial.requestPort();
                await serialPort.open({ baudRate: 9600 });
                const encoder = new TextEncoderStream();
                const writableStreamClosed = encoder.readable.pipeTo(serialPort.writable);
                serialWriter = encoder.writable.getWriter();
                return true;
            } catch (err) {
                console.error("Failed to initialize Serial:", err);
                return false;
            }
        }
        return true;
    }

    async function sendSerialCommand(cmd) {
        if (serialWriter) {
            try {
                await serialWriter.write(cmd);
                console.log(`Sent to Arduino: ${cmd}`);
            } catch (err) {
                console.error("Failed to write to Serial:", err);
            }
        }
    }

    // ─── Connect Arduino Button ───
    const connectBtn = document.getElementById('arduino-connect');
    if (connectBtn) {
        connectBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            const label = document.getElementById('arduino-connect-label');
            if (label) label.textContent = 'Connecting…';
            const success = await initSerial();
            if (success) {
                connectBtn.setAttribute('data-connected', 'true');
                connectBtn.style.borderColor = 'rgba(80, 230, 140, 0.6)';
                connectBtn.style.color = 'rgba(120, 240, 160, 0.9)';
                connectBtn.style.pointerEvents = 'none';
                if (label) label.textContent = 'Arduino Connected';
            } else {
                connectBtn.style.borderColor = 'rgba(255, 100, 80, 0.6)';
                connectBtn.style.color = 'rgba(255, 140, 120, 0.9)';
                if (label) label.textContent = 'Connection Failed';
                setTimeout(() => {
                    connectBtn.style.borderColor = 'rgba(100, 160, 230, 0.45)';
                    connectBtn.style.color = 'rgba(140, 195, 255, 0.85)';
                    if (label) label.textContent = 'Connect Arduino';
                }, 2500);
            }
        });
    }

    // ─── Keyboard Shortcut (A + F) to trigger Prophecy ───
    const KEY_HOLD = 'a';
    const KEY_FIRE = 'f';
    let holdDown = false;

    async function triggerShow() {
        if (!serialWriter) {
            await initSerial();
        }

        // Restart logic if already playing
        if (currentProphecyIdx !== -1) {
            prophecyAudio.pause();
            prophecyAudio.currentTime = 0;
            if (prophecyTranscript) prophecyTranscript.innerHTML = '';
            hasProphecyPlayed = false;
            currentProphecyIdx = -1;
        }

        sendSerialCommand('G');
        playNarratorSequence(0);

        const pSection = document.getElementById('prophecy');
        if (pSection && lenis) {
            lenis.scrollTo(pSection, {
                duration: 2,
                easing: (t) => Math.min(1, 1.001 - Math.pow(2, -10 * t))
            });
        }
    }

    document.addEventListener('keydown', e => {
        if (e.key.toLowerCase() === KEY_HOLD) holdDown = true;
        if (e.key.toLowerCase() === KEY_FIRE && holdDown) triggerShow();
    });

    document.addEventListener('keyup', e => {
        if (e.key.toLowerCase() === KEY_HOLD) holdDown = false;
    });

});


