import React, { useState, useEffect, useRef } from 'react';
import { backend_api, esp32 } from '../api';

const Upload = ({ 
  genres = [], 
  artists = [], 
  setTrackMetadata, 
  onUpload, 
  setSongs: updateGlobalSongs, // Rename to avoid confusion
  onVoiceToMidiUpload
}) => {
  // Keep local songs state for upload component functionality
  const [songs, setSongs] = useState([]);
  
  const [file, setFile] = useState(null);
  const [genre, setGenre] = useState('');
  const [title, setTitle] = useState('');
  const [artist, setArtist] = useState('');
  const [newGenre, setNewGenre] = useState('');
  const [newArtist, setNewArtist] = useState('');
  const [searchTerm, setSearchTerm] = useState('');
  const [artistSearchTerm, setArtistSearchTerm] = useState('');
  const [error, setError] = useState('');
  const [showGenreDropdown, setShowGenreDropdown] = useState(false);
  const [showArtistDropdown, setShowArtistDropdown] = useState(false);
  const [isCustomFile, setIsCustomFile] = useState(false);
  const [uploadMessage, setUploadMessage] = useState('');
  
  // New states for ESP32 upload status
  const [uploadStatus, setUploadStatus] = useState('idle');
  const [uploadProgress, setUploadProgress] = useState('');
  const [isUploading, setIsUploading] = useState(false);
  const [uploadPercentage, setUploadPercentage] = useState(0);

  // WebSocket reference and connection state
  const wsRef = useRef(null);
  const [wsConnected, setWsConnected] = useState(false);
  const reconnectTimeoutRef = useRef(null);
  const reconnectAttemptsRef = useRef(0);
  const uploadTimeoutRef = useRef(null);

  // Add a ref to store the original upload data
  const uploadDataRef = useRef(null);

  // Initialize WebSocket connection with unlimited reconnection attempts
  useEffect(() => {
    const connectWebSocket = () => {
      try {
        // Clean up existing connection
        if (wsRef.current) {
          wsRef.current.close();
        }

        console.log('Attempting to connect to WebSocket...');
        wsRef.current = new WebSocket('ws://10.245.188.200/ws');
        
        wsRef.current.onopen = () => {
          console.log('WebSocket connected to ESP32');
          setWsConnected(true);
          reconnectAttemptsRef.current = 0; // Reset reconnect attempts
          
          // Clear any existing reconnect timeout
          if (reconnectTimeoutRef.current) {
            clearTimeout(reconnectTimeoutRef.current);
            reconnectTimeoutRef.current = null;
          }
        };
        
        wsRef.current.onmessage = (event) => {
          try {
            const data = JSON.parse(event.data);
            
            // Clear upload timeout since we received a message
            if (uploadTimeoutRef.current) {
              clearTimeout(uploadTimeoutRef.current);
              uploadTimeoutRef.current = null;
            }
            
            // Update progress based on ESP32 feedback
            if (data.stage && data.percentage !== undefined && data.message) {
              setUploadProgress(`${data.stage}: ${data.message}`);
              setUploadPercentage(data.percentage);
              
              // Check if upload is complete
              if (data.percentage === 100 || data.stage === 'Complete') {
                setUploadStatus('completed');
                setUploadProgress('Upload completed successfully!');
                setIsUploading(false);
                setUploadPercentage(100);
                
                // Use the stored upload data instead of current state
                if (uploadDataRef.current) {
                  console.log('Upload completed, using stored data:', uploadDataRef.current);
                  handleUploadSuccess({
                    title: uploadDataRef.current.title,
                    artist: uploadDataRef.current.artist,
                    genre: uploadDataRef.current.genre,
                    duration_formatted: '00:00'
                  });
                } else {
                  console.error('Upload completed but no stored data found!');
                }
                
              } else if (data.stage === 'Error' || data.message.includes('failed')) {
                setUploadStatus('failed');
                setUploadProgress('Upload failed: ' + data.message);
                setError('Upload failed: ' + data.message);
                setIsUploading(false);
                // Clear the stored data on failure
                uploadDataRef.current = null;
              } else {
                // Reset upload timeout for ongoing uploads
                setUploadTimeout();
              }
            }
          } catch (err) {
            // Handle non-JSON messages quietly
          }
        };
        
        wsRef.current.onclose = (event) => {
          console.log('WebSocket disconnected from ESP32. Code:', event.code, 'Reason:', event.reason);
          setWsConnected(false);
          
          // Always attempt to reconnect with no limit
          reconnectAttemptsRef.current++;
          const delay = Math.min(1000 * Math.pow(2, Math.min(reconnectAttemptsRef.current - 1, 5)), 30000); // Cap exponential backoff at 2^5 = 32s, max 30s
          
          console.log(`Attempting to reconnect in ${delay/1000}s (attempt ${reconnectAttemptsRef.current})`);
          
          // Only show reconnection progress if actively uploading
          if (isUploading) {
            setUploadProgress(`Connection lost. Reconnecting in ${delay/1000}s...`);
          }
          
          reconnectTimeoutRef.current = setTimeout(() => {
            connectWebSocket();
          }, delay);
        };
        
        wsRef.current.onerror = (error) => {
          console.error('WebSocket error:', error);
          setWsConnected(false);
        };

      } catch (err) {
        console.error('Failed to create WebSocket connection:', err);
        setWsConnected(false);
        
        // Always retry connection
        reconnectAttemptsRef.current++;
        const delay = 3000;
        reconnectTimeoutRef.current = setTimeout(connectWebSocket, delay);
      }
    };

    // Set up upload timeout (in case ESP32 goes silent during upload)
    const setUploadTimeout = () => {
      if (uploadTimeoutRef.current) {
        clearTimeout(uploadTimeoutRef.current);
      }
      
      // Set a 2-minute timeout for upload silence
      uploadTimeoutRef.current = setTimeout(() => {
        if (isUploading) {
          console.warn('Upload timeout - no progress updates received');
          setUploadStatus('failed');
          setUploadProgress('Upload timed out - no progress updates received');
          setError('Upload timed out. The ESP32 may have encountered an issue.');
          setIsUploading(false);
        }
      }, 120000); // 2 minutes
    };

    connectWebSocket();

    // Cleanup on unmount
    return () => {
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
      if (uploadTimeoutRef.current) {
        clearTimeout(uploadTimeoutRef.current);
      }
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, []); // Remove isUploading dependency to prevent reconnection loops

  // Handle file input change
  const handleFileChange = (e) => {
    const uploadedFile = e.target.files[0];
    if (uploadedFile) {
      setFile(uploadedFile);
      setIsCustomFile(false);
      setUploadMessage('');
      setUploadStatus('idle');
      setUploadProgress('');
      setUploadPercentage(0);
    }
  };

  // Listen for custom events from VoiceToMIDI
  useEffect(() => {
    const handleVoiceMidiUpload = (event) => {
      const midiFile = event.detail;
      setFile(midiFile);
      setIsCustomFile(true);
      setUploadMessage('Please fill in the upload fields');
      setUploadStatus('idle');
      setUploadProgress('');
      setUploadPercentage(0);
      
      setTimeout(() => {
        setUploadMessage('');
      }, 8000);
    };

    document.addEventListener('voiceMidiUpload', handleVoiceMidiUpload);
    
    return () => {
      document.removeEventListener('voiceMidiUpload', handleVoiceMidiUpload);
    };
  }, [title]);

  // Main upload function
  // Update the handleUpload function in Upload.js
const handleUpload = async () => {
  if (!file || !genre || !title || !artist) {
    setError('Please fill in all fields (file, genre, title, and artist)');
    return;
  }
  
  if (isUploading) {
    setError('Upload already in progress. Please wait...');
    return;
  }

  // Check WebSocket connection before starting upload
  if (!wsConnected) {
    setError('WebSocket not connected. Please wait for connection or refresh the page.');
    return;
  }
  
  // Store the upload data in a ref so it persists during the upload process
  uploadDataRef.current = {
    title: title,
    artist: artist,
    genre: genre,
    fileName: file.name
  };
  
  setError('');
  setUploadMessage('');
  setIsUploading(true);
  setUploadStatus('uploading');
  setUploadProgress('Processing file...');
  setUploadPercentage(0);

  console.log('Uploading file:', file);
  console.log('Upload data stored:', uploadDataRef.current);
  
  try {
    // Step 1: Send to backend for binary processing
    const formData = new FormData();
    formData.append('midi_file', file);
    formData.append('genre', genre);
    formData.append('title', title);
    formData.append('artist', artist);

    setUploadProgress('Converting MIDI to binary format...');
    setUploadPercentage(10);
    
    // Use the new binary endpoint
    const response = await backend_api.post('/upload-midi-binary', formData, {
      headers: {
        'Content-Type': 'multipart/form-data',
      },
      responseType: 'arraybuffer' // Important: get binary data
    });

    // Extract metadata from response headers
    const responseHeaders = response.headers;
    const binaryMetadata = {
      title: responseHeaders['x-title'] || title,
      artist: responseHeaders['x-artist'] || artist,
      genre: responseHeaders['x-genre'] || genre,
      duration_formatted: responseHeaders['x-duration'] || '00:00',
      file_size: responseHeaders['x-size'] || '0',
      event_count: responseHeaders['x-event-count'] || '0'
    };

    console.log('Binary metadata from headers:', binaryMetadata);
    setTrackMetadata(binaryMetadata);
    setUploadPercentage(30);

    // Step 2: Prepare binary data for ESP32 transmission
    setUploadProgress('Preparing binary data for ESP32...');
    
    // Get the binary data as ArrayBuffer
    const binaryData = response.data;
    console.log('Binary data size (bytes):', binaryData.byteLength);
    console.log('Event count:', binaryMetadata.event_count);
    
    // Clean the metadata for ESP32 (remove spaces and special characters)
    const sanitizedTitle = binaryMetadata.title.replace(/\s+/g, '_').replace(/[^a-zA-Z0-9_]/g, '');
    const sanitizedArtist = binaryMetadata.artist.replace(/\s+/g, '_').replace(/[^a-zA-Z0-9_]/g, '');
    const sanitizedGenre = binaryMetadata.genre.replace(/\s+/g, '_').replace(/[^a-zA-Z0-9_]/g, '');

    // Create a blob from the binary data
    const binaryBlob = new Blob([binaryData], { type: 'application/octet-stream' });
    
    // Create FormData for ESP32 upload
    const espData = new FormData();
    espData.append('data', binaryBlob, 'guitar_events.bin'); // Binary file
    espData.append('artist', sanitizedArtist);
    espData.append('title', sanitizedTitle);
    espData.append('genre', sanitizedGenre);
    espData.append('duration', binaryMetadata.duration_formatted);
    espData.append('file_size', binaryMetadata.file_size);
    espData.append('event_count', binaryMetadata.event_count);

    console.log('ESP32 FormData prepared with binary file:', binaryBlob.size, 'bytes');
    setUploadPercentage(40);

    // Step 3: Send binary data to ESP32 with progress tracking
    setUploadProgress('Uploading binary data to ESP32. Watch for real-time progress...');
    
    // Set upload timeout
    if (uploadTimeoutRef.current) {
      clearTimeout(uploadTimeoutRef.current);
    }
    uploadTimeoutRef.current = setTimeout(() => {
      if (isUploading) {
        setUploadStatus('failed');
        setUploadProgress('Upload timed out - no response from ESP32');
        setError('Upload timed out. Please try again.');
        setIsUploading(false);
      }
    }, 120000); // 2 minutes timeout
    
    try {
      const espResponse = await esp32.post('/upload-binary', espData, {
        headers: {
          'Content-Type': 'multipart/form-data',
        },
        timeout: 15000, // 15 second timeout for initial response
      });
      
      console.log('ESP32 binary upload response:', espResponse.data);
      
      if (espResponse.status === 200) {
        // Initial upload successful - progress will be tracked via WebSocket
        setUploadProgress('Binary data sent to ESP32. Processing...');
        setUploadPercentage(50);
        // Don't set completed here - wait for WebSocket confirmation
      } else {
        throw new Error('Unexpected ESP32 response: ' + espResponse.data);
      }
      
    } catch (espError) {
      console.error('Failed to send binary data to ESP32:', espError);
      
      // Clear upload timeout
      if (uploadTimeoutRef.current) {
        clearTimeout(uploadTimeoutRef.current);
        uploadTimeoutRef.current = null;
      }
      
      if (espError.response && espError.response.status === 501) {
        setUploadStatus('failed');
        setUploadProgress('ESP32 binary upload not implemented');
        setError('ESP32 binary upload endpoint returned 501 - Not Implemented. The route may not be properly configured.');
      } else {
        setUploadStatus('failed');
        setUploadProgress('Upload failed');
        setError('Failed to upload to ESP32: ' + espError.message);
      }
      setIsUploading(false);
      setUploadPercentage(0);
      return;
    }
    
  } catch (error) {
    console.error('Error in binary upload process:', error);
    setError('Upload failed: ' + error.message);
    setUploadStatus('failed');
    setUploadProgress('');
    setUploadPercentage(0);
    setIsUploading(false);
    
    // Clear upload timeout
    if (uploadTimeoutRef.current) {
      clearTimeout(uploadTimeoutRef.current);
      uploadTimeoutRef.current = null;
    }
  }
};

  // Handle successful upload completion (called when WebSocket confirms success)
  const handleUploadSuccess = (responseData) => {
    console.log('handleUploadSuccess called with:', responseData);
    
    // Clear upload timeout
    if (uploadTimeoutRef.current) {
      clearTimeout(uploadTimeoutRef.current);
      uploadTimeoutRef.current = null;
    }

    // Validate the response data
    if (!responseData.title || !responseData.artist || !responseData.genre) {
      console.error('Invalid response data in handleUploadSuccess:', responseData);
      setError('Upload completed but song data is incomplete');
      return;
    }

    const newSong = {
      title: responseData.title,
      artist: responseData.artist,
      genre: responseData.genre,
      duration_formatted: responseData.duration_formatted || '00:00',
    };

    console.log('Creating new song object:', newSong);

    // Update local songs list (for upload component dropdowns)
    setSongs((prevSongs) => {
      console.log('Updating local songs list, previous count:', prevSongs.length);
      const updated = [...prevSongs, newSong];
      console.log('Updated local songs list, new count:', updated.length);
      return updated;
    });

    // Update the main App.js songs list
    if (updateGlobalSongs && typeof updateGlobalSongs === 'function') {
      console.log('Updating global songs list');
      updateGlobalSongs((prevSongs) => {
        console.log('Global songs update, previous count:', prevSongs.length);
        const updated = [...prevSongs, newSong];
        console.log('Global songs update, new count:', updated.length);
        return updated;
      });
    } else {
      console.error('updateGlobalSongs function not available:', typeof updateGlobalSongs);
    }

    // Clear the input fields AFTER storing the data
    setFile(null);
    setGenre('');
    setTitle('');
    setArtist('');
    setArtistSearchTerm('');
    setSearchTerm('');
    setIsCustomFile(false);
    
    // Clear the stored upload data
    uploadDataRef.current = null;

    console.log('Song added to both local and global lists:', responseData.title);
    onUpload(responseData.title, responseData.genre);
  };

  // Cancel upload function
  const cancelUpload = () => {
    if (uploadTimeoutRef.current) {
      clearTimeout(uploadTimeoutRef.current);
      uploadTimeoutRef.current = null;
    }
    
    setIsUploading(false);
    setUploadStatus('idle');
    setUploadProgress('');
    setUploadPercentage(0);
    setError('Upload cancelled by user');
  };

  // Handle adding a new genre
  const handleAddGenre = () => {
    if (newGenre.trim() !== '') {
      setSongs((prevSongs) => {
        const genreExists = prevSongs.some((song) => song.genre === newGenre);
        if (!genreExists) {
          return [...prevSongs, { title: '', artist: '', genre: newGenre }];
        }
        return prevSongs;
      });
      setNewGenre('');
    }
  };

  // Handle adding a new artist
  const handleAddArtist = () => {
    if (newArtist.trim() !== '') {
      setSongs((prevSongs) => {
        const artistExists = prevSongs.some((song) => song.artist === newArtist);
        if (!artistExists) {
          return [...prevSongs, { title: '', artist: newArtist, genre: '' }];
        }
        return prevSongs;
      });
      setNewArtist('');
    }
  };

  // Filter suggestions for artist and genre (limited to top 5)
  const filteredArtistSuggestions = artists
    .filter(artist =>
      artist && artist.toLowerCase().includes(artistSearchTerm.toLowerCase())
    )
    .slice(0, 5);

  const filteredGenreSuggestions = genres
    .filter(genre =>
      genre && genre.toLowerCase().includes(searchTerm.toLowerCase())
    )
    .slice(0, 5);

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

  // Get status message styling
  const getStatusMessageClass = () => {
    switch (uploadStatus) {
      case 'uploading':
        return 'upload-progress-message';
      case 'completed':
        return 'upload-success-message';
      case 'failed':
        return 'error-message';
      default:
        return '';
    }
  };

  return (
    <div>
      {error && (
        <div className="error-message">
          {error}
        </div>
      )}

      {/* Display upload message in Upload component */}
      {uploadMessage && (
        <div className="upload-success-message">
          {uploadMessage}
        </div>
      )}

      {/* Upload status and progress */}
      {uploadProgress && (
        <div className={getStatusMessageClass()}>
          {uploadProgress}
          {uploadStatus === 'uploading' && (
            <div className="upload-progress-container">
              <div className="upload-spinner">⟳</div>
              <div className="upload-percentage">{uploadPercentage}%</div>
              <button 
                onClick={cancelUpload}
                className="cancel-upload-button"
                style={{ marginLeft: '10px', fontSize: '12px' }}
              >
                Cancel
              </button>
            </div>
          )}
        </div>
      )}
      
      {/* Custom file input with conditional display */}
      <div className="file-input-container">
        <input 
          type="file" 
          onChange={handleFileChange} 
          accept=".midi, .mid"
          style={{ display: 'none' }}
          id="file-upload"
          disabled={isUploading}
        />
        <label htmlFor="file-upload" className={`file-upload-label ${isUploading ? 'disabled' : ''}`}>
          {file ? (
            isCustomFile ? (
              <span className="file-status custom">Custom file added</span>
            ) : (
              <span className="file-status selected">{file.name}</span>
            )
          ) : (
            <span className="file-status">Choose MIDI file</span>
          )}
        </label>
      </div>

      <input
        type="text"
        placeholder="Track Title"
        value={title}
        onChange={(e) => setTitle(e.target.value)}
        disabled={isUploading}
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
          disabled={isUploading}
        />
        {showArtistDropdown && filteredArtistSuggestions.length > 0 && !isUploading && (
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
          disabled={isUploading}
        />
        {showGenreDropdown && filteredGenreSuggestions.length > 0 && !isUploading && (
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
      <button 
        onClick={handleUpload} 
        disabled={isUploading || !wsConnected}
        className={isUploading || !wsConnected ? 'upload-button-disabled' : ''}
      >
        {isUploading ? `Uploading... ${uploadPercentage}%` : 
         !wsConnected ? 'Connecting...' : 'Upload'}
      </button>
    </div>
  );
};

export default Upload;