import React, { useState, useEffect, useRef } from 'react';

// Command map to actions
const commandMap = {
  play: 'play',
  pause: 'pause',
  shuffle: 'shuffle',
  skip: 'skip',
};

const VoiceControl = ({ onCommand }) => {
  const [isListening, setIsListening] = useState(false);
  const [recognizedText, setRecognizedText] = useState('');
  const [status, setStatus] = useState('Ready');
  
  // Create a ref to hold the recognition instance
  const recognitionRef = useRef(null);

  useEffect(() => {
    // Check if SpeechRecognition is supported
    if (!('webkitSpeechRecognition' in window)) {
      alert('Your browser does not support Speech Recognition');
      return;
    }

    const recognition = new window.webkitSpeechRecognition();
    recognition.continuous = false;
    recognition.interimResults = false;
    recognition.lang = 'en-US';

    recognition.onstart = () => {
      setIsListening(true);
      setStatus('Listening...');
    };

    recognition.onresult = (event) => {
      const command = event.results[0][0].transcript.toLowerCase();
      setRecognizedText(command);
      console.log('Recognized command:', command);
      const matchedCommand = Object.keys(commandMap).find(cmd => command.includes(cmd));
      if (matchedCommand) {
        onCommand(commandMap[matchedCommand]);
        setStatus(`Command: ${matchedCommand} recognized.`);
      } else {
        setStatus(`Command not recognized: ${command}`);
      }
    };

    recognition.onend = () => {
      setIsListening(false);
      setStatus('Ready');
    };

    recognition.onerror = (event) => {
      setIsListening(false);
      setStatus('Error occurred');
      console.error(event.error);
    };

    // Store the recognition instance in the ref
    recognitionRef.current = recognition;

    // Cleanup function when component unmounts
    return () => {
      if (recognitionRef.current && recognitionRef.current.stop) {
        recognitionRef.current.stop();
      }
    };
  }, [onCommand]);

  const handleButtonClick = () => {
    if (isListening) {
      recognitionRef.current.stop(); // Stop listening
    } else {
      recognitionRef.current.start(); // Start listening
    }
    setIsListening(!isListening);
  };

  return (
    <div>
      <h3>Voice Control</h3>
      <p>Status: {status}</p>
      <p>Recognized Command: {recognizedText}</p>
      <button onClick={handleButtonClick}>
        {isListening ? 'Stop Listening' : 'Start Listening'}
      </button>
    </div>
  );
};

export default VoiceControl;
