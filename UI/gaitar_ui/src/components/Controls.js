// src/components/Controls.js
import React from 'react';
import { FaPlay, FaPause, FaStepForward, FaStepBackward, FaRandom } from 'react-icons/fa';
import { esp32 } from '../api';

const Controls = ({ currentTrack, setCurrentTrack, currentPlaylist, setIsPlaying, setCurrentPlaylist }) => {
  
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
};

export default Controls;
