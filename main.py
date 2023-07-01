import pyaudio
import wave
import threading
import cv2
import time
import numpy as np
import ffmpeg
import subprocess
import sys
import os
from moviepy.editor import VideoFileClip

def get_sound(video_path, output_path):
    # Load the video
    clip = VideoFileClip(video_path)

    # Extract audio
    audio = clip.audio

    # Save the audio to a file
    audio_output_path = output_path + '_SOUND.wav'
    audio.write_audiofile(audio_output_path)

    # Close the video clip
    clip.close()

def edit_video(video_path, output_path):
    cap = cv2.VideoCapture(video_path)

    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = cap.get(cv2.CAP_PROP_FPS)

    writer = cv2.VideoWriter(output_path + '_VIDEO.mp4', cv2.VideoWriter_fourcc(*'mp4v'), fps, (width, height))
    face_cascade = cv2.CascadeClassifier('haarcascade_frontalface_default.xml')

    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        faces = face_cascade.detectMultiScale(gray, 1.3, 5)
        for (x, y, w, h) in faces:
            # To draw a rectangle in a face
            cv2.rectangle(frame, (x - 20, y - 20), (x + w + 20, y + h + 20), (255, 255, 0), 2)
            frame[y - 20 : y + h + 20, x - 20 : x + w + 20] = cv2.blur(frame[y - 20 : y + h + 20, x - 20 : x + w + 20], (50, 50))

        writer.write(frame)
        frame = mirror_this(frame, False, False)
        # cv2.imshow('CONFIDENTIAL', frame)
        if cv2.waitKey(1) & 0xFF == 27:
            break

    cap.release()
    writer.release()
    cv2.destroyAllWindows()

def mirror_this(image_file, gray_scale=False, with_plot=False):
    image_rgb = image_file
    image_mirror = np.fliplr(image_rgb)
    if with_plot:
        fig = plt.figure(figsize=(10, 20))
        ax1 = fig.add_subplot(2, 2, 1)
        ax1.axis("off")
        ax1.title.set_text('Original')
        ax2 = fig.add_subplot(2, 2, 2)
        ax2.axis("off")
        ax2.title.set_text("Mirrored")
        if not gray_scale:
            ax1.imshow(image_rgb)
            ax2.imshow(image_mirror)
        else:
            ax1.imshow(image_rgb, cmap='gray')
            ax2.imshow(image_mirror, cmap='gray')
        return True
    return image_mirror

def finalize(output_path):
    # Modify audio pitch
    cmd_audio = ['ffmpeg', '-i', output_path + '_SOUND.wav', '-filter_complex', '[0:a]atempo=0.7,asetrate=44100*1.3[out]', '-map', '[out]', output_path + '_SOUND_MODIFIED.wav', '-y']
    with open(os.devnull, 'w') as devnull:
        subprocess.call(cmd_audio, stdout=devnull, stderr=subprocess.STDOUT)
    # Merge video and modified audio
    cmd = ['ffmpeg', '-i', output_path + '_VIDEO.mp4', '-i', output_path + '_SOUND_MODIFIED.wav', '-c:v', 'copy', '-c:a', 'aac', '-strict', 'experimental', output_path , '-y']
    with open(os.devnull, 'w') as devnull:
        subprocess.call(cmd, stdout=devnull, stderr=subprocess.STDOUT)

    # Clean up temporary files
    os.remove(output_path + '_SOUND_MODIFIED.wav')
    os.remove(output_path + '_SOUND.wav')

    print('VIDEO READY')
if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Please provide the input video path and output path.")
        print("Usage: python script.py <video_path> <output_path>")
        sys.exit(1)

    video_path = sys.argv[1]
    output_path = sys.argv[2]

    edit_video(video_path, output_path)
    get_sound(video_path, output_path)
    finalize(output_path)
    
    
    
    
    
    
