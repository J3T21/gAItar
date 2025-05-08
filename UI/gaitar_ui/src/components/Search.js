import React, { useState } from 'react';

const Search = ({ songs, onPlay, onAddToPlaylist }) => {
  const [query, setQuery] = useState('');
  const [selectedArtist, setSelectedArtist] = useState('');
  const [selectedGenre, setSelectedGenre] = useState('');
  const [results, setResults] = useState([]);
  const [suggestions, setSuggestions] = useState([]);

  const uniqueArtists = [...new Set(songs.map(song => song.artist))];
  const uniqueGenres = [...new Set(songs.map(song => song.genre))];

  const handleSearch = () => {
    const filteredSongs = songs.filter(song => {
      const matchesQuery = song.title.toLowerCase().includes(query.toLowerCase());
      const matchesArtist = selectedArtist ? song.artist === selectedArtist : true;
      const matchesGenre = selectedGenre ? song.genre === selectedGenre : true;
      return matchesQuery && matchesArtist && matchesGenre;
    });
    setResults(filteredSongs);
  };

  const handleInputChange = (e) => {
    const input = e.target.value;
    setQuery(input);

    if (input) {
      const matchingSuggestions = songs
        .filter(song => song.title.toLowerCase().startsWith(input.toLowerCase()))
        .map(song => song.title);
      setSuggestions(matchingSuggestions);
    } else {
      setSuggestions([]);
    }
  };

  const handleSuggestionClick = (suggestion) => {
    setQuery(suggestion);
    setSuggestions([]);
  };

  return (
    <div className="search-container">
      <input
        type="text"
        placeholder="Search for a song..."
        value={query}
        onChange={handleInputChange}
        className="search-input"
      />
      {suggestions.length > 0 && (
        <ul className="suggestions-list">
          {suggestions.map((suggestion, index) => (
            <li
              key={index}
              onClick={() => handleSuggestionClick(suggestion)}
              className="suggestion-item"
            >
              {suggestion}
            </li>
          ))}
        </ul>
      )}
      <select
        value={selectedArtist}
        onChange={(e) => setSelectedArtist(e.target.value)}
        className="artist-filter"
      >
        <option value="">All Artists</option>
        {uniqueArtists.map((artist, index) => (
          <option key={index} value={artist}>
            {artist}
          </option>
        ))}
      </select>
      <select
        value={selectedGenre}
        onChange={(e) => setSelectedGenre(e.target.value)}
        className="genre-filter"
      >
        <option value="">All Genres</option>
        {uniqueGenres.map((genre, index) => (
          <option key={index} value={genre}>
            {genre}
          </option>
        ))}
      </select>
      <button onClick={handleSearch} className="search-button">Search</button>
      <ul className="search-results">
        {results.map((song, index) => (
          <li key={index} className="search-result-item">
            <span>{song.title} by {song.artist}</span>
            <button onClick={() => onPlay(song)} className="play-button">Play</button>
            <button onClick={() => onAddToPlaylist(song)} className="add-button">Add to Playlist</button>
          </li>
        ))}
      </ul>
    </div>
  );
};

export default Search;