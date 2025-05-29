import { backend_api, esp32 } from '../api'; // Import the API instance
import React, { useState } from 'react';
import { Midi } from '@tonejs/midi';
import * as Tone from 'tone';

export default function MidiGenerator() {
  const [prompt, setPrompt] = useState('');
  const [loading, setLoading] = useState(false);
  const [midiUrl, setMidiUrl] = useState(null);
  const [error, setError] = useState(null);

  const handleGenerate = async () => {
    setLoading(true);
    setError(null);
    setMidiUrl(null);

    try {
      const response = await backend_api.post(
        '/generate-midi/', // adjust this to match your backend
        { prompt },
        { responseType: 'blob' }
      );

      const blob = new Blob([response.data], { type: 'audio/midi' });
      const url = URL.createObjectURL(blob);
      setMidiUrl(url);
    } catch (err) {
      setError('Failed to generate MIDI.');
      console.error(err);
    } finally {
      setLoading(false);
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
            Play MIDI
          </button>
          <a
            href={midiUrl}
            download="generated.mid"
            className="midi-download-link"
          >
            Download MIDI
          </a>
        </div>
      )}

      {error && <p className="midi-error">{error}</p>}
    </div>
  );
}
