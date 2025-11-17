from flask import Flask, render_template, Response, jsonify
import cv2
import face_recognition
import os
import numpy as np
import time
from collections import Counter

app = Flask(__name__)

KNOWN_ENCODINGS = {"whitelist": [], "blacklist": []}
THRESHOLD = 0.45
CACHE_TIMEOUT = 10
RECOGNIZED_CACHE = {}
CURRENT_DETECTIONS = Counter()
DETECTION_LOG = []

FRAME_WIDTH = 640
FRAME_HEIGHT = 480


def load_faces():
    for list_type in ["whitelist", "blacklist"]:
        base_dir = os.path.join("known_faces", list_type)
        if not os.path.exists(base_dir):
            print(f"Folder {base_dir} not found!")
            continue
        for person in os.listdir(base_dir):
            person_dir = os.path.join(base_dir, person)
            if not os.path.isdir(person_dir):
                continue
            for img_name in os.listdir(person_dir):
                img_path = os.path.join(person_dir, img_name)
                image = face_recognition.load_image_file(img_path)
                encodings = face_recognition.face_encodings(image)
                if encodings:
                    KNOWN_ENCODINGS[list_type].append((encodings[0], person))


def get_face_id(face_encoding):
    return hash(tuple(np.round(face_encoding, 3)))


def reidentify(face_encoding):
    now = time.time()
    face_id = get_face_id(face_encoding)

    to_delete = [fid for fid, (ts, _) in RECOGNIZED_CACHE.items() if now - ts > CACHE_TIMEOUT]
    for fid in to_delete:
        del RECOGNIZED_CACHE[fid]

    if face_id in RECOGNIZED_CACHE:
        ts, category = RECOGNIZED_CACHE[face_id]
        RECOGNIZED_CACHE[face_id] = (now, category)
        return True, category
    return False, None


def detect_faces():
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_WIDTH)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)
    process_this_frame = True

    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        frame = cv2.flip(frame, 1)
        small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)
        rgb_small_frame = cv2.cvtColor(small_frame, cv2.COLOR_BGR2RGB)
        face_names = []

        if process_this_frame:
            CURRENT_DETECTIONS.clear()

            face_locations = face_recognition.face_locations(rgb_small_frame, model='hog')
            face_encodings = face_recognition.face_encodings(rgb_small_frame, face_locations)

            for face_encoding, face_location in zip(face_encodings, face_locations):
                name = "Unknown"
                category = "Unknown"
                face_id = get_face_id(face_encoding)

                cached, cached_category = reidentify(face_encoding)
                if cached:
                    name = "Cached"
                    category = cached_category
                else:
                    for known_encoding, person_name in KNOWN_ENCODINGS["whitelist"]:
                        dist = np.linalg.norm(known_encoding - face_encoding)
                        if dist < THRESHOLD:
                            name = person_name
                            category = "Whitelist"
                            RECOGNIZED_CACHE[face_id] = (time.time(), category)
                            break

                    if category == "Unknown":
                        for known_encoding, person_name in KNOWN_ENCODINGS["blacklist"]:
                            dist = np.linalg.norm(known_encoding - face_encoding)
                            if dist < THRESHOLD:
                                name = person_name
                                category = "Blacklist"
                                RECOGNIZED_CACHE[face_id] = (time.time(), category)
                                break

                CURRENT_DETECTIONS[category] += 1
                DETECTION_LOG.append(f"{time.strftime('%H:%M:%S')} - {category}: {name}")
                face_names.append((name, category, face_location))

        process_this_frame = not process_this_frame

        for name, category, (top, right, bottom, left) in face_names:
            top *= 4
            right *= 4
            bottom *= 4
            left *= 4
            color = (0, 255, 0) if category == "Whitelist" else (0, 0, 255) if category == "Blacklist" else (0, 255, 255)
            label = f"{category}: {name}" if category != "Unknown" else "Unknown"
            cv2.rectangle(frame, (left, top), (right, bottom), color, 2)
            cv2.putText(frame, label, (left, top - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2)

        ret, jpeg = cv2.imencode('.jpg', frame)
        if not ret:
            continue
        yield (b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + jpeg.tobytes() + b'\r\n')


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/video_feed')
def video_feed():
    return Response(detect_faces(), mimetype='multipart/x-mixed-replace; boundary=frame')


@app.route('/current_data')
def current_data():
    return jsonify({
        "whitelist": CURRENT_DETECTIONS.get("Whitelist", 0),
        "blacklist": CURRENT_DETECTIONS.get("Blacklist", 0),
        "unknown": CURRENT_DETECTIONS.get("Unknown", 0),
        "log": DETECTION_LOG[-20:]
    })


if __name__ == '__main__':
    print("Loading known faces...")
    load_faces()
    app.run(host='0.0.0.0', port=5000, debug=False)
