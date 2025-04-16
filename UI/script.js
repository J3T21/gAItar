// Dummy song data
let songs = [
    { title: 'Song 1', genre: 'rock', duration: 180 },
    { title: 'Song 2', genre: 'pop', duration: 210 },
    { title: 'Song 3', genre: 'jazz', duration: 240 },
    { title: 'Song 4', genre: 'classical', duration: 300 }
];

// Song library and queue
let queue = [];
let currentSongIndex = 0;
let isPlaying = false;

// DOM Elements
const playPauseButton = document.getElementById('playPause');
const elapsedTimeDisplay = document.getElementById('elapsed');
const totalTimeDisplay = document.getElementById('total');
const songList = document.getElementById('songList');
const genreSelect = document.getElementById('genre');
const queueList = document.getElementById('queueList');

// Initialize UI with songs
function updateSongLibrary() {
    songList.innerHTML = '';
    songs.forEach((song, index) => {
        let li = document.createElement('li');
        li.textContent = `${song.title} (${song.genre})`;
        li.addEventListener('click', () => addSongToQueue(index));
        songList.appendChild(li);
    });
}

// Add song to queue
function addSongToQueue(index) {
    queue.push(songs[index]);
    updateQueue();
}

// Update queue UI
function updateQueue() {
    queueList.innerHTML = '';
    queue.forEach(song => {
        let li = document.createElement('li');
        li.textContent = song.title;
        queueList.appendChild(li);
    });
}

// Play/Pause toggle
playPauseButton.addEventListener('click', () => {
    isPlaying = !isPlaying;
    playPauseButton.textContent = isPlaying ? 'Pause' : 'Play';
    if (isPlaying) {
        // Simulate playing music
        startPlaying();
    } else {
        stopPlaying();
    }
});

// Simulate playing and updating time
function startPlaying() {
    let song = queue[currentSongIndex];
    totalTimeDisplay.textContent = formatTime(song.duration);
    let elapsedTime = 0;

    let playInterval = setInterval(() => {
        if (elapsedTime < song.duration && isPlaying) {
            elapsedTime++;
            elapsedTimeDisplay.textContent = formatTime(elapsedTime);
        } else {
            clearInterval(playInterval);
            isPlaying = false;
            playPauseButton.textContent = 'Play';
            currentSongIndex++;
            if (currentSongIndex < queue.length) {
                // Play the next song
                startPlaying();
            }
        }
    }, 1000);
}

function stopPlaying() {
    // Stop playing the song
    elapsedTimeDisplay.textContent = '00:00';
}

function formatTime(seconds) {
    const minutes = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${String(minutes).padStart(2, '0')}:${String(secs).padStart(2, '0')}`;
}

// Initialize library
updateSongLibrary();
