import numpy as np
import pyaudio

STRING_OPEN_NOTE_FREQ = {
        6: 40,  # E2
        5: 45,  # A2
        4: 50,  # D3
        3: 55,  # G3
        2: 59,  # B3
        1: 64   # E4
    }

def midi_to_freq(midi_note):
    return 440.0 * 2 ** ((midi_note - 69) / 12)

