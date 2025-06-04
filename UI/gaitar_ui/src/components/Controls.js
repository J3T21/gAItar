// src/components/Controls.js
import React, { forwardRef, useImperativeHandle } from 'react';
import { FaPlay, FaPause, FaStepForward, FaStepBackward, FaRandom } from 'react-icons/fa';
import { esp32 } from '../api';

const Controls = forwardRef(({ 
  currentTrack, 
  setCurrentTrack, 
  currentPlaylist, 
  setCurrentPlaylist, 
  setIsPlaying 
}, ref) => {
  
  // Enhanced helper function to sanitize track data for ESP32
  const sanitizeTrackForESP32 = (track) => {
    // Function to clean and sanitize individual fields
    const cleanAndSanitize = (str) => {
      if (!str) return '';
      
      // Remove all types of whitespace and control characters from start and end
      const cleaned = str.replace(/[\r\n\t\f\v]/g, ' ') // Replace control chars with space
                        .trim() // Remove leading/trailing whitespace
                        .replace(/\s+/g, '_') // Replace remaining spaces with underscores
                        .replace(/_+$/, '') // Remove trailing underscores
                        .replace(/^_+/, ''); // Remove leading underscores
      
      return cleaned;
    };

    return {
      ...track,
      title: cleanAndSanitize(track.title),
      artist: cleanAndSanitize(track.artist),
      genre: cleanAndSanitize(track.genre)
    };
  };

  const handlePlay = () => {
    if (currentTrack) {
      console.log('Play:', currentTrack.title);
      const sanitizedTrack = sanitizeTrackForESP32(currentTrack);
      console.log('Sending sanitized track to ESP32:', sanitizedTrack);
      
      esp32.post('/play', sanitizedTrack)
        .then(response => {
          console.log('Playing track on ESP32:', response.data);
          setIsPlaying(true); // Set to playing
        })
        .catch(error => console.error('Error playing track on ESP32:', error));
    } else {
      console.log('No track selected to play');
    }
  };

  const handlePause = () => {
    if (currentTrack) {
      console.log('Pause:', currentTrack.title);
      esp32.post('/pause')
        .then(response => {
          console.log('Paused on ESP32:', response.data);
          setIsPlaying(false); // Set to paused
        })
        .catch(error => console.error('Error pausing on ESP32:', error));
    } else {
      console.log('No track selected to pause');
    }
  };

  const handleSkip = () => {
    if (currentPlaylist.length > 0) {
      const currentIndex = currentPlaylist.findIndex(track => track.title === currentTrack?.title);
      const nextIndex = (currentIndex + 1) % currentPlaylist.length;
      const nextTrack = currentPlaylist[nextIndex];
      setCurrentTrack(nextTrack);
      console.log('Skipped to:', nextTrack.title);
      
      const sanitizedTrack = sanitizeTrackForESP32(nextTrack);
      console.log('Sending sanitized track to ESP32:', sanitizedTrack);
      
      esp32.post('/play', sanitizedTrack)
        .then(response => {
          console.log('Playing next track on ESP32:', response.data);
          setIsPlaying(true); // Set to playing
        })
        .catch(error => console.error('Error skipping track on ESP32:', error));
    } else {
      console.log('No tracks in the playlist to skip');
    }
  };

  const handlePrevious = () => {
    if (currentPlaylist.length > 0) {
      const currentIndex = currentPlaylist.findIndex(track => track.title === currentTrack?.title);
      const prevIndex = currentIndex === 0 ? currentPlaylist.length - 1 : currentIndex - 1;
      const prevTrack = currentPlaylist[prevIndex];
      setCurrentTrack(prevTrack);
      console.log('Previous track:', prevTrack.title);
      
      const sanitizedTrack = sanitizeTrackForESP32(prevTrack);
      console.log('Sending sanitized track to ESP32:', sanitizedTrack);
      
      esp32.post('/play', sanitizedTrack)
        .then(response => {
          console.log('Playing previous track on ESP32:', response.data);
          setIsPlaying(true); // Set to playing
        })
        .catch(error => console.error('Error playing previous track on ESP32:', error));
    } else {
      console.log('No tracks in the playlist to go back to');
    }
  };

  const handleShuffle = () => {
    if (currentPlaylist.length > 0) {
      // Shuffle the playlist
      const shuffledPlaylist = [...currentPlaylist].sort(() => Math.random() - 0.5);

      // Update the playlist in the App component
      setCurrentPlaylist(shuffledPlaylist);

      // Select the first track from the shuffled playlist
      const nextTrack = shuffledPlaylist[0];
      setCurrentTrack(nextTrack);
      console.log('Shuffled to:', nextTrack.title);

      const sanitizedTrack = sanitizeTrackForESP32(nextTrack);
      console.log('Sending sanitized track to ESP32:', sanitizedTrack);

      esp32.post('/play', sanitizedTrack)
        .then(response => {
          console.log('Playing shuffled track on ESP32:', response.data);
          setIsPlaying(true); // Set to playing
        })
        .catch(error => console.error('Error shuffling playlist on ESP32:', error));
    } else {
      console.log('No tracks in the playlist to shuffle');
      alert('No tracks in the playlist to shuffle.'); // Raise an error to the user
    }
  };

  // New function to play a specific track
  const handlePlaySpecific = (track) => {
    if (track) {
      console.log('Controls: Playing specific track via voice:', track.title);
      const sanitizedTrack = sanitizeTrackForESP32(track);
      console.log('Controls: Sending specific track to ESP32:', sanitizedTrack);
      
      esp32.post('/play', sanitizedTrack)
        .then(response => {
          console.log('Controls: Successfully started playing specific track on ESP32:', response.data);
          setIsPlaying(true);
        })
        .catch(error => {
          console.error('Controls: Error playing specific track on ESP32:', error);
          setIsPlaying(false);
        });
    } else {
      console.log('Controls: No specific track provided');
    }
  };

  // Expose methods to parent component via ref
  useImperativeHandle(ref, () => ({
    handlePlay: () => {
      console.log('Controls: handlePlay called via voice control for current track:', currentTrack?.title);
      handlePlay();
    },
    handlePlaySpecific: (track) => {
      console.log('Controls: handlePlaySpecific called via voice control for track:', track?.title);
      handlePlaySpecific(track);
    },
    handlePause: () => {
      console.log('Controls: handlePause called via voice control');
      handlePause();
    },
    handleSkip: () => {
      console.log('Controls: handleSkip called via voice control');
      handleSkip();
    },
    handlePrevious: () => {
      console.log('Controls: handlePrevious called via voice control');
      handlePrevious();
    }
  }));
  
  return (
    <div className="controls">
      <button onClick={handleShuffle} className="control-button">
        <FaRandom />
      </button>
      <button onClick={handlePrevious} className="control-button">
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
  );
});

export default Controls;
