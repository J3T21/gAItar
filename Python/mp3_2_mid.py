from basic_pitch.inference import predict, predict_and_save
from basic_pitch import ICASSP_2022_MODEL_PATH
import pretty_midi

model_output, midi_data, note_events = predict("midi_tracks\Blackbird.mp3")

midi_data.write("midi_tracks\output_blackbird.mid")


