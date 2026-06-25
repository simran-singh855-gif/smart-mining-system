# MicroPython script for Arduino Nicla Vision (The Eyes)
# Project: Smart Mining & Safety System
# Role: Camera people counter running a privacy-first FOMO model, sending count over I2C.

import sensor
import image
import time
import tf
from pyb import I2C

# =============================================================================
# Camera & Model Initialization
# =============================================================================
sensor.reset()
sensor.set_pixformat(sensor.RGB565)  # Use RGB color frames
sensor.set_framesize(sensor.QVGA)    # 320x240 resolution
sensor.set_windowing((240, 240))     # Crop to square window for model compatibility
sensor.skip_frames(60)                # Allow sensor parameters to stabilize (60 frames ~ 2 seconds)

# Load the trained FOMO (Faster Objects, More Objects) model from Edge Impulse.
# The model file 'model.tflite' must be copied to the root of the Nicla Vision flash drive.
model_path = "model.tflite"
labels = ["???", "person"]

try:
    # Load the network into memory
    net = tf.load(model_path, load_to_fb=True)
    print("[Vision] Model loaded successfully: %s" % model_path)
except Exception as e:
    net = None
    print("[ERROR] Could not load model '%s': %s" % (model_path, str(e)))
    print("[ERROR] Please copy your Edge Impulse TFLite model to the device root.")

# =============================================================================
# Select slave/peripheral mode constant depending on firmware version
i2c_mode = getattr(I2C, 'PERIPHERAL', None) or getattr(I2C, 'SLAVE', None)
if i2c_mode is None:
    raise ValueError("Neither I2C.PERIPHERAL nor I2C.SLAVE is defined in this firmware!")

# Initialize I2C Bus 1 (ESLOV / SCL & SDA pins on the Nicla Vision outer connectors)
# Portenta H7 acts as the master and will poll address 0x35 for the count.
i2c = I2C(1, i2c_mode, addr=0x35)

# Transmit buffer (1-byte payload for worker count)
tx_buf = bytearray(1)

# Time clock for measuring framerate (FPS)
clock = time.clock()

print("[Vision] I2C Peripheral Mode Initialized at address 0x35. Starting loop...")

# =============================================================================
# Main Inference Loop
# =============================================================================
while True:
    clock.tick()
    
    # 1. Capture snapshot from camera sensor
    img = sensor.snapshot()
    
    head_count = 0
    
    # 2. Run object detection using the FOMO model (if loaded)
    if net is not None:
        # net.detect runs the TFLite model on the frame.
        # It returns a list of lists (one list of detections per class).
        # We set class 0 (background/???) threshold to 255 to ignore it completely.
        # We set class 1 (person) threshold to 127 (50% confidence).
        for i, detection_list in enumerate(net.detect(img, thresholds=[(255, 255), (127, 255)])):
            if i == 0:
                continue  # Skip the background/negative class
                
            for d in detection_list:
                head_count += 1
                
                # Draw visual feedback on the framebuffer (visible in OpenMV IDE)
                # FOMO detects center points, resulting in small bounding boxes.
                x, y, w, h = d.rect()
                center_x = x + (w // 2)
                center_y = y + (h // 2)
                
                # Draw green indicator circle at the center coordinates (protects face privacy)
                img.draw_circle(center_x, center_y, 4, color=(0, 255, 0), thickness=2)
    
    # 3. Update the I2C transmit buffer
    # Cap headcount to 255 to fit in a single byte
    tx_buf[0] = min(head_count, 255)
    
    # 4. Transmit headcount byte to the Portenta H7 Master
    # A short timeout (5ms) prevents the camera loop from freezing if the H7 is rebooting or busy.
    try:
        i2c.send(tx_buf, timeout=5)
    except OSError:
        # OSError occurs if the I2C master does not acknowledge or initiate a read.
        # We catch it and pass to ensure the camera loop and FPS remain stable.
        pass
        
    # Log local statistics to serial console
    print("FPS: %.2f | Detected Workers: %d" % (clock.fps(), head_count))
