// src/components/Controls.js
import React from 'react';
import { esp32 } from '../api';

const Controls = ({ currentTrack, setCurrentTrack, currentPlaylist }) => {
      const handlePlay = () => {
        console.log('Play:', currentTrack);
        esp32.post('/play',  currentTrack )
          .then(response => console.log('Playing track on ESP32:', response.data))
          .catch(error => console.error('Error playing track on ESP32:', error));
      };
    
      const handlePause = () => {
        console.log('Pause');
        esp32.post('/pause')
          .then(response => console.log('Paused on ESP32:', response.data))
          .catch(error => console.error('Error pausing on ESP32:', error));
      };
    
      const handleSkip = () => {
        const nextTrackIndex = (currentPlaylist.indexOf(currentTrack) + 1) % currentPlaylist.length;
        const nextTrack = currentPlaylist[nextTrackIndex];
        setCurrentTrack(nextTrack);
        esp32.post('/skip', { nextTrack })
          .then(response => console.log('Skipped to track on ESP32:', response.data))
          .catch(error => console.error('Error skipping track on ESP32:', error));
      };
    
      const handleShuffle = () => {
        const shuffledPlaylist = [...currentPlaylist].sort(() => Math.random() - 0.5);
        esp32.post('/shuffle', { shuffledPlaylist })
          .then(response => console.log('Shuffled playlist on ESP32:', response.data))
          .catch(error => console.error('Error shuffling playlist on ESP32:', error));
      };
    
      return (
        <div className="controls">
          <button onClick={handlePlay}/*disabled={!currentTrack}*/>Play</button>
          <button onClick={handlePause}>Pause</button>
          <button onClick={handleSkip}/*disabled={currentPlaylist.length <= 1}*/>Skip</button>
          <button onClick={handleShuffle}>Shuffle</button>
        </div>
      );
    };

export default Controls;
