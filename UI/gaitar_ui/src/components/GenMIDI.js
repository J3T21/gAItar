import { backend_api, esp32 } from '../api'; // Import the API instance
import React, { useState } from 'react';
import { Midi } from '@tonejs/midi';
import * as Tone from 'tone';
import { FaMusic, FaUpload } from 'react-icons/fa'; // Add FaUpload import

export default function MidiGenerator({ onUploadToForm }) { // Add prop for upload callback
  const [prompt, setPrompt] = useState('');
  const [loading, setLoading] = useState(false);
  const [midiUrl, setMidiUrl] = useState(null);
  const [midiBlob, setMidiBlob] = useState(null); // Add midiBlob state for upload
  const [error, setError] = useState(null);

  const handleGenerate = async () => {
    setLoading(true);
    setError(null);
    setMidiUrl(null);
    setMidiBlob(null); // Clear previous blob

    try {
      const response = await backend_api.post(
        '/generate-midi/', // adjust this to match your backend
        { prompt },
        { responseType: 'blob' }
      );

      const blob = new Blob([response.data], { type: 'audio/midi' });
      const url = URL.createObjectURL(blob);
      setMidiBlob(blob); // Store blob for upload
      setMidiUrl(url);
    } catch (err) {
      setError('Failed to generate MIDI.');
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  // Convert MIDI blob to File and send to Upload component
  const uploadMidiToForm = () => {
    if (!midiBlob) {
      setError('No MIDI file to upload');
      return;
    }

    try {
      // Convert blob to File object
      const midiFile = new File([midiBlob], 'generated-midi.mid', { type: 'audio/midi' });
      
      // Call the callback function to send file to Upload component
      if (onUploadToForm) {
        onUploadToForm(midiFile);
      }
    } catch (err) {
      console.error('Error uploading MIDI to form:', err);
      setError('Failed to upload MIDI to form');
    }
  };

  const playMidi = async () => {
    if (!midiUrl) return;

    const response = await fetch(midiUrl);
    const arrayBuffer = await response.arrayBuffer();
    const midi = new Midi(arrayBuffer);

    await Tone.start(); // Required on some browsers

    const now = Tone.now();

    // Use a PolySynth to handle overlapping notes
    const polySynth = new Tone.PolySynth(Tone.Synth).toDestination();

    // Filter out non-melodic tracks (e.g., percussion on channel 10)
    const melodicTracks = midi.tracks.filter((track) => {
      return track.notes.length > 0 && track.channel !== 9; // Channel 10 is 0-indexed as 9
    });

    melodicTracks.forEach((track) => {
      track.notes.forEach((note) => {
        // Schedule each note to play at its specified time
        polySynth.triggerAttackRelease(note.name, note.duration, now + note.time);
      });
    });
  };

  return (
    <div className="midi-generator">
      <h2>Generate MIDI from Prompt</h2>
      <textarea
        className="midi-prompt-textarea"
        rows="6"
        placeholder="Enter a detailed prompt for MIDI generation... (e.g., 'A peaceful guitar melody in C major with a slow tempo')"
        value={prompt}
        onChange={(e) => setPrompt(e.target.value)}
      />
      <button
        onClick={handleGenerate}
        disabled={loading}
        className="midi-generate-button"
      >
        {loading ? 'Generating...' : 'Generate MIDI'}
      </button>

      {midiUrl && (
        <div className="midi-controls">
          <button
            onClick={playMidi}
            className="midi-play-button"
          >
            <FaMusic />
            Play MIDI
          </button>
          
          <button
            onClick={uploadMidiToForm}
            className="upload-midi-button"
            disabled={!midiBlob}
          >
            <FaUpload />
            Upload to Form
          </button>
        </div>
      )}

      {error && <p className="midi-error">{error}</p>}
    </div>
  );
}
