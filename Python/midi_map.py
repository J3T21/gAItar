import mido
from mido import MidiFile, MidiTrack, Message
import py_midicsv as pm
import pandas as pd
from collections import defaultdict
import numpy as np
import pandas as pd
import pygame
import pygame.midi
import time
import rtmidi
from time import sleep
import math


import sounddevice as sd

# Guitar string base frequencies (in Hz for open strings)
STRING_FREQUENCIES = {
    6: 82.41,  # Low E string (82.41 Hz)
    5: 110.00,  # A string (110.00 Hz)
    4: 146.83,  # D string (146.83 Hz)
    3: 196.00,  # G string (196.00 Hz)
    2: 246.94,  # B string (246.94 Hz)
    1: 329.63   # High E string (329.63 Hz)
}
STRING_NOTE_MAP = {
    6: 40,  # Low E string (MIDI note 40)
    5: 45,  # A string
    4: 50,  # D string
    3: 55,  # G string
    2: 59,  # B string
    1: 64   # High E string
}

def create_midi_from_guitar(commands, output_file):
    # Create a new MIDI file and a track
    midi = MidiFile()
    track = MidiTrack()
    midi.tracks.append(track)

    last_time = 0  # Initial time reference for timing of the events

    for command in commands:
        event_time, string, fret_number = command

        # Convert string and fret number to MIDI note
        base_note = STRING_NOTE_MAP[string]
        midi_note = base_note + int(fret_number)  # Ensure fret_number is an integer

        # Calculate the time difference from the last event (in ticks)
        time_diff = (event_time - last_time)  # Assuming event_time is in milliseconds
        ticks = int(time_diff)  # Convert ms to ticks

        # Ensure ticks is an integer (in case of float precision issues)
        ticks = int(ticks)

        # Add 'note_on' and 'note_off' events for each pluck
        track.append(Message('note_on', note=int(midi_note), velocity=64, time=ticks))
        track.append(Message('note_off', note=int(midi_note), velocity=64, time=10))  # 480 ticks duration for the note

        # Update the last time
        last_time = event_time

    # Save the MIDI file
    midi.save(output_file)
    print(f'MIDI file saved as {output_file}')

def process_midi_to_guitar(file_path):
    """Processes a MIDI CSV file, applies tempo & time signature changes, and maps MIDI notes to guitar plucks."""
    
    # Load MIDI CSV file as a DataFrame
    midi_data = pd.read_csv(file_path, on_bad_lines='skip', header=None)

    # Guitar string ranges (open string note values)
    STRING_RANGES = {
        6: (40, 48),  # Low E
        5: (45, 53),  # A
        4: (50, 58),  # D
        3: (55, 63),  # G
        2: (59, 67),  # B
        1: (64, 72)   # High E
    }

    # Default MIDI settings
    PPQ = 480  # Ticks per quarter note (Pulse Per Quarter)
    DEFAULT_TEMPO = 500000  # Microseconds per quarter note (120 BPM)
    current_tempo = DEFAULT_TEMPO
    beats_per_measure = 4  # Default time signature (4/4)
    note_value_per_beat = 4  # Quarter note is the beat (denominator in time signature)

    absolute_time = 0
    last_tick = 0
    active_notes = {}
    string_usage = {s: None for s in STRING_RANGES}
    commands = []
    tempo_changes = []
    time_signatures = []

    # #step 0: remove non melody channels
    # for _, row in midi_data.iterrows():
    #     drop_channel = None
    #     if row[2] == "Program_c" and int(row[4]) > 111:
    #         drop_channel = int(row[3])
    #     midi_data = midi_data[midi_data[3] != drop_channel]


    # ** Step 1: Sort MIDI events by time (ticks) **
    midi_data = midi_data.sort_values(by=[1])  # Column 1 is 'ticks'

    # ** Step 2: Process MIDI events in order **
    for _, row in midi_data.iterrows():
        if len(row) < 3:
            continue  # Skip malformed rows

        track = row[0]
        ticks = row[1]  # MIDI tick timestamp
        event = row[2]  # Event type

        # ** Convert Ticks to Absolute Time **
        tick_delta = ticks - last_tick
        absolute_time += (tick_delta / PPQ) * (current_tempo / 1000)  # Convert µs to ms
        last_tick = ticks

        # ** Handle Tempo Changes **
        if "Tempo" in str(event):
            new_tempo = int(row[3])  # Read new tempo value
            tempo_changes.append((absolute_time, new_tempo))  # Store tempo change event
            current_tempo = new_tempo  # Apply new tempo
            continue  # Move to next event

        # ** Handle Time Signature Changes **
        if "Time_signature" in str(event):
            beats_per_measure = int(row[3])  # Numerator (number of beats per measure)
            note_value_per_beat = 2 ** int(row[4])  # Denominator (power of 2)
            time_signatures.append((absolute_time, beats_per_measure, note_value_per_beat))
            continue  # Move to next event

        # ** Process Note Events (On/Off) **
        if len(row) < 6:
            continue  # Skip if not a valid Note event

        channel = row[3]
        note = row[4]
        velocity = row[5]

        try:
            note = int(note)
        except ValueError:
            continue  # Ignore invalid notes

        # ** Handle Note-On Event **
        if "Note_on_c" in str(event) and velocity > 0:
            if note in active_notes:
                continue  # Skip if note is already playing

            # Assign note to a guitar string
            available_string = None
            for s in sorted(STRING_RANGES.keys(), reverse=True):  # Prefer higher strings first
                min_note, max_note = STRING_RANGES[s]
                if string_usage[s] is None and min_note <= note <= max_note:
                    available_string = s
                    break

            if available_string:
                open_note = STRING_RANGES[available_string][0]  # Open string MIDI note
                fret_number = note - open_note  # Calculate fret number
                commands.append((absolute_time, available_string, fret_number))  # Store pluck event
                active_notes[note] = available_string
                string_usage[available_string] = note  # Mark string as in use

        # ** Handle Note-Off Event **
        elif "Note_off_c" in str(event) or ("Note_on_c" in str(event) and velocity == 0):
            if note in active_notes:
                assigned_string = active_notes[note]
                string_usage[assigned_string] = None  # Free the string
                del active_notes[note]

    # ** Step 3: Sort and return processed guitar events **
    commands.sort()
    return commands

