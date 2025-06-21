import React, { useState, useEffect, useRef } from 'react';

const PlaybackInfo = ({ currentTrack }) => {
  const [progress, setProgress] = useState(0); // current playback time from ESP32 in milliseconds
  const [totalDuration, setTotalDuration] = useState(180000); // total track duration from ESP32 in milliseconds
  const [isPlaying, setIsPlaying] = useState(false); // calculated from time changes
  const [isPaused, setIsPaused] = useState(false); // calculated from time stagnation
  const [songName, setSongName] = useState(''); // current song name from ESP32
  const [wsConnected, setWsConnected] = useState(false);
  
  const wsRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);
  const reconnectAttemptsRef = useRef(0);
  const lastProgressRef = useRef(0);
  const progressCheckTimeoutRef = useRef(null);
  const lastUpdateTimeRef = useRef(0);
  const UPDATE_THROTTLE = 1000; // Only update every 1 second

  // WebSocket connection for real-time playback updates
  useEffect(() => {
    const connectWebSocket = () => {
      try {
        if (wsRef.current) {
          wsRef.current.close();
        }

        console.log('PlaybackInfo: Connecting to WebSocket...');
        wsRef.current = new WebSocket('ws://192.168.113.200/ws');
        
        wsRef.current.onopen = () => {
          console.log('PlaybackInfo: WebSocket connected');
          setWsConnected(true);
          reconnectAttemptsRef.current = 0;
          
          if (reconnectTimeoutRef.current) {
            clearTimeout(reconnectTimeoutRef.current);
            reconnectTimeoutRef.current = null;
          }
        };
        
        wsRef.current.onmessage = (event) => {
          try {
            const data = JSON.parse(event.data);
            
            // Handle playback status updates
            if (data.type === 'playback_status') {
              // Throttle updates to reduce frequency
              const now = Date.now();
              if (now - lastUpdateTimeRef.current < UPDATE_THROTTLE) {
                return; // Skip this update
              }
              lastUpdateTimeRef.current = now;
              
              // Reduce console logging
              // console.log('PlaybackInfo: Received playback status:', data); // Comment out or remove
              
              // Update playback time (keep in milliseconds)
              if (data.currentTime !== undefined) {
                const newProgress = data.currentTime; // Keep as milliseconds
                setProgress(newProgress);
                
                // Calculate playing state based on time progression
                const wasPlaying = newProgress > lastProgressRef.current;
                const isFinished = data.totalTime && newProgress >= data.totalTime;
                
                if (isFinished) {
                  setIsPlaying(false);
                  setIsPaused(false);
                } else if (wasPlaying) {
                  setIsPlaying(true);
                  setIsPaused(false);
                } else {
                  // Check for pause state after a delay
                  if (progressCheckTimeoutRef.current) {
                    clearTimeout(progressCheckTimeoutRef.current);
                  }
                  
                  progressCheckTimeoutRef.current = setTimeout(() => {
                    // If time hasn't changed after 2 seconds and we're not at the end, consider it paused
                    if (newProgress > 0 && !isFinished) {
                      setIsPlaying(false);
                      setIsPaused(true);
                    }
                  }, 2000);
                }
                
                lastProgressRef.current = newProgress;
              }
              
              // Update total duration (keep in milliseconds)
              if (data.totalTime !== undefined) {
                setTotalDuration(data.totalTime);
              }
              
              // Handle raw data fallback
              if (data.rawData) {
                console.log('PlaybackInfo: Raw playback data:', data.rawData);
              }
            }
          } catch (err) {
            // Reduce console logging
            // console.log('PlaybackInfo: Non-JSON WebSocket message:', event.data); // Comment out
          }
        };
        
        wsRef.current.onclose = (event) => {
          console.log('PlaybackInfo: WebSocket disconnected');
          setWsConnected(false);
          
          // Attempt to reconnect
          reconnectAttemptsRef.current++;
          const delay = Math.min(1000 * Math.pow(2, Math.min(reconnectAttemptsRef.current - 1, 5)), 30000);
          
          reconnectTimeoutRef.current = setTimeout(() => {
            connectWebSocket();
          }, delay);
        };
        
        wsRef.current.onerror = (error) => {
          console.error('PlaybackInfo: WebSocket error:', error);
          setWsConnected(false);
        };

      } catch (err) {
        console.error('PlaybackInfo: Failed to create WebSocket connection:', err);
        setWsConnected(false);
        
        // Retry connection
        reconnectAttemptsRef.current++;
        const delay = 3000;
        reconnectTimeoutRef.current = setTimeout(connectWebSocket, delay);
      }
    };

    connectWebSocket();

    // Cleanup on unmount
    return () => {
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
      if (progressCheckTimeoutRef.current) {
        clearTimeout(progressCheckTimeoutRef.current);
      }
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, []);

  // Reset progress when track changes
  useEffect(() => {
    if (currentTrack) {
      setProgress(0);
      setIsPlaying(false);
      setIsPaused(false);
      lastProgressRef.current = 0;
    }
  }, [currentTrack]);

  // Convert milliseconds to MM:SS format (matches the script.js formatTime function)
  const formatTime = (milliseconds) => {
    const totalSeconds = Math.floor(milliseconds / 1000);
    const minutes = Math.floor(totalSeconds / 60);
    const seconds = totalSeconds % 60;
    return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
  };

  const progressPercent = totalDuration > 0 ? (progress / totalDuration) * 100 : 0;

  // Show playback info only if we have a current track
  if (!currentTrack) return null;

  return (
    <div className="playback-info">
      {/* Progress bar (visual only, no scrubbing) */}
      <div className="progress-bar">
        <div className="progress-bar-filled" style={{ width: `${progressPercent}%` }} />
      </div>
      
      {/* Time information */}
      <div className="time-info">
        <span className="current-time">{formatTime(progress)}</span>
        <span className="total-time">{formatTime(totalDuration)}</span>
      </div>
    </div>
  );
};

export default PlaybackInfo;