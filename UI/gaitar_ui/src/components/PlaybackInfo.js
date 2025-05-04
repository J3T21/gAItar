import React, { useState, useEffect } from 'react';

const PlaybackInfo = ({ currentTrack }) => {
  const [progress, setProgress] = useState(0); // current playback time
  const [isScrubbing, setIsScrubbing] = useState(false);
  const [scrubTime, setScrubTime] = useState(0);
  const duration = 180; // example: 3-minute track (static for now)

  useEffect(() => {
    if (!currentTrack) return;

    const interval = setInterval(() => {
      if (!isScrubbing) {
        setProgress(prev => {
          if (prev >= duration) {
            clearInterval(interval);
            return duration;
          }
          return prev + 1;
        });
      }
    }, 1000);

    return () => clearInterval(interval);
  }, [currentTrack, isScrubbing]);

  const handleScrubStart = () => setIsScrubbing(true);
  const handleScrubEnd = () => {
    setIsScrubbing(false);
    setProgress(scrubTime); // Update playback time when scrubbing ends
  };
  const handleScrub = (e) => {
    const newTime = Math.min(Math.max(e.nativeEvent.offsetX / e.target.offsetWidth * duration, 0), duration);
    setScrubTime(newTime);
  };

  const formatTime = (seconds) => {
    const min = Math.floor(seconds / 60);
    const sec = String(seconds % 60).padStart(2, '0');
    return `${min}:${sec}`;
  };

  const progressPercent = (progress / duration) * 100;
  const scrubPercent = (scrubTime / duration) * 100;

  if (!currentTrack) return null;

  return (
    <div className="playback-info">
      <h3>{currentTrack}</h3>
      <div
        className="progress-bar"
        onMouseDown={handleScrubStart}
        onMouseUp={handleScrubEnd}
        onMouseMove={isScrubbing ? handleScrub : null}
      >
        <div className="progress-bar-filled" style={{ width: `${progressPercent}%` }} />
        <div className="scrubber" style={{ left: `${scrubPercent}%` }} />
      </div>
      <div className="time-info">
        {formatTime(progress)} / {formatTime(duration)}
      </div>
    </div>
  );
};

export default PlaybackInfo;
