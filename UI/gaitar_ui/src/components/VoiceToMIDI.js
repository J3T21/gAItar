// gaitar_ui/src/components/VoiceToMIDI.js
import React, { useState, useRef, forwardRef, useImperativeHandle } from 'react';
import { FaMicrophone, FaStop, FaPlay, FaDownload, FaMusic, FaUpload } from 'react-icons/fa';
import { backend_api } from '../api';
import { Midi } from '@tonejs/midi';
import * as Tone from 'tone';

  const VoiceToMIDI = forwardRef(({ onUploadToForm }, ref) => {
  const [isRecording, setIsRecording] = useState(false);
  const [audioBlob, setAudioBlob] = useState(null);
  const [audioUrl, setAudioUrl] = useState(null);
  const [midiUrl, setMidiUrl] = useState(null);
  const [midiBlob, setMidiBlob] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [recordingTime, setRecordingTime] = useState(0);
  const [isPlayingMidi, setIsPlayingMidi] = useState(false);
  const [uploadMessage, setUploadMessage] = useState(''); // Add upload message state
  
  const mediaRecorderRef = useRef(null);
  const audioChunksRef = useRef([]);
  const timerRef = useRef(null);
  const synthRef = useRef(null);

  // Check if browser supports audio recording
  const isRecordingSupported = navigator.mediaDevices && navigator.mediaDevices.getUserMedia;

  // Start recording
  const startRecording = async () => {
    if (!isRecordingSupported) {
      setError('Audio recording is not supported in your browser');
      return;
    }

    try {
      setError(null);
      setAudioBlob(null);
      setAudioUrl(null);
      setMidiUrl(null);
      setMidiBlob(null);
      setUploadMessage(''); // Clear upload message
      audioChunksRef.current = [];

      const stream = await navigator.mediaDevices.getUserMedia({ 
        audio: {
          echoCancellation: true,
          noiseSuppression: true,
          sampleRate: 44100
        } 
      });

      // Try WAV first, fall back to WebM
      let mimeType = 'audio/wav';
      if (!MediaRecorder.isTypeSupported(mimeType)) {
        mimeType = 'audio/webm;codecs=opus';
      }

      const mediaRecorder = new MediaRecorder(stream, { mimeType });
      mediaRecorderRef.current = mediaRecorder;

      mediaRecorder.ondataavailable = (event) => {
        if (event.data.size > 0) {
          audioChunksRef.current.push(event.data);
        }
      };

      mediaRecorder.onstop = () => {
        const audioBlob = new Blob(audioChunksRef.current, { type: mimeType });
        setAudioBlob(audioBlob);
        setAudioUrl(URL.createObjectURL(audioBlob));
        
        // Stop all tracks to release microphone
        stream.getTracks().forEach(track => track.stop());
      };

      mediaRecorder.start();
      setIsRecording(true);
      setRecordingTime(0);

      // Start timer
      timerRef.current = setInterval(() => {
        setRecordingTime(prev => prev + 1);
      }, 1000);

    } catch (err) {
      console.error('Error accessing microphone:', err);
      setError('Could not access microphone. Please check permissions.');
    }
  };

  // Stop recording
  const stopRecording = () => {
    if (mediaRecorderRef.current && isRecording) {
      mediaRecorderRef.current.stop();
      setIsRecording(false);
      
      if (timerRef.current) {
        clearInterval(timerRef.current);
        timerRef.current = null;
      }
    }
  };

  // Convert recorded audio to WAV format
  const convertToWAV = async (audioBlob) => {
    try {
      // If it's already WAV, return as is
      if (audioBlob.type === 'audio/wav') {
        return new File([audioBlob], 'recording.wav', { type: 'audio/wav' });
      }

      // Convert using Web Audio API
      const audioContext = new (window.AudioContext || window.webkitAudioContext)();
      const arrayBuffer = await audioBlob.arrayBuffer();
      const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);
      
      // Convert to WAV
      const wavBuffer = audioBufferToWav(audioBuffer);
      return new File([wavBuffer], 'recording.wav', { type: 'audio/wav' });
    } catch (error) {
      console.error('Error converting to WAV:', error);
      // Fallback: send original file with .wav extension
      return new File([audioBlob], 'recording.wav', { type: 'audio/wav' });
    }
  };

  // Convert AudioBuffer to WAV format
  const audioBufferToWav = (buffer) => {
    const length = buffer.length;
    const numberOfChannels = Math.min(buffer.numberOfChannels, 2); // Limit to stereo
    const sampleRate = buffer.sampleRate;
    const bitsPerSample = 16;
    const bytesPerSample = bitsPerSample / 8;
    const blockAlign = numberOfChannels * bytesPerSample;
    const byteRate = sampleRate * blockAlign;
    const dataSize = length * blockAlign;
    const bufferLength = 44 + dataSize;
    
    const arrayBuffer = new ArrayBuffer(bufferLength);
    const view = new DataView(arrayBuffer);
    
    // WAV header
    const writeString = (offset, string) => {
      for (let i = 0; i < string.length; i++) {
        view.setUint8(offset + i, string.charCodeAt(i));
      }
    };
    
    writeString(0, 'RIFF');
    view.setUint32(4, bufferLength - 8, true);
    writeString(8, 'WAVE');
    writeString(12, 'fmt ');
    view.setUint32(16, 16, true);
    view.setUint16(20, 1, true);
    view.setUint16(22, numberOfChannels, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, byteRate, true);
    view.setUint16(32, blockAlign, true);
    view.setUint16(34, bitsPerSample, true);
    writeString(36, 'data');
    view.setUint32(40, dataSize, true);
    
    // Convert audio data
    let offset = 44;
    for (let i = 0; i < length; i++) {
      for (let channel = 0; channel < numberOfChannels; channel++) {
        const channelData = buffer.getChannelData(channel);
        const sample = Math.max(-1, Math.min(1, channelData[i]));
        view.setInt16(offset, sample * 0x7FFF, true);
        offset += 2;
      }
    }
    
    return arrayBuffer;
  };

  // Convert audio and send to MIDI
  const convertToMIDI = async () => {
    if (!audioBlob) {
      setError('No audio recorded');
      return;
    }

    setLoading(true);
    setError(null);
    setMidiUrl(null);
    setMidiBlob(null);
    setUploadMessage(''); // Clear upload message

    try {
      // Convert to WAV format
      const wavFile = await convertToWAV(audioBlob);
      
      // Create FormData for the backend
      const formData = new FormData();
      formData.append('audio_file', wavFile, 'recording.wav');

      console.log('Sending WAV file to backend:', wavFile.type, wavFile.size, 'bytes');

      // Send to backend for MIDI conversion
      const response = await backend_api.post('/convert-audio-to-midi/', formData, {
        headers: {
          'Content-Type': 'multipart/form-data',
        },
        responseType: 'blob',
        timeout: 60000 // 60 second timeout
      });

      // Create MIDI blob and URL for download and playback
      const midiBlob = new Blob([response.data], { type: 'audio/midi' });
      const midiUrl = URL.createObjectURL(midiBlob);
      setMidiBlob(midiBlob);
      setMidiUrl(midiUrl);

    } catch (err) {
      console.error('Error converting to MIDI:', err);
      if (err.code === 'ECONNABORTED') {
        setError('Conversion timed out. Try a shorter recording.');
      } else if (err.response?.status === 400) {
        setError('Audio format not supported. Please try recording again.');
      } else {
        setError('Failed to convert audio to MIDI. Please try again with a clearer recording.');
      }
    } finally {
      setLoading(false);
    }
  };

  // Play MIDI using Tone.js
  const playMidi = async () => {
    if (!midiBlob) {
      setError('No MIDI file to play');
      return;
    }

    try {
      setIsPlayingMidi(true);
      setError(null);

      // Convert blob to array buffer
      const arrayBuffer = await midiBlob.arrayBuffer();
      const midi = new Midi(arrayBuffer);

      // Start Tone.js (required on some browsers)
      await Tone.start();

      // Stop any existing synth
      if (synthRef.current) {
        synthRef.current.dispose();
      }

      // Create a new PolySynth for playing multiple notes
      const polySynth = new Tone.PolySynth(Tone.Synth).toDestination();
      synthRef.current = polySynth;

      const now = Tone.now();

      // Filter out non-melodic tracks (e.g., percussion on channel 10)
      const melodicTracks = midi.tracks.filter((track) => {
        return track.notes.length > 0 && track.channel !== 9; // Channel 10 is 0-indexed as 9
      });

      if (melodicTracks.length === 0) {
        setError('No melodic content found in MIDI file');
        setIsPlayingMidi(false);
        return;
      }

      // Schedule all notes to play
      let maxDuration = 0;
      melodicTracks.forEach((track) => {
        track.notes.forEach((note) => {
          // Schedule each note to play at its specified time
          polySynth.triggerAttackRelease(
            note.name, 
            note.duration, 
            now + note.time
          );
          // Track the maximum duration for auto-stop
          maxDuration = Math.max(maxDuration, note.time + note.duration);
        });
      });

      // Auto-stop playing state after MIDI finishes
      setTimeout(() => {
        setIsPlayingMidi(false);
        if (synthRef.current) {
          synthRef.current.dispose();
          synthRef.current = null;
        }
      }, (maxDuration + 1) * 1000); // Add 1 second buffer

      console.log(`Playing MIDI with ${melodicTracks.length} tracks for ${maxDuration.toFixed(2)} seconds`);

    } catch (err) {
      console.error('Error playing MIDI:', err);
      setError('Failed to play MIDI file');
      setIsPlayingMidi(false);
    }
  };

  // Stop MIDI playback
  const stopMidi = () => {
    if (synthRef.current) {
      synthRef.current.dispose();
      synthRef.current = null;
    }
    setIsPlayingMidi(false);
  };

  // Convert MIDI blob to File and send to Upload component
  const uploadMidiToForm = () => {
    if (!midiBlob) {
      setError('No MIDI file to upload');
      return;
    }

    try {
      // Convert blob to File object
      const midiFile = new File([midiBlob], 'voice-recording.mid', { type: 'audio/midi' });
      
      // Call the callback function to send file to Upload component
      if (onUploadToForm) {
        onUploadToForm(midiFile);
      }
    } catch (err) {
      console.error('Error uploading MIDI to form:', err);
      setError('Failed to upload MIDI to form');
    }
  };

  // Format recording time
  const formatTime = (seconds) => {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins}:${secs.toString().padStart(2, '0')}`;
  };

  // Play recorded audio
  const playRecording = () => {
    if (audioUrl) {
      const audio = new Audio(audioUrl);
      audio.play().catch(err => {
        console.error('Error playing audio:', err);
        setError('Could not play the recording. Please try recording again.');
      });
    }
  };

  // Expose methods to parent component
  useImperativeHandle(ref, () => ({
    startRecording,
    stopRecording,
    convertToMIDI
  }));

  return (
    <div className="voice-to-midi">
      <h3>Voice to MIDI</h3>
      
      {!isRecordingSupported && (
        <div className="error-message">
          Audio recording is not supported in your browser
        </div>
      )}

      {error && (
        <div className="error-message">
          {error}
        </div>
      )}

      <div className="recording-controls">
        {!isRecording ? (
          <button 
            onClick={startRecording} 
            className="record-button"
            disabled={!isRecordingSupported}
          >
            <FaMicrophone />
            Start Recording
          </button>
        ) : (
          <div className="recording-active">
            <button onClick={stopRecording} className="stop-button">
              <FaStop />
              Stop Recording
            </button>
            <div className="recording-timer">
              {formatTime(recordingTime)}
            </div>
          </div>
        )}
      </div>

      {audioUrl && (
        <div className="audio-controls">
          <div className="recorded-audio">
            <p>Recording complete! ({formatTime(recordingTime)})</p>
            <button onClick={playRecording} className="voice-play-button">
              <FaPlay />
              Play Recording
            </button>
          </div>

          <button 
            onClick={convertToMIDI} 
            className="convert-button"
            disabled={loading}
          >
            {loading ? 'Converting...' : 'Convert to MIDI'}
          </button>
        </div>
      )}

      {midiUrl && (
        <div className="midi-result">
          <p>MIDI conversion successful!</p>
          
          <div className="midi-playback-controls">
            <button 
              onClick={isPlayingMidi ? stopMidi : playMidi}
              className="midi-play-button"
              disabled={!midiBlob}
            >
              <FaMusic />
              {isPlayingMidi ? 'Stop MIDI' : 'Play MIDI'}
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
        </div>
      )}
    </div>
  );
});

export default VoiceToMIDI;