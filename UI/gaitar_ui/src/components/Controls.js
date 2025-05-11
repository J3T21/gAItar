// src/components/Controls.js
import React from 'react';
import { FaPlay, FaPause, FaStepForward, FaStepBackward, FaRandom } from 'react-icons/fa';
import { esp32 } from '../api';

const Controls = ({ currentTrack, setCurrentTrack, currentPlaylist, setIsPlaying }) => {
  const handlePlay = () => {
    if (currentTrack) {
      console.log('Play:', currentTrack.title);
      esp32.post('/play', currentTrack)
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
      esp32.post('/play', nextTrack)
        .then(response => {
          console.log('Playing next track on ESP32:', response.data);
          setIsPlaying(true); // Set to playing
        })
        .catch(error => console.error('Error skipping track on ESP32:', error));
    } else {
      console.log('No tracks in the playlist to skip');
    }
  };

  const handleShuffle = () => {
    if (currentPlaylist.length > 0) {
      const shuffledPlaylist = [...currentPlaylist].sort(() => Math.random() - 0.5);
      const nextTrack = shuffledPlaylist[0];
      setCurrentTrack(nextTrack);
      console.log('Shuffled to:', nextTrack.title);
      esp32.post('/play', nextTrack)
        .then(response => {
          console.log('Playing shuffled track on ESP32:', response.data);
          setIsPlaying(true); // Set to playing
        })
        .catch(error => console.error('Error shuffling playlist on ESP32:', error));
    } else {
      console.log('No tracks in the playlist to shuffle');
    }
  };

  return (
    <div className="controls">
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
  );
};

export default Controls;
