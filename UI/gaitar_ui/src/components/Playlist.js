// src/components/Playlist.js
import React from 'react';

const Playlist = ({ currentPlaylist, selectPlaylist }) => {
  return (
    <div>
      <h3>Current Playlist</h3> {/* Added title */}
      <ul>
        {currentPlaylist.map((track, index) => (
          <li key={index} onClick={() => selectPlaylist(track)}>
            {track}
          </li>
        ))}
      </ul>
    </div>
  );
};

export default Playlist;
