import './App.css';
import React, { useState, useEffect, useRef, useCallback } from 'react';
import Playlist from './components/Playlist';
import Controls from './components/Controls';
import PlaybackInfo from './components/PlaybackInfo';
import Upload from './components/Upload';
import MidiGenerator from './components/GenMIDI';
import VoiceToMIDI from './components/VoiceToMIDI';
import VoiceControl from './components/VoiceControl'; // Add this import
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
  const controlsRef = useRef(null);

  const uniqueArtists = [...new Set(songs.map(song => song.artist))].sort();
  const uniqueGenres = [...new Set(songs.map(song => song.genre))].sort();
  const fetchSongsCalled = useRef(false); // Ref to track if fetchSongs has been called

  // Add a ref to always have access to current songs
  const songsRef = useRef([]);

  // Update the ref whenever songs change
  useEffect(() => {
    songsRef.current = songs;
    console.log('Songs ref updated with', songs.length, 'songs');
  }, [songs]);

  useEffect(() => {
  const fetchSongs = async (retryCount = 0) => {
    const maxRetries = 5;
    const retryDelay = 2000; // 2 seconds
    
    try {
      console.log(`Attempting to fetch songs (attempt ${retryCount + 1}/${maxRetries + 1})`);
      const response = await esp32.get('/existing-songs');
      console.log('Fetched songs from ESP32:', response.data);
      
      if (response.data === '' || !response.data) {
        console.log('No songs found on SD card, will retry...');
        if (retryCount < maxRetries) {
          setTimeout(() => fetchSongs(retryCount + 1), retryDelay);
          return;
        } else {
          console.error('No songs found after maximum retries');
          return;
        }
      }

      // Split the response string into an array of paths
      const paths = response.data.split('\n').filter((path) => path.trim() !== '');

      if (Array.isArray(paths) && paths.length > 0) {
        const parsedSongs = paths.map((path) => {
          // Split the path into parts
          const parts = path.split('/');
          const genreRaw = parts[1] || null;
          const artistRaw = parts[2] || null;
          const titleWithExtension = parts[3] || null;
          
          if (!genreRaw || !artistRaw || !titleWithExtension) {
            return null;
          }
          
          const titleRaw = titleWithExtension.replace('.json', '')

          const unsanitize = (str) => {
            return str.replace(/_/g, ' ')
                     .replace(/[\r\n\t\f\v]/g, ' ')
                     .replace(/\s+/g, ' ')
                     .trim();
          };

          const genre = unsanitize(genreRaw);
          const artist = unsanitize(artistRaw);
          const title = unsanitize(titleRaw);

          if (!genre || !artist || !title) {
            return null;
          }

          return { title, artist, genre };
        }).filter(song => song !== null);

        if (parsedSongs.length > 0) {
          setSongs(parsedSongs);
          console.log(`Successfully loaded ${parsedSongs.length} songs`);
        } else {
          console.log('No valid songs parsed, will retry...');
          if (retryCount < maxRetries) {
            setTimeout(() => fetchSongs(retryCount + 1), retryDelay);
          }
        }
      } else {
        console.log('Invalid response format, will retry...');
        if (retryCount < maxRetries) {
          setTimeout(() => fetchSongs(retryCount + 1), retryDelay);
        }
      }
    } catch (error) {
      console.error(`Error fetching songs from ESP32 (attempt ${retryCount + 1}):`, error);
      if (retryCount < maxRetries) {
        console.log(`Retrying in ${retryDelay}ms...`);
        setTimeout(() => fetchSongs(retryCount + 1), retryDelay);
      } else {
        console.error('Failed to fetch songs after maximum retries');
      }
    }
  };

  if (!fetchSongsCalled.current) {
    fetchSongsCalled.current = true;
    fetchSongs();
  }
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

  // Function to handle MIDI upload from VoiceToMIDI
  const handleVoiceToMidiUpload = (midiFile) => {
    // Create a custom event to pass the file to Upload component
    const uploadEvent = new CustomEvent('voiceMidiUpload', { detail: midiFile });
    document.dispatchEvent(uploadEvent);
  };

  // Add voice command handler
  // ...existing code...

  // Enhanced voice command handler with component integration
  // Add this import at the top if not already there
const handleVoiceCommand = useCallback((action, data) => {
  console.log('Voice command received:', action, data);
  console.log('Songs array length in handleVoiceCommand:', songsRef.current.length); // Use ref!
  
  const currentSongs = songsRef.current; // Get current songs from ref
  
  switch (action) {
    case 'play':
      if (currentTrack) {
        console.log('Voice: Playing current track');
        if (controlsRef.current?.handlePlay) {
          controlsRef.current.handlePlay();
        }
        setIsPlaying(true);
      } else if (currentPlaylist.length > 0) {
        const firstTrack = currentPlaylist[0];
        setCurrentTrack(firstTrack);
        setIsPlaying(true);
        console.log('Voice: Playing first track in playlist:', firstTrack.title);
      } else {
        console.log('Voice: No track or playlist to play');
      }
      break;
      
    case 'pause':
      console.log('Voice: Pausing playback');
      if (controlsRef.current?.handlePause) {
        controlsRef.current.handlePause();
      }
      setIsPlaying(false);
      break;
      
    case 'skip':
    case 'next':
      console.log('Voice: Skipping to next track');
      if (controlsRef.current?.handleSkip) {
        controlsRef.current.handleSkip();
      } else {
        const currentIndex = currentPlaylist.findIndex(track => track.title === currentTrack?.title);
        if (currentIndex !== -1 && currentIndex < currentPlaylist.length - 1) {
          const nextTrack = currentPlaylist[currentIndex + 1];
          setCurrentTrack(nextTrack);
          setIsPlaying(true);
          console.log('Voice: Skipped to:', nextTrack.title);
        }
      }
      break;
      
    case 'previous':
      console.log('Voice: Going to previous track');
      if (controlsRef.current?.handlePrevious) {
        controlsRef.current.handlePrevious();
      } else {
        const currentIndex = currentPlaylist.findIndex(track => track.title === currentTrack?.title);
        if (currentIndex > 0) {
          const previousTrack = currentPlaylist[currentIndex - 1];
          setCurrentTrack(previousTrack);
          setIsPlaying(true);
          console.log('Voice: Went back to:', previousTrack.title);
        }
      }
      break;
      
    case 'shuffle':
      console.log('Voice: Shuffling playlist');
      const shuffledPlaylist = [...currentPlaylist].sort(() => Math.random() - 0.5);
      setCurrentPlaylist(shuffledPlaylist);
      if (shuffledPlaylist.length > 0) {
        setCurrentTrack(shuffledPlaylist[0]);
        setIsPlaying(true);
      }
      break;
      
    case 'search':
      if (data) {
        setQuery(data);
        setDropdownActive(true);
        console.log('Voice: Searching for:', data);
      }
      break;
      
    case 'playSpecificSong':
      if (data && typeof data === 'string') { // Add type check here
        console.log('Voice: Looking for song with name:', data);
        console.log('Available songs:', currentSongs.map(s => `"${s.title}" by ${s.artist}`));
        
        // Convert data to string just in case
        const searchTerm = String(data).toLowerCase();
        
        // Try exact title match first (case-insensitive)
        let foundSong = currentSongs.find(song => 
          song.title.toLowerCase() === searchTerm
        );
        
        // If no exact match, try partial matching (case-insensitive)
        if (!foundSong) {
          foundSong = currentSongs.find(song => 
            song.title.toLowerCase().includes(searchTerm) ||
            song.artist.toLowerCase().includes(searchTerm)
          );
        }
        
        // If still no match, try word-by-word matching (case-insensitive)
        if (!foundSong) {
          const searchWords = searchTerm.split(' ');
          foundSong = currentSongs.find(song => {
            const titleWords = song.title.toLowerCase().split(' ');
            const artistWords = song.artist.toLowerCase().split(' ');
            return searchWords.every(word => 
              titleWords.some(titleWord => titleWord.includes(word)) ||
              artistWords.some(artistWord => artistWord.includes(word))
            );
          });
        }
        
        // If still no match, try fuzzy matching for common speech recognition errors
        if (!foundSong) {
          foundSong = currentSongs.find(song => {
            const songTitle = song.title.toLowerCase();
            
            if (songTitle.includes(searchTerm)) return true;
            if (songTitle.startsWith(searchTerm)) return true;
            
            const cleanSongTitle = songTitle.replace(/\b(the|a|an)\b/g, '').trim();
            const cleanSearchTerm = searchTerm.replace(/\b(the|a|an)\b/g, '').trim();
            
            return cleanSongTitle.includes(cleanSearchTerm) || cleanSearchTerm.includes(cleanSongTitle);
          });
        }
        
        if (foundSong) {
          console.log('Voice: Found song:', foundSong.title, 'by', foundSong.artist);
          
          setCurrentTrack(foundSong);
          
          if (!currentPlaylist.some(track => track.title === foundSong.title)) {
            setCurrentPlaylist(prev => [...prev, foundSong]);
          }
          
          setTimeout(() => {
            console.log('Voice: About to play track:', foundSong.title);
            setIsPlaying(true);
            
            if (controlsRef.current?.handlePlaySpecific) {
              controlsRef.current.handlePlaySpecific(foundSong);
            } else {
              const sanitizedTrack = sanitizeTrackForESP32(foundSong);
              esp32.post('/play', sanitizedTrack)
                .then(response => {
                  console.log('Voice: Successfully started playing on ESP32:', response.data);
                  setIsPlaying(true);
                })
                .catch(error => {
                  console.error('Voice: Error playing track on ESP32:', error);
                  setIsPlaying(false);
                });
            }
          }, 100);
          
          console.log('Voice: Set to play specific song:', foundSong.title);
        } else {
          console.log('Voice: Song not found:', data);
          console.log('Available songs for debugging:');
          currentSongs.forEach((song, index) => {
            console.log(`${index + 1}. "${song.title}" by ${song.artist} (${song.genre})`);
          });
          
          setQuery(String(data)); // Convert to string here too
          setDropdownActive(true);
        }
      } else {
        console.error('Voice: Invalid data received for playSpecificSong:', data, typeof data);
        console.log('Voice: Expected string, got:', data);
      }
      break;
      
    case 'playByArtist':
      if (data) {
        const artistSongs = currentSongs.filter(song => 
          song.artist.toLowerCase().includes(data.toLowerCase())
        );
        if (artistSongs.length > 0) {
          setCurrentPlaylist(artistSongs);
          setCurrentTrack(artistSongs[0]);
          setIsPlaying(true);
          console.log(`Voice: Playing songs by ${data}, found ${artistSongs.length} tracks`);
        } else {
          console.log('Voice: No songs found by artist:', data);
          setQuery(`by ${data}`);
          setDropdownActive(true);
        }
      }
      break;
      
    case 'addToPlaylist':
      if (query) {
        handleAddToPlaylist();
        console.log('Voice: Adding to playlist');
      } else {
        console.log('Voice: No search query to add to playlist');
      }
      break;
      
    case 'clearPlaylist':
      setCurrentPlaylist([]);
      setCurrentTrack(null);
      setIsPlaying(false);
      console.log('Voice: Playlist cleared');
      break;
      
    default:
      console.log('Voice: Unknown command:', action);
  }
}, [currentTrack, currentPlaylist, query, controlsRef, setIsPlaying, setCurrentTrack, setCurrentPlaylist, setQuery, setDropdownActive]); // Remove songs from dependencies, use ref instead

  // Rest of your component...
  // Add this helper function near the top of your App component:
const sanitizeTrackForESP32 = (track) => {
  const cleanAndSanitize = (str) => {
    if (!str) return '';
    
    return str.replace(/[\r\n\t\f\v]/g, ' ')
             .trim()
             .replace(/\s+/g, '_')
             .replace(/_+$/, '')
             .replace(/^_+/, '');
  };

  return {
    ...track,
    title: cleanAndSanitize(track.title),
    artist: cleanAndSanitize(track.artist),
    genre: cleanAndSanitize(track.genre)
  };
};

  return (
    <div className="app-container">
      <header className="app-header">
        <div className="header-content">
          <div className="header-left">
            {/* Voice Control with compact icons */}
            <VoiceControl 
              onCommand={handleVoiceCommand}
              currentTrack={currentTrack}
              isPlaying={isPlaying}
              currentPlaylist={currentPlaylist}
              songs={songs}  // Add this line to pass songs array
              compact={true} // Add compact mode prop
            />
          </div>
          
          <div className="header-center">
            <h1>gAItar: the robotic guitarist</h1>
          </div>
          
          <div className="header-right">
            {/* You could add other header controls here */}
          </div>
        </div>
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
                onFocus={() => setDropdownActive(true)}
                onBlur={handleBlur}
                className="search-input"
              />
              {dropdownActive && suggestions.length > 0 && (
                <ul className="suggestions-list active">
                  {suggestions.map((song, index) => (
                    <li
                      key={index}
                      className="suggestion-item"
                      tabIndex="0"
                      onMouseDown={() => handleSuggestionClick(song)}
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
          
          <div className="voice-generator">
            <VoiceToMIDI 
              onUploadToForm={handleVoiceToMidiUpload} 
            />
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
          
          <PlaybackInfo currentTrack={currentTrack} />
          
          <Controls
            ref={controlsRef}
            currentTrack={currentTrack}
            setCurrentTrack={setCurrentTrack}
            currentPlaylist={currentPlaylist}
            setCurrentPlaylist={setCurrentPlaylist}
            setIsPlaying={setIsPlaying}
          />
          
          <div className="midi-generator">
            <MidiGenerator 
              onUploadToForm={handleVoiceToMidiUpload} 
            />
          </div>
        </div>

        <div className="right-column">
          <div className="playlist">
            <Playlist 
              currentPlaylist={currentPlaylist} 
              setCurrentPlaylist={setCurrentPlaylist}
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
            setSongs={setSongs}
          />
        </div>
      </div>
    </div>
  );
};

export default App;
