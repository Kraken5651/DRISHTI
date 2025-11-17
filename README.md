# =========================================================
#  DRISHTI — Wireless Smart Camera Aiming System
#  Terminal-Style README.md
# =========================================================

$ whoami
> A smart, wireless pan–tilt camera control system built using ESP32,
> joystick remote input, and optional OpenCV auto-tracking.

$ mission
> Smooth real-time camera movement.
> Wireless manual control.
> Automated object/face tracking.
> Simple hardware. Flexible software. Clean design.

# ---------------------------------------------------------
#  FEATURES
# ---------------------------------------------------------
$ drishti --features
> • ESP32 joystick remote (X/Y + button)  
> • OLED display: mode + live coordinates  
> • Wi-Fi UDP low-latency link  
> • Smooth 2-axis servo pan–tilt  
> • Laser toggle button (optional)  
> • Auto-tracking with OpenCV (Python)  
> • Manual mode for direct joystick control  
> • Easy to expand, customize, and integrate  

# ---------------------------------------------------------
#  SYSTEM OVERVIEW
# ---------------------------------------------------------
$ drishti --architecture
> REMOTE ESP32 (joystick + OLED + buttons)
>         │
>         └── Wi-Fi UDP ──► RECEIVER ESP32
>                           │
>                           └── PAN-TILT SERVOS ──► camera/laser module
>
> Optional:
> PC/OpenCV ──► tracking coordinates ──► receiver ESP32

# ---------------------------------------------------------
#  HARDWARE
# ---------------------------------------------------------
$ ls hardware/
> Remote:
>   - ESP32 Dev Board
>   - Joystick module (X/Y)
>   - SSD1306 OLED (SDA=25, SCL=26)
>   - Mode switch
>   - Laser toggle switch
>   - Buzzer

> Receiver:
>   - ESP32 Dev Board
>   - 2 × Servos (pan + tilt)
>   - 5V regulated power supply
>   - Optional laser module

> Tracking:
>   - Webcam
>   - Python + OpenCV

# ---------------------------------------------------------
#  SOFTWARE COMPONENTS
# ---------------------------------------------------------
$ tree software/
> remote/      # captures joystick + buttons, updates OLED, sends UDP
> receiver/    # reads UDP, smooths servo motion, applies mode logic
> tracking/    # OpenCV face/black-object detection + UDP coordinate output

# ---------------------------------------------------------
#  COMMUNICATION FORMAT
# ---------------------------------------------------------
$ cat udp_packets.txt
> MODE:MANUAL
> MODE:AUTO
> X,Y        # e.g., 90,120

# ---------------------------------------------------------
#  USE CASES
# ---------------------------------------------------------
$ drishti --use-cases
> robotics prototypes  
> camera motion rigs  
> remote inspection systems  
> object-tracking demonstrations  
> college engineering projects  
> smart automation setups  

# ---------------------------------------------------------
#  FUTURE ROADMAP
# ---------------------------------------------------------
$ drishti --roadmap
> • Brushless gimbal upgrade
> • Mobile app control
> • Edge-AI tracking on ESP32-S3
> • Mesh networking for long range
> • Stabilization algorithms

# ---------------------------------------------------------
#  COLLABORATORS
# ---------------------------------------------------------
$ nano CONTRIBUTORS.md
> Add your name below:
>
> • Karan Kumar — ESP32 Controlled Joystick Remote
> • Harsh Raj - 
> • Ashutosh Bharti -

# ---------------------------------------------------------
#  END OF FILE
# ---------------------------------------------------------
$ exit
