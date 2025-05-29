// src/components/Playlist.js
import React, { useState } from 'react';
import { FaTimes, FaGripVertical } from 'react-icons/fa';

const Playlist = ({ currentPlaylist, setCurrentPlaylist, selectPlaylist }) => {
  const [draggedIndex, setDraggedIndex] = useState(null);
  const [dragOverIndex, setDragOverIndex] = useState(null);

  const handleDragStart = (e, index) => {
    setDraggedIndex(index);
    e.dataTransfer.effectAllowed = 'move';
  };

  const handleDragOver = (e, index) => {
    e.preventDefault();
    setDragOverIndex(index);
    e.dataTransfer.dropEffect = 'move';
  };

  const handleDragLeave = () => {
    setDragOverIndex(null);
  };

  const handleDrop = (e, dropIndex) => {
    e.preventDefault();
    
    if (draggedIndex === null || draggedIndex === dropIndex) {
      setDraggedIndex(null);
      setDragOverIndex(null);
      return;
    }

    const newPlaylist = [...currentPlaylist];
    const draggedItem = newPlaylist[draggedIndex];
    
    // Remove the dragged item
    newPlaylist.splice(draggedIndex, 1);
    
    // Insert at new position
    newPlaylist.splice(dropIndex, 0, draggedItem);
    
    setCurrentPlaylist(newPlaylist);
    setDraggedIndex(null);
    setDragOverIndex(null);
  };

  const handleDragEnd = () => {
    setDraggedIndex(null);
    setDragOverIndex(null);
  };

  const removeTrack = (indexToRemove) => {
    const newPlaylist = currentPlaylist.filter((_, index) => index !== indexToRemove);
    setCurrentPlaylist(newPlaylist);
  };

  const handleTrackClick = (track) => {
    if (selectPlaylist) {
      selectPlaylist(track);
    }
  };

  return (
    <div>
      <h3>Current Playlist</h3>
      <ul className="playlist-items">
        {currentPlaylist.map((track, index) => (
          <li
            key={`${track.title}-${index}`}
            className={`playlist-item ${draggedIndex === index ? 'dragging' : ''} ${dragOverIndex === index ? 'drag-over' : ''}`}
            draggable
            onDragStart={(e) => handleDragStart(e, index)}
            onDragOver={(e) => handleDragOver(e, index)}
            onDragLeave={handleDragLeave}
            onDrop={(e) => handleDrop(e, index)}
            onDragEnd={handleDragEnd}
          >
            <div className="playlist-item-content">
              <FaGripVertical className="drag-handle" />
              <div 
                className="track-info"
                onClick={() => handleTrackClick(track)}
              >
                <span className="track-title">{track.title}</span>
                <span className="track-details">by {track.artist} ({track.genre})</span>
              </div>
              <button
                className="remove-button"
                onClick={(e) => {
                  e.stopPropagation();
                  removeTrack(index);
                }}
                title="Remove from playlist"
              >
                <FaTimes />
              </button>
            </div>
          </li>
        ))}
      </ul>
      {currentPlaylist.length === 0 && (
        <p className="empty-playlist">No tracks in playlist</p>
      )}
    </div>
  );
};

export default Playlist;
