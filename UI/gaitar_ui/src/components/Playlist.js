// src/components/Playlist.js
import React from 'react';

const Playlist = ({ currentPlaylist, selectPlaylist }) => {
  return (
    <div>
      <h3>Playlist</h3>
      <ul>
        {currentPlaylist.map((track, index) => (
          <li key={index}>
            {track}
          </li>
        ))}
      </ul>
    </div>
  );
};

export default Playlist;
