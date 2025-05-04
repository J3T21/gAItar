import './App.css';
import React, { useState } from 'react';
import Playlist from './components/Playlist';
import Upload from './components/Upload';  // Upload component now handles genres too
import Controls from './components/Controls';
import PlaybackInfo from './components/PlaybackInfo';
import VoiceControl from './components/VoiceControl';


const App = () => {
  const [genres, setGenres] = useState([
    { name: 'Pop', playlists: ['Track 1', 'Track 2', 'Track 3'] },
    { name: 'Rock', playlists: ['Track 4', 'Track 5', 'Track 6'] }
  ]);

  const [currentPlaylist, setCurrentPlaylist] = useState([]);
  const [currentTrack, setCurrentTrack] = useState(null);

  const addGenre = (genreName) => {
    setGenres([...genres, { name: genreName, playlists: [] }]);
  };

  const [artists, setArtists] = useState([
    { name: 'Artist 1' },
    { name: 'Artist 2' },
  ]);
  
  const addArtist = (newArtist) => {
    setArtists([...artists, { name: newArtist }]);
  };

  const addTrackToPlaylist = (track, genreName) => {
    const updatedGenres = genres.map(genre => {
      if (genre.name === genreName) {
        genre.playlists.push(track);
      }
      return genre;
    });
    setGenres(updatedGenres);
  };

  const selectPlaylist = (playlist) => {
    setCurrentPlaylist(playlist);
    setCurrentTrack(playlist[0]);
  };

  const [trackMetadata, setTrackMetadata] = useState({
    title: '',
    artist: '',
    genre: '',
    duration_formatted: ''
  });



  // Handle voice command from VoiceControl
  // const handleVoiceCommand = (command) => {
  //   console.log('Voice command received:', command);
  //   switch (command) {
  //     case 'play':
  //       handlePlay();
  //       break;
  //     case 'pause':
  //       handlePause();
  //       break;
  //     case 'shuffle':
  //       handleShuffle();
  //       break;
  //     default:
  //       console.log('Command not recognized:', command);
  //       break;
  //   }
  // };

  return (
    <div>
      <h1>Music Player</h1>

      {/* Upload component now handles both file upload and genre management */}
      <Upload
        genres={genres}  // Make sure this is an array and correctly passed
        artists={artists}  // Make sure this is an array and correctly passed
        setTrackMetadata={setTrackMetadata}
        onUpload={addTrackToPlaylist}
        addArtist={addArtist}
      />


      {/* Playlist and Controls */}
      <Playlist currentPlaylist={currentPlaylist} selectPlaylist={selectPlaylist} />
      <Controls
        currentTrack={currentTrack}
        setCurrentTrack={setCurrentTrack}
        currentPlaylist={currentPlaylist}
      />
      <PlaybackInfo currentTrack={currentTrack} />
      {/*<VoiceControl onCommand={handleVoiceCommand} />*/}
    </div>
  );
};

export default App;
