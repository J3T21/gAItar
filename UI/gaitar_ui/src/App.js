import './App.css';
import React, { useState, useEffect, useRef } from 'react';
import Playlist from './components/Playlist';
import Controls from './components/Controls';
import PlaybackInfo from './components/PlaybackInfo';
import Upload from './components/Upload';
import MidiGenerator from './components/GenMIDI';
import { esp32 } from './api'; // Import the API instance

const App = () => {
  const [songs, setSongs] = useState([]);
  const [query, setQuery] = useState('');
  const [suggestions, setSuggestions] = useState([]);
  const [selectedArtist, setSelectedArtist] = useState('');
  const [selectedGenre, setSelectedGenre] = useState('');
  const [dropdownActive, setDropdownActive] = useState(false);
  const [currentPlaylist, setCurrentPlaylist] = useState([]);
  const [currentTrack, setCurrentTrack] = useState(null);
  const [isPlaying, setIsPlaying] = useState(false); // New state to track play/pause
  const [playlistError, setPlaylistError] = useState('');

  const uniqueArtists = [...new Set(songs.map(song => song.artist))].sort();
  const uniqueGenres = [...new Set(songs.map(song => song.genre))].sort();
  const fetchSongsCalled = useRef(false); // Ref to track if fetchSongs has been called

  useEffect(() => {
    const fetchSongs = async () => {
      if (fetchSongsCalled.current) return; // Prevent multiple calls
      fetchSongsCalled.current = true; // Mark as called

      try {
        const response = await esp32.get('/existing-songs'); // Adjust the endpoint if necessary
        console.log('Fetched songs from ESP32:', response.data);

        // Split the response string into an array of paths
        const paths = response.data.split('\n').filter((path) => path.trim() !== '');

        if (Array.isArray(paths)) {
          const parsedSongs = paths.map((path) => {
            // Split the path into parts
            const parts = path.split('/');
            const genreRaw = parts[1] || 'Unknown'; // Extract genre
            const artistRaw = parts[2] || 'Unknown'; // Extract artist
            const titleWithExtension = parts[3] || 'Unknown'; // Extract title with extension
            const titleRaw = titleWithExtension.replace('.json', ''); // Remove the .json extension

            // Enhanced unsanitize function to handle Windows line endings
            const unsanitize = (str) => {
              return str.replace(/_/g, ' ')           // Replace underscores with spaces
                       .replace(/[\r\n\t\f\v]/g, ' ') // Replace control characters with spaces
                       .replace(/\s+/g, ' ')           // Collapse multiple spaces
                       .trim();                        // Remove leading/trailing whitespace
            };

            const genre = unsanitize(genreRaw);
            const artist = unsanitize(artistRaw);
            const title = unsanitize(titleRaw);

            return { title, artist, genre }; // Return the parsed song object with spaces
          });

          setSongs(parsedSongs); // Update the songs state with the parsed data
        } else {
          console.error('Unexpected response format:', response.data);
        }
      } catch (error) {
        console.error('Error fetching songs from ESP32:', error);
      }
    };

    fetchSongs();
  }, []); // Empty dependency array ensures this runs only once

  useEffect(() => {
    // Filter songs based on the query, selected artist, and selected genre
    const matchingSuggestions = songs.filter((song) => {
      const matchesQuery = query ? song.title.toLowerCase().includes(query.toLowerCase()) : true;
      const matchesArtist = selectedArtist ? song.artist === selectedArtist : true;
      const matchesGenre = selectedGenre ? song.genre === selectedGenre : true;
      return matchesQuery && matchesArtist && matchesGenre;
    });

    setSuggestions(matchingSuggestions);
  }, [query, selectedArtist, selectedGenre, songs]); // Dependencies ensure this runs whenever these states change

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
    setQuery(suggestion.title); // Set the search bar value
    setCurrentTrack(suggestion); // Set the current track to the selected suggestion
    setIsPlaying(false); // Default to paused when a new track is selected
    setDropdownActive(false); // Hide the dropdown
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
        if (!prevPlaylist.some((track) => track.title === matchingSong.title)) {
          return [...prevPlaylist, matchingSong]; // Add full song object
        }
        return prevPlaylist;
      });
      setQuery(''); // Clear the search bar
      setPlaylistError(''); // Clear any previous error
    } else {
      setPlaylistError('Song not recognized. Please select a valid song.');
    }
  };

  const handleUpload = (title, genre) => {
    console.log(`Uploaded track: ${title} (${genre})`);
    // Add the uploaded track to the playlist or update the UI as needed
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
            {playlistError && (
              <div style={{ color: 'red', marginBottom: '10px' }}>
                {playlistError}
              </div>
            )}
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
            <h3>
              {currentTrack
                ? isPlaying
                  ? `Now Playing: ${currentTrack.title}`
                  : `Paused: ${currentTrack.title}`
                : 'No Track Playing'}
            </h3>
          </div>
          <Controls
            currentTrack={currentTrack}
            setCurrentTrack={setCurrentTrack}
            currentPlaylist={currentPlaylist}
            setCurrentPlaylist={setCurrentPlaylist} // Pass setCurrentPlaylist here
            setIsPlaying={setIsPlaying}
          />
        <div className="generator">
        <MidiGenerator />
        </div>
        </div>


        <div className="right-column">
          <div className="playlist">
            <Playlist 
              currentPlaylist={currentPlaylist} 
              setCurrentPlaylist={setCurrentPlaylist} // Add this prop
              selectPlaylist={(track) => {
                setCurrentTrack(track);
                setIsPlaying(false);
              }}
            />
          </div>
        </div>

        <div className="upload-column">
          <h3 className="upload-title">Upload</h3>
          <Upload
            genres={uniqueGenres}
            artists={uniqueArtists}
            setTrackMetadata={setTrackMetadata}
            onUpload={handleUpload}
            setSongs={setSongs} // Pass setSongs to allow updates to the artist and genre lists
          />
        </div>
      </div>
    </div>
  );
};

export default App;
