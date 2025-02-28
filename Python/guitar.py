
import pygame.midi

pygame.midi.init()
device_count = pygame.midi.get_count()

# Print available MIDI devices
for i in range(device_count):
    device_info = pygame.midi.get_device_info(i)
    print(f"Device {i}: {device_info}")

pygame.midi.quit()