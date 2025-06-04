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
  
  // Playlist management
  'clear playlist': 'clearPlaylist',
};

const VoiceControl = ({ 
  onCommand, 
  currentTrack, 
  isPlaying,
  currentPlaylist,
  songs = [],
  compact = false
}) => {
  const [isListening, setIsListening] = useState(false);
  const [recognizedText, setRecognizedText] = useState('');
  const [status, setStatus] = useState('Ready');
  const [isEnabled, setIsEnabled] = useState(true);
  const [isContinuousListening, setIsContinuousListening] = useState(false);
  const [waitingForCommand, setWaitingForCommand] = useState(false);
  
  const recognitionRef = useRef(null);
  const triggerTimeoutRef = useRef(null);
  const manualTimeoutRef = useRef(null); // Add manual timeout ref

  // Add a new state variable for tracking if manual mode should auto-stop
  const [manualCommandProcessed, setManualCommandProcessed] = useState(false);

  // Debug logging for songs prop
  useEffect(() => {
    console.log('VoiceControl: Received songs array with', songs.length, 'songs');
    if (songs.length > 0) {
      console.log('VoiceControl: First few songs:', songs.slice(0, 3));
    }
  }, [songs]);

  // Permission checking function
  const checkPermissions = async () => {
    try {
      // Check if we're on HTTPS or localhost
      if (window.location.protocol !== 'https:' && window.location.hostname !== 'localhost') {
        setStatus('HTTPS required for voice control');
        return false;
      }

      // Check microphone permissions with better error handling
      let stream;
      try {
        stream = await navigator.mediaDevices.getUserMedia({ 
          audio: {
            echoCancellation: true,
            noiseSuppression: true,
            sampleRate: 16000, // Lower sample rate for speech recognition
            channelCount: 1     // Mono audio
          } 
        });
        
        // Immediately stop the stream to release the microphone
        stream.getTracks().forEach(track => {
          track.stop();
        });
        
        return true;
      } catch (err) {
        console.error('Microphone permission error:', err);
        setStatus(`Microphone error: ${err.name}`);
        return false;
      }
    } catch (err) {
      setStatus('Permission check failed');
      console.error('Permission check failed:', err);
      return false;
    }
  };

  // Initialize speech recognition
  useEffect(() => {
    if (!isEnabled) {
      // Clean up if disabled
      if (recognitionRef.current) {
        recognitionRef.current.stop();
        recognitionRef.current = null;
      }
      // Clear manual timeout too
      if (manualTimeoutRef.current) {
        clearTimeout(manualTimeoutRef.current);
        manualTimeoutRef.current = null;
      }
      setIsListening(false);
      setStatus('Disabled');
      return;
    }

    // Check if SpeechRecognition is supported
    if (!('webkitSpeechRecognition' in window) && !('SpeechRecognition' in window)) {
      setStatus('Speech Recognition not supported');
      return;
    }

    const initializeRecognition = async () => {
      try {
        // Check permissions first
        const hasPermissions = await checkPermissions();
        if (!hasPermissions) return;

        // Clean up existing recognition before creating new one
        if (recognitionRef.current) {
          recognitionRef.current.stop();
          recognitionRef.current = null;
        }

        const SpeechRecognition = window.webkitSpeechRecognition || window.SpeechRecognition;
        const recognition = new SpeechRecognition();
        
        // Always use continuous mode for better command capture
        recognition.continuous = true;
        recognition.interimResults = false;
        recognition.lang = 'en-US';
        recognition.maxAlternatives = 3;

        // Add these properties to improve command capture
        if ('webkitSpeechRecognition' in window) {
          recognition.webkitSpeechRecognitionList = null;
          // Increase timeouts for better command capture
          try {
            recognition.serviceURI = null; // Use default service
            // These might not be available in all browsers but won't hurt to try
            if (recognition.hasOwnProperty('audiostart')) {
              recognition.audiostart = () => console.log('Audio capture started');
            }
          } catch (e) {
            // Ignore if properties don't exist
          }
        }

        let isRestarting = false;
        // Prevent restart loops
        recognition.onstart = () => {
          console.log('Voice Control: Recognition started');
          setIsListening(true);
          if (isContinuousListening) {
            setStatus('Listening for "Hey Guitar"...');
          } else {
            setStatus('Listening...');
            // Set 5-second timeout for manual mode
            if (manualTimeoutRef.current) {
              clearTimeout(manualTimeoutRef.current);
            }
            manualTimeoutRef.current = setTimeout(() => {
              if (recognitionRef.current && !isContinuousListening) {
                console.log('Manual mode timeout - stopping recognition');
                try {
                  recognitionRef.current.stop();
                } catch (err) {
                  console.log('Error stopping recognition on timeout:', err);
                }
                setStatus('Timeout - click mic to try again');
              }
            }, 5000); // 5 second timeout for manual mode
          }
        };

        recognition.onresult = (event) => {
          // Clear manual timeout when we get a result
          if (manualTimeoutRef.current) {
            clearTimeout(manualTimeoutRef.current);
            manualTimeoutRef.current = null;
          }

          const results = Array.from(event.results[0]).map((result) => result.transcript.toLowerCase());
          const transcript = results[0];
          setRecognizedText(transcript);
          
          console.log('Recognized speech:', transcript);
          console.log('Speech confidence:', event.results[0][0].confidence);
          
          if (isContinuousListening) {
            // Filter out clearly irrelevant speech
            if (transcript.length > 100) {
              console.log('Transcript too long, ignoring:', transcript.length, 'characters');
              return;
            }
            
            // Skip if it contains words that indicate it's not a command
            const irrelevantWords = ['center', 'starts', 'banging', 'legs', 'conversation', 'talking'];
            const hasIrrelevantWords = irrelevantWords.some(word => transcript.includes(word));
            
            if (hasIrrelevantWords && !checkForTrigger(transcript, results)) {
              console.log('Transcript contains irrelevant words and no trigger, ignoring');
              return;
            }
            
            // Check for trigger phrase
            if (checkForTrigger(transcript, results)) {
              console.log('Trigger detected in transcript:', transcript);
              
              // ALWAYS wait for command if we only got the trigger
              const command = removeCommandFromTrigger(transcript);
              console.log('Extracted command after trigger removal:', command);
              
              if (command && command.trim().length > 2) { // Require at least 3 characters for a command
                // Command found in the same transcript - process immediately
                console.log('Processing command immediately:', command);
                setStatus('✅ Processing command...');
                processCommand(command, results);
                
                // Reset state after processing
                setWaitingForCommand(false);
                setTimeout(() => {
                  setStatus('Listening for "Hey Guitar"...');
                }, 2000);
              } else {
                // Always wait for the next speech input when trigger is detected
                console.log('Trigger detected but no command found, waiting for command...');
                setStatus('✅ Say your command now...');
                setWaitingForCommand(true);
                
                // Clear any existing timeout
                if (triggerTimeoutRef.current) {
                  clearTimeout(triggerTimeoutRef.current);
                }
                
                // Set longer timeout to give user time to say command
                console.log('Setting command timeout for 12 seconds...');
                triggerTimeoutRef.current = setTimeout(() => {
                  console.log('Command timeout - no command received after trigger');
                  setWaitingForCommand(false);
                  setStatus('Timeout - say "Hey Guitar" again');
                  
                  // Give a moment then reset to listening
                  setTimeout(() => {
                    setStatus('Listening for "Hey Guitar"...');
                  }, 2000);
                }, 12000); // Increased to 12 seconds
              }
              
            } else if (waitingForCommand) {
              // Process the command after trigger was detected in a previous transcript
              console.log('Processing follow-up command:', transcript);
              processCommand(transcript, results);
              
              // Reset state
              setWaitingForCommand(false);
              if (triggerTimeoutRef.current) {
                clearTimeout(triggerTimeoutRef.current);
              }
              
              // Brief pause before listening for trigger again
              setTimeout(() => {
                setStatus('Listening for "Hey Guitar"...');
              }, 2000);
            } else {
              // No trigger detected and not waiting for command
              console.log('No trigger detected, continuing to listen...');
            }
          } else {
            // Manual mode - process command directly
            console.log('Manual mode: Processing command and will stop listening');
            processCommand(transcript, results);
            
            // Set flag to indicate command was processed
            setManualCommandProcessed(true);
            
            // Stop recognition after processing command in manual mode
            if (recognitionRef.current) {
              try {
                recognitionRef.current.stop();
                console.log('Manual mode: Stopped recognition after processing command');
              } catch (err) {
                console.log('Error stopping recognition after manual command:', err);
              }
            }
          }
        };

        recognition.onend = () => {
          console.log('Voice Control: Recognition ended');
          setIsListening(false);
          
          // Clear manual timeout
          if (manualTimeoutRef.current) {
            clearTimeout(manualTimeoutRef.current);
            manualTimeoutRef.current = null;
          }

          // Handle manual mode end differently
          if (!isContinuousListening) {
            if (manualCommandProcessed) {
              setStatus('Click mic for next command');
              setManualCommandProcessed(false); // Reset the flag
            } else {
              setStatus('Ready');
            }
            return; // Don't restart in manual mode
          }

          // Don't restart if we're waiting for a command - let the user speak!
          if (waitingForCommand) {
            console.log('Voice Control: Not restarting - waiting for command');
            // Start recognition again immediately to capture the command
            setTimeout(() => {
              if (recognitionRef.current && isEnabled && isContinuousListening && waitingForCommand) {
                try {
                  recognitionRef.current.start();
                  console.log('Voice Control: Restarted to listen for command');
                } catch (err) {
                  console.log('Recognition restart error:', err);
                }
              }
            }, 100);
            return;
          }
          
          // Only restart if still enabled, in continuous mode, and not already restarting
          if (isContinuousListening && isEnabled && !isRestarting) {
            isRestarting = true;
            setTimeout(() => {
              if (recognitionRef.current && isEnabled && isContinuousListening) {
                try {
                  recognitionRef.current.start();
                  console.log('Voice Control: Restarted recognition for continuous mode');
                } catch (err) {
                  console.log('Recognition restart error:', err);
                }
              }
              isRestarting = false;
            }, 500);
          } else if (status === 'Listening...') {
            setStatus('Ready');
          }
        };

        recognition.onerror = (event) => {
          console.error('Speech recognition error:', event.error);
          setIsListening(false);
          
          // Clear manual timeout on error
          if (manualTimeoutRef.current) {
            clearTimeout(manualTimeoutRef.current);
            manualTimeoutRef.current = null;
          }
          
          // Don't restart on certain errors to prevent loops
          if (['not-allowed', 'service-not-allowed', 'bad-grammar'].includes(event.error)) {
            setStatus(`Error: ${event.error}`);
            setIsEnabled(false);
            return;
          }
          
          // For no-speech errors, be less aggressive with restarts
          if (event.error === 'no-speech') {
            if (isContinuousListening) {
              setStatus('No speech detected, listening...');
              // Only restart after a longer delay for no-speech errors
              setTimeout(() => {
                if (recognitionRef.current && isEnabled && isContinuousListening && !isRestarting) {
                  isRestarting = true;
                  try {
                    recognitionRef.current.start();
                  } catch (err) {
                    console.log('Recognition restart after no-speech error:', err);
                  }
                  isRestarting = false;
                }
              }, 3000); // Longer delay for no-speech
            }
            return;
          }
          
          setStatus(`Error: ${event.error}`);
          
          // Only restart for recoverable errors and if not already restarting
          if (isContinuousListening && 
              ['audio-capture', 'network'].includes(event.error) && 
              !isRestarting) {
            isRestarting = true;
            setTimeout(() => {
              if (recognitionRef.current && isEnabled && isContinuousListening) {
                try {
                  recognitionRef.current.start();
                } catch (err) {
                  console.log('Recognition restart after error:', err);
                }
              }
              isRestarting = false;
            }, 2000);
          }
        };

        recognitionRef.current = recognition;
        
        // Auto-start if continuous listening is enabled
        if (isContinuousListening && !isRestarting) {
          try {
            recognition.start();
            console.log('Voice Control: Auto-started continuous listening');
          } catch (err) {
            console.error('Auto-start failed:', err);
            setStatus(`Auto-start failed: ${err.message}`);
          }
        }
      } catch (error) {
        console.error('Failed to initialize recognition:', error);
        setStatus('Initialization failed');
      }
    };

    initializeRecognition();

    return () => {
      // Cleanup function
      if (recognitionRef.current) {
        try {
          recognitionRef.current.stop();
        } catch (err) {
          console.log('Cleanup error:', err);
        }
        recognitionRef.current = null;
      }
      if (triggerTimeoutRef.current) {
        clearTimeout(triggerTimeoutRef.current);
        triggerTimeoutRef.current = null;
      }
      // Clean up manual timeout
      if (manualTimeoutRef.current) {
        clearTimeout(manualTimeoutRef.current);
        manualTimeoutRef.current = null;
      }
      setIsListening(false);
      setManualCommandProcessed(false); // Reset flag on cleanup
    };
  }, [isEnabled, isContinuousListening]);

  // Check if the transcript contains the trigger phrase
  const checkForTrigger = (transcript, alternatives) => {
    console.log('Checking for trigger in:', transcript);
    
    const triggerPhrases = ['hey guitar', 'hey gitar', 'a guitar', 'hey guiter', 'guitar', 'ok guitar'];
    
    // Clean the transcript by removing punctuation and normalizing
    const cleanTranscript = transcript.replace(/[.,!?;:'"]/g, ' ')  // Replace punctuation with spaces
                                     .replace(/\s+/g, ' ')           // Collapse multiple spaces
                                     .trim()                         // Remove leading/trailing spaces
                                     .toLowerCase();
    
    console.log('Cleaned transcript for trigger check:', cleanTranscript);
    
    // Check main transcript
    for (const trigger of triggerPhrases) {
      if (cleanTranscript.includes(trigger)) {
        console.log('Found trigger "' + trigger + '" in cleaned transcript');
        return true;
      }
    }
    
    // Check alternatives with same cleaning
    for (const alt of alternatives) {
      const cleanAlt = alt.replace(/[.,!?;:'"]/g, ' ')
                         .replace(/\s+/g, ' ')
                         .trim()
                         .toLowerCase();
      
      for (const trigger of triggerPhrases) {
        if (cleanAlt.includes(trigger)) {
          console.log('Found trigger "' + trigger + '" in cleaned alternative:', cleanAlt);
          return true;
        }
      }
    }
    
    console.log('No trigger found in transcript or alternatives');
    return false;
  };

  // Remove trigger phrase from command
  const removeCommandFromTrigger = (transcript) => {
    console.log('Removing trigger from:', transcript);
    
    const triggerPhrases = ['hey guitar', 'hey gitar', 'a guitar', 'hey guiter', 'guitar', 'ok guitar'];
    
    // Clean the transcript first
    let cleanedTranscript = transcript.replace(/[.,!?;:'"]/g, ' ')  // Replace punctuation with spaces
                                     .replace(/\s+/g, ' ')           // Collapse multiple spaces
                                     .trim()                         // Remove leading/trailing spaces
                                     .toLowerCase();
    
    console.log('Cleaned transcript for trigger removal:', cleanedTranscript);
    
    let command = cleanedTranscript;
    for (const trigger of triggerPhrases) {
      if (command.includes(trigger)) {
        // Remove the trigger and any following comma/pause
        command = command.replace(trigger, '').replace(/^[,\s]+/, '').trim();
        console.log('After removing trigger "' + trigger + '":', command);
        break;
      }
    }
    
    // Final cleanup
    command = command.replace(/[.,!?;:]+$/, ''); // Remove trailing punctuation
    command = command.replace(/\s+/g, ' ');      // Normalize whitespace
    command = command.trim();                    // Final trim
    
    console.log('Final cleaned command:', command);
    return command;
  };

  // Process the actual command
  const processCommand = (command, alternatives) => {
    // Clean up the command text first
    const cleanCommand = command.replace(/[.,!?;:]+$/, '').trim();
    console.log('Processing command:', cleanCommand, '(original:', command + ')');
    
    // Enhanced command matching with context
    const matchedCommand = findBestMatch(cleanCommand, alternatives);
    
    if (matchedCommand) {
      // For playSpecificSong, ensure data is a string and not empty
      if (matchedCommand.action === 'playSpecificSong' && matchedCommand.data) {
        if (typeof matchedCommand.data === 'string' && matchedCommand.data.trim().length > 0) {
          console.log('Sending playSpecificSong with string data:', matchedCommand.data);
          onCommand(matchedCommand.action, matchedCommand.data.trim());
        } else {
          console.error('Invalid song name data:', matchedCommand.data, typeof matchedCommand.data);
          setStatus(`❌ Invalid song name`);
          return;
        }
      } else {
        console.log('Sending command:', matchedCommand.action, 'with data:', matchedCommand.data);
        onCommand(matchedCommand.action, matchedCommand.data);
      }
      setStatus(`✅ ${matchedCommand.action} executed`);
    } else {
      setStatus(`❌ Command not recognized: "${cleanCommand}"`);
    }
  };

  // Enhanced command matching function
  const findBestMatch = (command, alternatives) => {
    console.log('Finding best match for command:', command);
    
    // CHECK FOR SONG COMMANDS FIRST - before checking exact matches
    if (command.includes('play')) {
      // Handle "by artist" patterns first
      if (command.includes(' by ')) {
        const songName = extractSongName(command);
        if (songName && typeof songName === 'string') {
          console.log('Detected play song command with name and artist:', songName);
          return { action: 'playSpecificSong', data: songName };
        }
        
        // If no song name, try artist extraction
        const artist = extractArtistName(command);
        if (artist && typeof artist === 'string') {
          console.log('Detected play by artist command:', artist);
          return { action: 'playByArtist', data: artist };
        }
      } else {
        // Simple play command - check if there's something after "play"
        const songName = extractSongName(command);
        if (songName && typeof songName === 'string') {
          console.log('Detected simple play song command:', songName);
          return { action: 'playSpecificSong', data: songName };
        }
      }
    }

    // NOW check for exact command matches (only if no song was found)
    for (const [key, action] of Object.entries(commandMap)) {
      // Skip 'play' if we already checked for songs above
      if (key === 'play' && command.includes('play') && command.trim() !== 'play') {
        continue; // Skip exact 'play' match if there are additional words
      }
      
      if (command.includes(key)) {
        console.log('Found exact match:', key, '->', action);
        return { action, data: null };
      }
    }

    // Try alternative results
    for (const alt of alternatives) {
      // Check for song commands in alternatives first
      if (alt.includes('play')) {
        const songName = extractSongName(alt);
        if (songName && typeof songName === 'string') {
          console.log('Found song command in alternatives:', songName);
          return { action: 'playSpecificSong', data: songName };
        }
      }
      
      // Then check for exact matches in alternatives
      for (const [key, action] of Object.entries(commandMap)) {
        if (key === 'play' && alt.includes('play') && alt.trim() !== 'play') {
          continue; // Skip exact 'play' match if there are additional words
        }
        
        if (alt.includes(key)) {
          console.log('Found match in alternatives:', key, '->', action);
          return { action, data: null };
        }
      }
    }

    console.log('No match found for command:', command);
    return null;
  };

  // Extract song name from voice command
  const extractSongName = (command) => {
    console.log('Extracting song name from command:', command);
    
    // Extract song name from commands like "play the entertainer", "play song name", etc.
    const patterns = [
      /play song (.+)/i,
      /play track (.+)/i,
      /play the (.+?) by/i,     // "play the [song] by [artist]" - capture until "by"
      /play (.+?) by/i,         // "play [song] by [artist]" - capture until "by"
      /play the (.+)/i,         // "play the [song]" - capture everything after "the"
      /play (.+)/i              // "play [song]" - capture everything after "play"
    ];
    
    for (const pattern of patterns) {
      const match = command.match(pattern);
      if (match && match[1]) {
        let songName = String(match[1]).trim(); // Ensure it's a string
        
        // Clean up common punctuation and artifacts from speech recognition
        songName = songName.replace(/[.,!?;:]+$/, ''); // Remove trailing punctuation
        songName = songName.replace(/\s+/g, ' ');      // Normalize whitespace
        songName = songName.trim();                    // Final trim
        
        console.log('Pattern matched:', pattern, 'Extracted:', songName);
        
        // Filter out common command words that aren't song names
        const excludeWords = ['music', 'something', 'anything', 'song', 'track'];
        if (!excludeWords.includes(songName.toLowerCase()) && songName.length > 0) {
          console.log('Final extracted song name:', songName);
          return songName;
        }
      }
    }
    
    console.log('No song name extracted from command:', command);
    return null;
  };

  // Extract artist name from voice command
  const extractArtistName = (command) => {
    // Extract artist name from commands like "play something by [artist]"
    const match = command.match(/by (.+)/);
    return match && match[1] ? String(match[1]).trim() : null;
  };

  // Handle manual listen button click
  const handleButtonClick = async () => {
    console.log('Voice Control: Button clicked, isListening:', isListening, 'isEnabled:', isEnabled);
    
    if (isListening) {
      console.log('Voice Control: Stopping recognition');
      // Clear manual timeout when stopping manually
      if (manualTimeoutRef.current) {
        clearTimeout(manualTimeoutRef.current);
        manualTimeoutRef.current = null;
      }
      setManualCommandProcessed(false); // Reset flag when manually stopping
      recognitionRef.current?.stop();
    } else {
      console.log('Voice Control: Starting recognition');
      
      // Reset the command processed flag when starting new session
      setManualCommandProcessed(false);
      
      // Check permissions first
      const hasPermissions = await checkPermissions();
      if (!hasPermissions) {
        console.log('Voice Control: No permissions');
        return;
      }
      
      try {
        if (recognitionRef.current) {
          recognitionRef.current.start();
          console.log('Voice Control: Recognition started successfully');
        } else {
          console.error('Voice Control: Recognition not initialized');
          setStatus('Recognition not initialized');
        }
      } catch (err) {
        console.error('Voice Control: Error starting recognition:', err);
        setStatus(`Error: ${err.message}`);
      }
    }
  };

  // Toggle voice control on/off
  const toggleVoiceControl = () => {
    setIsEnabled(!isEnabled);
    if (isListening) {
      recognitionRef.current?.stop();
    }
    setStatus(isEnabled ? 'Disabled' : 'Ready');
  };

  // Toggle continuous listening mode
  const toggleContinuousListening = () => {
    setIsContinuousListening(!isContinuousListening);
    setWaitingForCommand(false);
    setManualCommandProcessed(false); // Reset flag when switching modes
    
    if (triggerTimeoutRef.current) {
      clearTimeout(triggerTimeoutRef.current);
    }
    
    // Clear manual timeout when switching modes
    if (manualTimeoutRef.current) {
      clearTimeout(manualTimeoutRef.current);
      manualTimeoutRef.current = null;
    }
    
    if (isListening) {
      recognitionRef.current?.stop();
    }
    
    setStatus('Ready');
  };

  // Get CSS class for status display
  const getStatusClass = () => {
    if (status.includes('✅')) return 'status-success';
    if (status.includes('❌')) return 'status-error';
    if (status === 'Listening...' || status.includes('Listening for')) return 'status-listening';
    if (status === 'Click mic for next command') return 'status-ready';
    if (waitingForCommand) return 'status-waiting';
    return 'status-normal';
  };

  // Compact mode for header
  if (compact) {
    return (
      <div className="voice-control-compact">
        <div className="voice-control-icons">
          <button 
            onClick={toggleVoiceControl}
            className={`voice-icon-button ${isEnabled ? 'enabled' : 'disabled'}`}
            title={isEnabled ? 'Voice Control Enabled' : 'Voice Control Disabled'}
          >
            <FaMicrophone className={isEnabled ? 'icon-enabled' : 'icon-disabled'} />
          </button>
          
          <button 
            onClick={toggleContinuousListening}
            className={`trigger-icon-button ${isContinuousListening ? 'active' : ''}`}
            disabled={!isEnabled}
            title={isContinuousListening ? 'Always Listening' : 'Manual Mode'}
          >
            🎤
          </button>
          
          {/* Manual listen button for when not in continuous mode */}
          {!isContinuousListening && isEnabled && (
            <button 
              onClick={handleButtonClick}
              className={`manual-listen-button ${isListening ? 'listening' : ''}`}
              title={isListening ? 'Stop Listening' : 'Start Listening'}
            >
              {isListening ? <FaMicrophoneSlash /> : <FaMicrophone />}
            </button>
          )}
          
          {/* Status indicator */}
          {isEnabled && (
            <div className="status-indicator-compact">
              <span className={`status-dot ${getStatusClass()}`}></span>
              {waitingForCommand && (
                <div className="command-prompt">
                  <span className="waiting-indicator-compact">⏳</span>
                  <span className="prompt-text">Say command now!</span>
                </div>
              )}
            </div>
          )}
        </div>
        
        {/* Expandable details on hover */}
        <div className="voice-control-tooltip">
          <div className="tooltip-content">
            <p><strong>Voice Control</strong></p>
            <p>Status: {status}</p>
            {recognizedText && (
              <p>Last: "{recognizedText}"</p>
            )}
            <p>Say "Hey Guitar" → [pause] → command</p>
          </div>
        </div>
      </div>
    );
  }

  // Regular full mode
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
        
        <button 
          onClick={toggleContinuousListening}
          className={`trigger-mode-button ${isContinuousListening ? 'active' : ''}`}
          disabled={!isEnabled}
        >
          {isContinuousListening ? '🎤 Always Listening' : '🎤 Manual Mode'}
        </button>
      </div>

      <div className="voice-status">
        <p className="status-text">
          Status: <span className={getStatusClass()}>{status}</span>
          {waitingForCommand && <span className="waiting-indicator"> ⏳</span>}
        </p>
        {recognizedText && (
          <p className="recognized-text">Last heard: "{recognizedText}"</p>
        )}
      </div>

      {!isContinuousListening && (
        <button 
          onClick={handleButtonClick}
          disabled={!isEnabled}
          className={`voice-button ${isListening ? 'listening' : ''}`}
        >
          {isListening ? <FaMicrophoneSlash /> : <FaMicrophone />}
          {isListening ? 'Stop Listening' : 'Start Listening'}
        </button>
      )}

      <div className="voice-commands-help">
        <details>
          <summary>Available Commands</summary>
          <div className="trigger-info">
            <p><strong>Continuous Mode:</strong> Say "Hey Guitar" then your command</p>
            <p><strong>Manual Mode:</strong> Click microphone, then say direct commands</p>
            <p><strong>Examples:</strong></p>
            <ul>
              <li>Continuous: "Hey Guitar, play the entertainer"</li>
              <li>Manual: "play the entertainer" (after clicking mic)</li>
            </ul>
            <p><strong>Tip:</strong> Speak slowly and clearly. You have 12 seconds to give your command after "Hey Guitar".</p>
          </div>
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
                <li>"clear playlist"</li>
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
};

export default VoiceControl;