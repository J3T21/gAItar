# gAItar
Final Year Masters Electronic and Information Engineering project: A mechatronic interface to play an acoustic guitar with some AI elements.

[![gAItar Demo Video](https://img.youtube.com/vi/pzkTf3Y7SAQ/0.jpg)](https://youtu.be/pzkTf3Y7SAQ)

## User Guide

### Introduction
Welcome to gAItar! This guide will help you understand how to set up and use the gAItar system.

### Setup Instructions

#### Software Setup

##### Backend API (`gAItar_api`)
1.  Navigate to the `gAItar_api/backend` directory in your terminal.
2.  It's recommended to create and activate a Python virtual environment:
    *   `python -m venv venv`
    *   On Windows: `venv\Scripts\activate`
    *   On macOS/Linux: `source venv/bin/activate`
3.  Install Python dependencies: `pip install -r requirements.txt` (Ensure you have a `requirements.txt` file in this directory listing libraries like `fastapi`, `uvicorn`, `torch`, `huggingface_hub`, `python-multipart`, `mido`, `miditok`, `basic-pitch`, `tensorflow`, `pretty_midi`).
4.  Run the FastAPI server: `uvicorn main:app --reload`
    *   The API will be available at `http://localhost:8000`.

##### User Interface (`UI/gaitar_ui`)
1.  Navigate to the `UI/gaitar_ui` directory in your terminal.
2.  Install Node.js dependencies: `npm install`
3.  Start the development server: `npm start`
    *   The UI will be accessible at `http://localhost:3000`.
4.  **Important:** Verify and update the API base URLs in `UI/gaitar_ui/src/api.js`:
    *   `backend_api`: Should point to your backend API (e.g., `http://localhost:8000`).
    *   `esp32`: Should point to your ESP32's IP address (e.g., `http://10.245.188.200`).

##### ESP32 Firmware (`gAItar_esp32`)
1.  Open the `gAItar_esp32` project folder in VS Code with PlatformIO extension.
2.  In `gAItar_esp32/src/main.cpp`:
    *   Update the `ssid` and `password` variables with your Wi-Fi network credentials. The current configuration is set for an Android mobile hotspot. Ensure this network is the same one your computer (running the UI and backend) is connected to.
    *   The ESP32 attempts to configure a static IP address (e.g., `10.245.188.200` as seen in the code). The actual IP address will be displayed in the Serial Monitor upon successful connection. Ensure this IP matches the `baseURL` for `esp32` in `UI/gaitar_ui/src/api.js`.
3.  Build and upload the firmware to the ESP32 board using PlatformIO.

##### Adafruit Grand Central Firmware (`gAItar_arduino`)
1.  Open the `gAItar_arduino` project folder in VS Code with PlatformIO extension.
2.  The firmware should generally not require changes for basic operation. Pin assignments for solenoids and servos are defined in `gAItar_arduino/src/globals.cpp`. Ensure these match your physical wiring.
3.  Build and upload the firmware to the Adafruit Grand Central board using PlatformIO.

### Using the gAItar Interface

#### Main Interface Overview
*   **Search & Playlist:**
    *   Use the search bar to find songs by title.
    *   Filter songs by artist or genre using the dropdowns.
    *   Click "Play" to play a song immediately or "Add to Playlist" to add it to the current queue.
    *   Manage the current playlist (e.g., reorder, remove songs).
*   **Playback Controls:**
    *   Use the Play, Pause, Skip Forward, Skip Backward, and Shuffle buttons to control playback on the physical guitar.
*   **MIDI Generation (`GenMIDI`):**
    *   Enter a text prompt describing the music you want to generate (e.g., "a sad blues riff in E minor").
    *   Click "Generate MIDI". The generated MIDI can be played in the browser or uploaded to the song upload form.
*   **Song Upload (`Upload`):**
    *   Select a MIDI file (`.mid` or `.midi`).
    *   Enter the Title, Artist, and Genre for the song.
    *   You can add new artists or genres if they are not in the existing dropdowns.
    *   Click "Upload". This will send the MIDI to the backend for processing and then transfer the processed data to the ESP32.
*   **Voice to MIDI (`VoiceToMIDI`):**
    *   Record audio using your microphone.
    *   Convert the recorded audio to a MIDI file.
    *   The generated MIDI can be played, downloaded, or sent to the Upload form.
*   **Voice Control:**
    *   Use voice commands to control playback (e.g., "Play song [Title]", "Pause", "Next song"). Refer to the UI for available commands and status.

### Troubleshooting
*   **Cannot connect to ESP32:**
    *   Verify the ESP32's IP address in `UI/gaitar_ui/src/api.js` matches the IP shown in the ESP32's serial output.
    *   Ensure the ESP32 is connected to the same Wi-Fi network as the computer running the UI.
    *   Check the ESP32 serial monitor for error messages (e.g., Wi-Fi connection issues, SPIFFS errors).
*   **MIDI generation fails:**
    *   Ensure the backend API (`gAItar_api`) is running and accessible at `http://localhost:8000`.
    *   Check the backend console for error messages (e.g., model loading issues, Hugging Face Hub connection).
*   **Song upload fails:**
    *   Ensure both the backend API and ESP32 are operational and connected.
    *   Check the browser console, backend console, and ESP32 serial monitor for specific error messages.
    *   Verify SPIFFS on the ESP32 is working correctly (it's formatted on ESP32 startup).
*   **Playback issues on the guitar:**
    *   Check wiring to solenoids and servos.
    *   Verify pin configurations in `gAItar_arduino/src/globals.cpp`.
    *   Monitor serial output from the Adafruit Grand Central for any errors during playback.