def print_midi_file(infile,outfile=None):
    '''few notes about midi:
    - midi files are composed of tracks
    - each track is composed of messages
    - each message is composed of a type and a value
    - the type can be note_on, note_off, control_change, etc
    - time is mesured in pulses per quarter note (ppqn)'''
    csv_string_list = pm.midi_to_csv(infile)
    with open(infile + ".csv", "w+") as f:
     f.writelines(csv_string_list)

    # Parse the CSV output of the previous command back into a MIDI file
    midi_object = pm.csv_to_midi(csv_string_list)
    if outfile:
        with open(outfile, "wb") as output_file:
            midi_writer = pm.FileWriter(output_file)
            midi_writer.write(midi_object)

if __name__ == '__main__':
    print_midi_file("midi_tracks\Beatles_White_Album__Blackbird.mid", "midi_tracks\procesed.mid")
    create_midi_from_guitar(process_midi_to_guitar('midi_tracks\Beatles_White_Album__Blackbird.mid.csv'), 'output_guitar_simulation_bb.mid')
    #imulate_guitar_playing(process_midi_to_guitar('midi_tracks\Tarrega_Recuerdos_de_la_Alhambra.mid.csv'))


# Function to generate a sine wave for a given frequency, duration, and sample rate
def generate_sine_wave(frequency, duration, sample_rate=44100, amplitude=0.5):
    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    waveform = amplitude * np.sin(2 * np.pi * frequency * t)
    return waveform

# Function to simulate guitar playing with synthesized sound
def simulate_guitar_playing(commands):
    last_time = 0  # Initial time reference
    for command in commands:
        event_time, string, fret_number = command
        
        # Sleep to simulate the passage of time (event_time is in milliseconds)
        if last_time != 0:
            time_diff = (event_time - last_time) / 10000 # Convert from ms to seconds
            #ime.sleep(time_diff)
        
        # Calculate the frequency for the string and fret
        base_freq = STRING_FREQUENCIES[string]
        note_frequency = base_freq * (2 ** (fret_number / 12))  # Calculate frequency for the given fret
        
        # Generate a sine wave for the note
        duration = 0.3  # Duration of the note in seconds (simulate pluck)
        waveform = generate_sine_wave(note_frequency, duration)
        
        # Play the generated waveform
        #(f"Plucking string {string} and pressing fret {fret_number}.")
        sd.play(waveform, samplerate=44100)  # Play the waveform through sounddevice
        sd.wait()  # Wait for the note to finish before continuing
        
        # Update the last time
        last_time = event_time