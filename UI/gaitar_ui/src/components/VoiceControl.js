import React, { useState, useEffect, useRef } from 'react';
import { FaMicrophone, FaMicrophoneSlash } from 'react-icons/fa';

// Enhanced command map with more actions
const commandMap = {
  // Playback controls
  'play': 'play',
  'pause': 'pause',
  'stop': 'pause',
  'skip': 'skip',
  'next': 'skip',
  'previous': 'previous',
  'back': 'previous',
  'shuffle': 'shuffle',
  
  // Search and selection
  'search': 'search',
  'find': 'search',
  'add to playlist': 'addToPlaylist',
  'add song': 'addToPlaylist',
  
  // Generation
  'generate midi': 'generateMidi',
  'create music': 'generateMidi',
  'record voice': 'recordVoice',
  'voice recording': 'recordVoice',
  
  // Playlist management
  'clear playlist': 'clearPlaylist',
  'show playlist': 'showPlaylist',
};

const VoiceControl = ({ 
  onCommand, 
  currentTrack, 
  isPlaying,
  currentPlaylist 
}) => {
  const [isListening, setIsListening] = useState(false);
  const [recognizedText, setRecognizedText] = useState('');
  const [status, setStatus] = useState('Ready');
  const [isEnabled, setIsEnabled] = useState(true);
  
  const recognitionRef = useRef(null);

  useEffect(() => {
    if (!isEnabled) return;

    // Check if SpeechRecognition is supported
    if (!('webkitSpeechRecognition' in window) && !('SpeechRecognition' in window)) {
      setStatus('Speech Recognition not supported');
      return;
    }

    const SpeechRecognition = window.webkitSpeechRecognition || window.SpeechRecognition;
    const recognition = new SpeechRecognition();
    
    recognition.continuous = false;
    recognition.interimResults = false;
    recognition.lang = 'en-US';
    recognition.maxAlternatives = 3;

    recognition.onstart = () => {
      setIsListening(true);
      setStatus('Listening...');
    };

    recognition.onresult = (event) => {
      const results = Array.from(event.results[0]).map(result => result.transcript.toLowerCase());
      const command = results[0];
      setRecognizedText(command);
      
      console.log('Recognized command:', command);
      console.log('Alternative results:', results);
      
      // Enhanced command matching with context
      const matchedCommand = findBestMatch(command, results);
      
      if (matchedCommand) {
        onCommand(matchedCommand.action, matchedCommand.data);
        setStatus(`✅ ${matchedCommand.action} executed`);
      } else {
        setStatus(`❌ Command not recognized: "${command}"`);
      }
    };

    recognition.onend = () => {
      setIsListening(false);
      if (status === 'Listening...') {
        setStatus('Ready');
      }
    };

    recognition.onerror = (event) => {
      setIsListening(false);
      setStatus(`Error: ${event.error}`);
      console.error('Speech recognition error:', event.error);
    };

    recognitionRef.current = recognition;

    return () => {
      if (recognitionRef.current) {
        recognitionRef.current.stop();
      }
    };
  }, [onCommand, isEnabled]);

  // Enhanced command matching function
  const findBestMatch = (command, alternatives) => {
    // Try exact matches first
    for (const [key, action] of Object.entries(commandMap)) {
      if (command.includes(key)) {
        return { action, data: null };
      }
    }

    // Check for song search commands
    if (command.includes('play') && (command.includes('song') || command.includes('track'))) {
      const songName = extractSongName(command);
      if (songName) {
        return { action: 'playSpecificSong', data: songName };
      }
    }

    // Check for artist/genre commands
    if (command.includes('play') && command.includes('by')) {
      const artist = extractArtistName(command);
      if (artist) {
        return { action: 'playByArtist', data: artist };
      }
    }

    // Try alternative results
    for (const alt of alternatives) {
      for (const [key, action] of Object.entries(commandMap)) {
        if (alt.includes(key)) {
          return { action, data: null };
        }
      }
    }

    return null;
  };

  const extractSongName = (command) => {
    // Extract song name from commands like "play song [song name]" or "play [song name]"
    const patterns = [
      /play song (.+)/,
      /play track (.+)/,
      /play (.+)/
    ];
    
    for (const pattern of patterns) {
      const match = command.match(pattern);
      if (match) {
        return match[1].trim();
      }
    }
    return null;
  };

  const extractArtistName = (command) => {
    // Extract artist name from commands like "play something by [artist]"
    const match = command.match(/by (.+)/);
    return match ? match[1].trim() : null;
  };

  const handleButtonClick = () => {
    if (isListening) {
      recognitionRef.current?.stop();
    } else {
      recognitionRef.current?.start();
    }
  };

  const toggleVoiceControl = () => {
    setIsEnabled(!isEnabled);
    if (isListening) {
      recognitionRef.current?.stop();
    }
    setStatus(isEnabled ? 'Disabled' : 'Ready');
  };

  return (
    <div className="voice-control">
      <h3>Voice Control</h3>
      
      <div className="voice-control-header">
        <button 
          onClick={toggleVoiceControl}
          className={`voice-toggle-button ${isEnabled ? 'enabled' : 'disabled'}`}
        >
          {isEnabled ? 'Enabled' : 'Disabled'}
        </button>
      </div>

      <div className="voice-status">
        <p className="status-text">Status: <span className={getStatusClass()}>{status}</span></p>
        {recognizedText && (
          <p className="recognized-text">Last command: "{recognizedText}"</p>
        )}
      </div>

      <button 
        onClick={handleButtonClick}
        disabled={!isEnabled}
        className={`voice-button ${isListening ? 'listening' : ''}`}
      >
        {isListening ? <FaMicrophoneSlash /> : <FaMicrophone />}
        {isListening ? 'Stop Listening' : 'Start Listening'}
      </button>

      <div className="voice-commands-help">
        <details>
          <summary>Available Commands</summary>
          <div className="commands-grid">
            <div>
              <h4>Playback</h4>
              <ul>
                <li>"play" / "pause"</li>
                <li>"skip" / "next"</li>
                <li>"previous" / "back"</li>
                <li>"shuffle"</li>
              </ul>
            </div>
            <div>
              <h4>Search & Add</h4>
              <ul>
                <li>"search [song name]"</li>
                <li>"play [song name]"</li>
                <li>"add to playlist"</li>
                <li>"play by [artist]"</li>
              </ul>
            </div>
            <div>
              <h4>Generation</h4>
              <ul>
                <li>"generate midi"</li>
                <li>"record voice"</li>
                <li>"create music"</li>
              </ul>
            </div>
          </div>
        </details>
      </div>

      {currentTrack && (
        <div className="current-context">
          <p>Now: {isPlaying ? '▶️' : '⏸️'} {currentTrack.title}</p>
          <p>Playlist: {currentPlaylist.length} songs</p>
        </div>
      )}
    </div>
  );

  function getStatusClass() {
    if (status.includes('✅')) return 'status-success';
    if (status.includes('❌')) return 'status-error';
    if (status === 'Listening...') return 'status-listening';
    return 'status-normal';
  }
};

export default VoiceControl;
