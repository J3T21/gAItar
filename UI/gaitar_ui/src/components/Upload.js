import React, { useState } from 'react';
import { backend_api, esp32 } from '../api'; // Import the API instance

const Upload = ({ genres = [], artists = [], setTrackMetadata, onUpload, setSongs }) => {
  const [file, setFile] = useState(null);
  const [genre, setGenre] = useState('');
  const [title, setTitle] = useState('');
  const [artist, setArtist] = useState('');
  const [newGenre, setNewGenre] = useState('');
  const [newArtist, setNewArtist] = useState('');
  const [searchTerm, setSearchTerm] = useState(''); // State for genre search
  const [artistSearchTerm, setArtistSearchTerm] = useState(''); // State for artist search
  const [error, setError] = useState('');
  const [showArtistDropdown, setShowArtistDropdown] = useState(false);
  const [showGenreDropdown, setShowGenreDropdown] = useState(false);

  // Handle file input change
  const handleFileChange = (e) => {
    const uploadedFile = e.target.files[0];
    if (uploadedFile) {
      setFile(uploadedFile);
    }
  };

  // Handle file upload
  const handleUpload = async () => {
    if (!file || !genre || !title || !artist) {
      setError('Please fill in all fields (file, genre, title, and artist)');
      return;
    }
    setError(''); // Clear any previous error

    console.log('Uploading file:', file);
    if (file && genre && title && artist) {
      const formData = new FormData();
      formData.append('midi_file', file);
      formData.append('genre', genre);
      formData.append('title', title);
      formData.append('artist', artist);

      try {
        // Send the form data to the backend
        const response = await backend_api.post('/upload-midi', formData, {
          headers: {
            'Content-Type': 'multipart/form-data',
          },
        });

        // Process the response
        if (response.data) {
          // Update track metadata in App component
          setTrackMetadata({
            title: response.data.title,
            artist: response.data.artist,
            genre: response.data.genre,
            duration_formatted: response.data.duration_formatted,
          });

          console.log('Track metadata:', response.data);

          // Prepare data for ESP32 transmission
          const json = JSON.stringify(response.data);
          console.log("JSON size (bytes):", new TextEncoder().encode(json).length);
          const sanitizedTitle = response.data.title.replace(/\s+/g, '_');
          const sanitizedArtist = response.data.artist.replace(/\s+/g, '_');
          const sanitizedGenre = response.data.genre.replace(/\s+/g, '_');
          const filename = `temp.json`;
          const blob = new Blob([json], { type: 'application/json' });
          const file = new File([blob], filename, { type: 'application/json' });
          const espData = new FormData();
          espData.append('data', file); // the JSON file
          espData.append('title', sanitizedTitle);
          espData.append('artist', sanitizedArtist);
          espData.append('genre', sanitizedGenre);

          try {
            await esp32.post('/upload', espData, {
              headers: {
                'Content-Type': 'multipart/form-data',
              },
            });
            console.log('MIDI sent to ESP32 successfully');
          } catch (espError) {
            console.error('Failed to send MIDI to ESP32:', espError);
          }

          // Add the new song to the songs list
          setSongs((prevSongs) => [
            ...prevSongs,
            {
              title: response.data.title,
              artist: response.data.artist,
              genre: response.data.genre,
            },
          ]);

          // Clear the input fields
          setFile(null);
          setGenre('');
          setTitle('');
          setArtist('');
          setArtistSearchTerm('');
          setSearchTerm('');

          console.log('Song added to the list:', response.data.title);

          // Notify parent component about the upload
          onUpload(response.data.title, genre);
        }
      } catch (error) {
        console.error('Error uploading MIDI file:', error);
      }
    } else {
      console.log('Please fill in all fields (file, genre, title, and artist)');
    }
  };

  // Handle adding a new genre
  const handleAddGenre = () => {
    if (newGenre.trim() !== '') {
      setSongs((prevSongs) => {
        // Check if the genre already exists
        const genreExists = prevSongs.some((song) => song.genre === newGenre);
        if (!genreExists) {
          // Add a dummy song with the new genre to update the genre list
          return [...prevSongs, { title: '', artist: '', genre: newGenre }];
        }
        return prevSongs;
      });
      setNewGenre(''); // Clear the input field
    }
  };

  // Handle adding a new artist
  const handleAddArtist = () => {
    if (newArtist.trim() !== '') {
      setSongs((prevSongs) => {
        // Check if the artist already exists
        const artistExists = prevSongs.some((song) => song.artist === newArtist);
        if (!artistExists) {
          // Add a dummy song with the new artist to update the artist list
          return [...prevSongs, { title: '', artist: newArtist, genre: '' }];
        }
        return prevSongs;
      });
      setNewArtist(''); // Clear the input field
    }
  };

  // Filter suggestions for artist and genre (limited to top 5)
  const filteredArtistSuggestions = artists
    .filter(artist =>
      artist && artist.toLowerCase().includes(artistSearchTerm.toLowerCase())
    )
    .slice(0, 5); // Limit to top 5 results

  const filteredGenreSuggestions = genres
    .filter(genre =>
      genre && genre.toLowerCase().includes(searchTerm.toLowerCase())
    )
    .slice(0, 5); // Limit to top 5 results

  // Handle artist suggestion click
  const handleArtistSuggestionClick = (selectedArtist) => {
    setArtist(selectedArtist);
    setArtistSearchTerm(selectedArtist);
    setShowArtistDropdown(false);
  };

  // Handle genre suggestion click
  const handleGenreSuggestionClick = (selectedGenre) => {
    setGenre(selectedGenre);
    setSearchTerm(selectedGenre);
    setShowGenreDropdown(false);
  };

  return (
    <div>
      {error && (
        <div style={{ color: 'red', marginBottom: '10px' }}>
          {error}
        </div>
      )}
      <input type="file" onChange={handleFileChange} accept=".midi, .mid" />
      <input
        type="text"
        placeholder="Track Title"
        value={title}
        onChange={(e) => setTitle(e.target.value)}
      />

      {/* Artist selection with dropdown */}
      <div className="artist-selection-container" style={{ position: 'relative' }}>
        <input
          type="text"
          placeholder="Enter or select artist"
          value={artistSearchTerm}
          onChange={(e) => {
            setArtistSearchTerm(e.target.value);
            setArtist(e.target.value);
            setShowArtistDropdown(true);
          }}
          onFocus={() => setShowArtistDropdown(true)}
          onBlur={() => setTimeout(() => setShowArtistDropdown(false), 200)}
          className="artist-input"
        />
        {showArtistDropdown && filteredArtistSuggestions.length > 0 && (
          <ul className="upload-suggestions-list">
            {filteredArtistSuggestions.map((artist, index) => (
              <li
                key={index}
                className="upload-suggestion-item"
                onMouseDown={() => handleArtistSuggestionClick(artist)}
              >
                {artist}
              </li>
            ))}
          </ul>
        )}
      </div>

      {/* Genre selection with dropdown */}
      <div className="genre-selection-container" style={{ position: 'relative' }}>
        <input
          type="text"
          placeholder="Enter or select genre"
          value={searchTerm}
          onChange={(e) => {
            setSearchTerm(e.target.value);
            setGenre(e.target.value);
            setShowGenreDropdown(true);
          }}
          onFocus={() => setShowGenreDropdown(true)}
          onBlur={() => setTimeout(() => setShowGenreDropdown(false), 200)}
          className="genre-input"
        />
        {showGenreDropdown && filteredGenreSuggestions.length > 0 && (
          <ul className="upload-suggestions-list">
            {filteredGenreSuggestions.map((genre, index) => (
              <li
                key={index}
                className="upload-suggestion-item"
                onMouseDown={() => handleGenreSuggestionClick(genre)}
              >
                {genre}
              </li>
            ))}
          </ul>
        )}
      </div>

      {/* Upload button */}
      <button onClick={handleUpload}>Upload</button>
    </div>
  );
};

export default Upload;
