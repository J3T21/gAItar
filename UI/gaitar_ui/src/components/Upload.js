import React, { useState } from 'react';
import { backend_api, esp32 } from '../api'; // Import the API instance
import '../css/upload.css'; // Import the CSS file for styling

const Upload = ({ genres, artists, setTrackMetadata, onUpload, addArtist }) => {
  const [file, setFile] = useState(null);
  const [genre, setGenre] = useState('');
  const [title, setTitle] = useState('');
  const [artist, setArtist] = useState('');
  const [newGenre, setNewGenre] = useState('');
  const [newArtist, setNewArtist] = useState('');
  const [searchTerm, setSearchTerm] = useState(''); // State for genre search
  const [artistSearchTerm, setArtistSearchTerm] = useState(''); // State for artist search

  // Handle file input change
  const handleFileChange = (e) => {
    const uploadedFile = e.target.files[0];
    if (uploadedFile) {
      setFile(uploadedFile);
    }
  };

  // Handle file upload
  const handleUpload = async () => {
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
          const json = JSON.stringify(response.data);
          console.log("JSON size (bytes):", new TextEncoder().encode(json).length);
          const filename = `${title}_${artist}.json`;
          const blob = new Blob([json], { type: 'application/json' });
          const file = new File([blob], filename, { type: 'application/json' });
          const espData = new FormData();
          espData.append('data', file);
        
          try {
            await esp32.post('/upload', espData, {
              headers: {
                'Content-Type': 'multipart/form-data',
              },}); 
            console.log('midi sent to ESP32 successfully');
          } catch (espError) {
            console.error('Failed to send midi to ESP32:', espError);
          }
        
          onUpload(response.data.title, genre); // Pass title and genre
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
    if (newGenre) {
      // In the actual implementation, you would need to update the genre list in your app
      // For example, calling a function like `addGenre(newGenre)`
      console.log('Adding new genre:', newGenre);
      setNewGenre('');
    }
  };

  // Handle adding a new artist
  const handleAddArtist = () => {
    if (newArtist) {
      addArtist(newArtist);  // Assuming addArtist is passed as a prop to update artist list
      console.log('Adding new artist:', newArtist);
      setNewArtist('');
    }
  };

  // Handle genre selection
  const handleGenreSelect = (selectedGenre) => {
    setGenre(selectedGenre);
  };

  // Handle artist selection
  const handleArtistSelect = (selectedArtist) => {
    setArtist(selectedArtist);
  };

  // Filter genres and artists based on search term
  const filteredGenres = genres.filter((g) =>
    g.name.toLowerCase().includes(searchTerm.toLowerCase())
  );

  const filteredArtists = artists.filter((a) =>
    a.name.toLowerCase().includes(artistSearchTerm.toLowerCase())
  );

  return (
    <div>
      <h4>Upload a MIDI File</h4>
      {/* File upload section */}
      <input type="file" onChange={handleFileChange} accept=".midi, .mid" />
      <input
        type="text"
        placeholder="Track Title"
        value={title}
        onChange={(e) => setTitle(e.target.value)}
      />

      {/* Artist selection and add new artist section */}
      <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
        <div className="artist-selection-container">
          <input
            type="text"
            placeholder="Search or Add Artist"
            value={artistSearchTerm}
            onChange={(e) => {
              setArtistSearchTerm(e.target.value);
              setArtist(e.target.value); // Set artist to the current input value
            }}
            className="artist-input"
            list="artist-options"
          />
          <datalist id="artist-options">
            {filteredArtists.map((artist, index) => (
              <option key={index} value={artist.name} />
            ))}
          </datalist>
        </div>

        <div className="add-artist-container">
          <button onClick={handleAddArtist} className="add-artist-button">
            Add Artist
          </button>
        </div>
      </div>

      {/* Genre selection and add new genre section */}
      <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
        <div className="genre-selection-container">
          <input
            type="text"
            placeholder="Search or Add Genre"
            value={searchTerm}
            onChange={(e) => {
              setSearchTerm(e.target.value);
              setGenre(e.target.value); // Set genre to the current input value
            }}
            list="genre-options"
          />
          <datalist id="genre-options">
            {filteredGenres.map((genre, index) => (
              <option key={index} value={genre.name} />
            ))}
          </datalist>
        </div>

        <div className="add-genre-container">
          <button onClick={handleAddGenre}>Add Genre</button>
        </div>
      </div>

      {/* Upload button */}
      <button onClick={handleUpload}>Upload</button>
    </div>
  );
};

export default Upload;
