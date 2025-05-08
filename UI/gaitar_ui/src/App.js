import './App.css';
import React, { useState, useEffect } from 'react';
import Playlist from './components/Playlist';
import Controls from './components/Controls';
import PlaybackInfo from './components/PlaybackInfo';
import Upload from './components/Upload';
import { FaPlay, FaPause, FaStepForward, FaStepBackward, FaRandom } from 'react-icons/fa';

const App = () => {
  const [songs, setSongs] = useState([
    { title: 'Blinding Lights', artist: 'The Weeknd', genre: 'Pop' },
    { title: 'Shape of You', artist: 'Ed Sheeran', genre: 'Pop' },
    { title: 'Levitating', artist: 'Dua Lipa', genre: 'Pop' },
    { title: 'Bohemian Rhapsody', artist: 'Queen', genre: 'Rock' },
    { title: 'Stairway to Heaven', artist: 'Led Zeppelin', genre: 'Rock' },
    { title: 'Hotel California', artist: 'Eagles', genre: 'Rock' },
  ]);

  const [query, setQuery] = useState('');
  const [suggestions, setSuggestions] = useState([]);
  const [selectedArtist, setSelectedArtist] = useState('');
  const [selectedGenre, setSelectedGenre] = useState('');
  const [dropdownActive, setDropdownActive] = useState(false);
  const [currentPlaylist, setCurrentPlaylist] = useState([]);
  const [currentTrack, setCurrentTrack] = useState(null);
  const [currentTime, setCurrentTime] = useState(0);
  const [totalDuration, setTotalDuration] = useState(240); // Example total duration in seconds

  const uniqueArtists = [...new Set(songs.map(song => song.artist))].sort();
  const uniqueGenres = [...new Set(songs.map(song => song.genre))].sort();

  useEffect(() => {
    // Filter songs based on the query, selected artist, and selected genre
    const matchingSuggestions = songs.filter((song) => {
      const matchesQuery = query ? song.title.toLowerCase().includes(query.toLowerCase()) : true;
      const matchesArtist = selectedArtist ? song.artist === selectedArtist : true;
      const matchesGenre = selectedGenre ? song.genre === selectedGenre : true;
      return matchesQuery && matchesArtist && matchesGenre;
    });

    setSuggestions(matchingSuggestions);
  }, [query, selectedArtist, selectedGenre, songs]);

  const handleInputChange = (e) => {
    const value = e.target.value;
    setQuery(value);

    // Update suggestions dynamically as the user types
    const filteredSuggestions = songs.filter((song) => {
      const matchesQuery = value ? song.title.toLowerCase().includes(value.toLowerCase()) : true;
      const matchesArtist = selectedArtist ? song.artist === selectedArtist : true;
      const matchesGenre = selectedGenre ? song.genre === selectedGenre : true;
      return matchesQuery && matchesArtist && matchesGenre;
    });

    setSuggestions(filteredSuggestions);
  };

  const handleFilterChange = (filterType, value) => {
    if (filterType === 'artist') {
      setSelectedArtist(value);
    } else if (filterType === 'genre') {
      setSelectedGenre(value);
    }
    setDropdownActive(true);

    const filteredSuggestions = songs.filter((song) => {
      const matchesQuery = song.title.toLowerCase().includes(query.toLowerCase());
      const matchesArtist = filterType === 'artist' ? value === song.artist : selectedArtist ? song.artist === selectedArtist : true;
      const matchesGenre = filterType === 'genre' ? value === song.genre : selectedGenre ? song.genre === selectedGenre : true;
      return matchesQuery && matchesArtist && matchesGenre;
    });

    setSuggestions(filteredSuggestions);
  };

  const handleSuggestionClick = (suggestion) => {
    setQuery(suggestion.title);
    setDropdownActive(false);
  };

  const handleAddToPlaylist = () => {
    if (!query || query.trim() === '') return;

    const matchingSong = songs.find(
      (song) =>
        song.title.toLowerCase() === query.toLowerCase() &&
        (!selectedArtist || song.artist === selectedArtist) &&
        (!selectedGenre || song.genre === selectedGenre)
    );

    if (matchingSong) {
      setCurrentPlaylist((prevPlaylist) => {
        if (!prevPlaylist.includes(matchingSong.title)) {
          return [...prevPlaylist, matchingSong.title];
        }
        return prevPlaylist;
      });
      setQuery(''); // Clear the search bar
    }
  };

  const handleUpload = (title, genre) => {
    console.log(`Uploaded track: ${title} (${genre})`);
    // Add the uploaded track to the playlist or update the UI as needed
  };

  const addArtist = (newArtist) => {
    console.log(`New artist added: ${newArtist}`);
    // Add the new artist to the list of artists
  };

  const setTrackMetadata = (metadata) => {
    console.log('Track metadata updated:', metadata);
    // Handle track metadata updates
  };

  const handleBlur = (e) => {
    // Check if the blur event is related to the dropdown or input
    if (!e.relatedTarget || !e.relatedTarget.classList.contains('suggestion-item')) {
      setTimeout(() => setDropdownActive(false), 200); // Delay hiding the dropdown
    }
  };

  const handlePlay = () => {
    console.log('Play button clicked');
  };

  const handlePause = () => {
    console.log('Pause button clicked');
  };

  const handleSkip = () => {
    console.log('Skip button clicked');
  };

  const handleShuffle = () => {
    console.log('Shuffle button clicked');
  };

  const formatTime = (time) => {
    const minutes = Math.floor(time / 60);
    const seconds = time % 60;
    return `${minutes}:${seconds < 10 ? '0' : ''}${seconds}`;
  };

  return (
    <div className="app-container">
      <header className="app-header">
        <h1>gAItar: the robotic guitarist</h1>
      </header>
      <div className="columns-container">
        <div className="left-column">
          <div className="search-container">
            <div className="search-bar">
              <input
                type="text"
                placeholder="Search for a song..."
                value={query}
                onChange={handleInputChange}
                onFocus={() => setDropdownActive(true)} // Show dropdown on focus
                onBlur={handleBlur} // Hide dropdown on blur
                className="search-input"
              />
              {dropdownActive && suggestions.length > 0 && (
                <ul className="suggestions-list active">
                  {suggestions.map((song, index) => (
                    <li
                      key={index}
                      className="suggestion-item"
                      tabIndex="0" // Make the suggestion focusable
                      onMouseDown={() => handleSuggestionClick(song)} // Prevent blur on click
                    >
                      {song.title}
                    </li>
                  ))}
                </ul>
              )}
            </div>
            <button
              onClick={handleAddToPlaylist}
              className="add-to-playlist-button"
            >
              Add to Playlist
            </button>
            <select
              value={selectedArtist}
              onChange={(e) => handleFilterChange('artist', e.target.value)}
              className="artist-filter"
            >
              <option value="">All Artists</option>
              {uniqueArtists.map((artist, index) => (
                <option key={index} value={artist}>
                  {artist}
                </option>
              ))}
            </select>
            <select
              value={selectedGenre}
              onChange={(e) => handleFilterChange('genre', e.target.value)}
              className="genre-filter"
            >
              <option value="">All Genres</option>
              {uniqueGenres.map((genre, index) => (
                <option key={index} value={genre}>
                  {genre}
                </option>
              ))}
            </select>
          </div>
        </div>

        <div className="middle-column">
          <div className="current-track">
            <h3>{currentTrack ? `Now Playing: ${currentTrack.title}` : 'No Track Playing'}</h3>
          </div>
          <div className="controls">
            <div className="player-controls">
              <button onClick={handleShuffle} className="control-button">
                <FaRandom />
              </button>
              <button onClick={handleSkip} className="control-button">
                <FaStepBackward />
              </button>
              <button onClick={handlePlay} className="control-button play-button">
                <FaPlay />
              </button>
              <button onClick={handlePause} className="control-button">
                <FaPause />
              </button>
              <button onClick={handleSkip} className="control-button">
                <FaStepForward />
              </button>
            </div>
            <div className="scrubber">
              <span className="time">{formatTime(currentTime)}</span>
              <input
                type="range"
                min="0"
                max={totalDuration}
                value={currentTime}
                onChange={(e) => setCurrentTime(Number(e.target.value))}
                className="scrubber-bar"
              />
              <span className="time">{formatTime(totalDuration)}</span>
            </div>
          </div>

          <PlaybackInfo currentTrack={currentTrack} />
        </div>

        <div className="right-column">
          <div className="playlist">
            <Playlist currentPlaylist={currentPlaylist} />
          </div>
        </div>

        <div className="upload-column">
          <h3 className="upload-title">Upload</h3>
          <Upload
            genres={uniqueGenres}
            artists={uniqueArtists}
            setTrackMetadata={setTrackMetadata}
            onUpload={handleUpload}
            addArtist={addArtist}
          />
        </div>
      </div>
    </div>
  );
};

export default App;
