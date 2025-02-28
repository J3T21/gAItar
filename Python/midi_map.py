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
        ticks = int(time_diff * midi.ticks_per_beat / 1000)  # Convert ms to ticks

        # Ensure ticks is an integer (in case of float precision issues)
        ticks = int(ticks)

        # Add 'note_on' and 'note_off' events for each pluck
        track.append(Message('note_on', note=int(midi_note), velocity=64, time=ticks))
        track.append(Message('note_off', note=int(midi_note), velocity=64, time=480))  # 480 ticks duration for the note

        # Update the last time
        last_time = event_time

    # Save the MIDI file
    midi.save(output_file)
    print(f'MIDI file saved as {output_file}')

def process_midi_to_guitar(file_path):
    """Processes a MIDI CSV file and maps it to guitar plucks while ensuring note preservation and fret limits."""
    midi_data = pd.read_csv(file_path, on_bad_lines='skip', header=None)

    # Guitar strings (1 = high E, 6 = low E) with open string MIDI note values
    STRING_RANGES = {
        6: (40, 48),  # Low E (40) to 8th fret (48)
        5: (45, 53),  # A string
        4: (50, 58),  # D string
        3: (55, 63),  # G string
        2: (59, 67),  # B string
        1: (64, 72)   # High E string
    }

    # Track active notes and string assignments
    active_notes = {}  # Active notes mapping (note -> string)
    string_usage = {s: None for s in STRING_RANGES}  # Track which string is playing which note
    commands = []  # To hold time, string, and fret information

    # Process MIDI messages
    for _, row in midi_data.iterrows():
        if len(row) < 6:
            continue  # Skip malformed rows

        track, time, event, channel, note, velocity = row[:6]

        if 'Note_on_c' in str(event) and velocity > 0:
            # If the note is already playing, do not reassign it
            if note in active_notes:
                continue  # Skip if note is already assigned
            
            # Find the lowest available string within the note range
            available_string = None
            for s in sorted(STRING_RANGES.keys(), reverse=True):  # Reverse sorted to prefer higher strings
                min_note, max_note = STRING_RANGES[s]
                if string_usage[s] is None and min_note <= note <= max_note:
                    available_string = s
                    break

            if available_string:
                # Calculate the fret number by subtracting the open string note from the assigned note
                open_note = STRING_RANGES[available_string][0]  # Get the open string note (e.g., E, A, D, G, B, or E)
                fret_number = note - open_note
                commands.append((time, available_string, fret_number))  # Append time, string, and fret number
                active_notes[note] = available_string  # Mark note as active
                string_usage[available_string] = note  # Assign string to note

        elif 'Note_off_c' in str(event) or ('Note_on_c' in str(event) and velocity == 0):
            # When a note is stopped, free up the corresponding string
            if note in active_notes:
                assigned_string = active_notes[note]
                string_usage[assigned_string] = None  # Mark string as available
                del active_notes[note]  # Remove note from active notes

    # Sort commands by time to ensure the events are ordered chronologically
    commands.sort()

    return commands  # Return the list of commands (time, string, fret_number)


def print_midi_file(infile,outfile=None):
    '''few notes about midi:
    - midi files are composed of tracks
    - each track is composed of messages
    - each message is composed of a type and a value
    - the type can be note_on, note_off, control_change, etc
    - time is mesured in pulses per quarter note (ppqn)'''
    csv_string_list = pm.midi_to_csv(infile)
    with open(infile + ".csv", "w") as f:
     f.writelines(csv_string_list)

    # Parse the CSV output of the previous command back into a MIDI file
    midi_object = pm.csv_to_midi(csv_string_list)

    # # Save the parsed MIDI file to disk
    # with open(outfile + ".mid", "wb") as output_file:
    #     midi_writer = pm.FileWriter(output_file)
    #     midi_writer.write(midi_object)
    # mid = mido.MidiFile(file_path)
    # with open("midi_messages_nor.txt", "w") as f:
    #     for i, track in enumerate(mid.tracks):
    #         f.write(f"Track {i}: {track.name}\n")
    #         for msg in track:
    #             f.write(f"{msg}\n")
    # # for i, track in enumerate(mid.tracks):
    # #     print(f"Track {i}: {track.name}")
    # #     for msg in track:
    # #         if msg.is_meta:
    # #             print(msg)
    # # #mid.print_tracks()
    # # for i, track in enumerate(mid.tracks):
    # #     for msg in track:
    # #         if msg.type == 'program_change':
    # #             print(f"Track {i} Channel {msg.channel} Instrument {msg.program}")
    # new_con = mido.MidiFile()
    # for i, track in enumerate(mid.tracks):
    #     new_mid = mido.MidiFile()
    #     new_file_path = f"track_{i}.mid"
    #     new_con.tracks.append(track)
    #     new_mid.tracks.append(track)
    #     new_mid.save(new_file_path)
    #     print(f"Saved track {i} as {new_file_path}")
    # new_con.save("test1.mid")
    # for msg in new_con:
    #     if msg.type == 'program_change':
    #         print(f"Track {i} Channel {msg.channel} Instrument {msg.program}")
    #     #print(msg)

if __name__ == '__main__':
    print_midi_file('output_guitar_simulation.mid')
    #create_midi_from_guitar(process_midi_to_guitar('midi_tracks\Tarrega_Gran_Vals.mid.csv'), 'output_guitar_simulation.mid')
    #imulate_guitar_playing(process_midi_to_guitar('midi_tracks\Tarrega_Recuerdos_de_la_Alhambra.mid.csv'))
