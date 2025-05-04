// src/components/Genre.js
import React, { useState } from 'react';

const Genre = ({ genres, addGenre, selectPlaylist }) => {
  const [newGenre, setNewGenre] = useState('');

  const handleAddGenre = () => {
    if (newGenre) {
      addGenre(newGenre);
      setNewGenre('');
    }
  };

  return (
    <div>
      <h3>Genres</h3>
      <ul>
        {genres.map((genre, index) => (
          <li key={index}>
            <button onClick={() => selectPlaylist(genre.playlists)}>
              {genre.name}
            </button>
          </li>
        ))}
      </ul>
      <input 
        type="text" 
        value={newGenre} 
        onChange={(e) => setNewGenre(e.target.value)} 
        placeholder="Add New Genre" 
      />
      <button onClick={handleAddGenre}>Add Genre</button>
    </div>
  );
};

export default Genre;
