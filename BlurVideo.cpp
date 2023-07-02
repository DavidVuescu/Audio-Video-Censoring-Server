#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>

// Function to blur the faces in a video using Haar cascade
void blur_video(const char *inputFilename, const char *outputFilename)
{
  cv::CascadeClassifier cascade;
  cascade.load("haarcascade_frontalface_default.xml"); // Load the Haar cascade XML file

  cv::VideoCapture capture(inputFilename); // Open the input video file
  if (!capture.isOpened())
  {
    printf("Error opening video file\n");
    return;
  }

  cv::VideoWriter writer(outputFilename, cv::VideoWriter::fourcc('H', '2', '6', '4'), capture.get(cv::CAP_PROP_FPS),
                         cv::Size(capture.get(cv::CAP_PROP_FRAME_WIDTH), capture.get(cv::CAP_PROP_FRAME_HEIGHT)));

  cv::Mat frame, grayFrame;
  while (capture.read(frame))
  {
    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY); // Convert frame to grayscale

    std::vector<cv::Rect> faces;
    // Detect faces using Haar cascade
    cascade.detectMultiScale(grayFrame, faces, 1.1, 3, 0, cv::Size(30, 30));

    // Blur the detected faces
    for (const cv::Rect &face : faces)
    {
      cv::Mat faceROI = frame(face);
      cv::GaussianBlur(faceROI, faceROI, cv::Size(55, 55), 0);
    }

    writer.write(frame); // Write the processed frame to the output video
  }

  capture.release();
  writer.release();
}

// Function to distort the audio in a video using FFmpeg
void audio_distort(const char *inputFilename, const char *outputFilename)
{
  char command[512];
  sprintf(command, "ffmpeg -i %s -af \"asetrate=44100*1.5, atempo=0.6667\" -c:v copy %s", inputFilename, outputFilename);
  system(command);
}

// Function to combine the blurred video and distorted audio into a single video using FFmpeg
void combine_videos(const char *blurredVideo, const char *distortedAudio, const char *output_path)
{
  // Use FFmpeg to combine the videos
  char command[512];
  sprintf(command, "ffmpeg -i %s -i %s -c:v copy -c:a aac -strict experimental %s", blurredVideo, distortedAudio, output_path);
  system(command);
}

void finalize(const char *output_path)
{
  // Modify audio pitch
  char cmd_audio[512];
  sprintf(cmd_audio, "ffmpeg -i %s_SOUND.wav -filter_complex '[0:a]atempo=0.7,asetrate=44100*1.3[out]' -map '[out]' %s_SOUND_MODIFIED.wav -y", output_path, output_path);
  system(cmd_audio);

  // Merge video and modified audio
  char cmd[512];
  sprintf(cmd, "ffmpeg -i %s_VIDEO.mp4 -i %s_SOUND_MODIFIED.wav -c:v copy -c:a aac -strict experimental %s -y", output_path, output_path, output_path);
  system(cmd);

  // Clean up temporary files
  remove((std::string(output_path) + "_SOUND_MODIFIED.wav").c_str());
  remove((std::string(output_path) + "_SOUND.wav").c_str());

  printf("VIDEO READY\n");
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Usage: program_name input_video output_video\n");
    return 1;
  }

  const char *inputVideo = argv[1];
  const char *blurredVideo = "blurred_video.mp4";
  const char *distortedAudio = "distorted_audio.mp4";
  std::string output_path = argv[2];

  // Blur video
  blur_video(inputVideo, blurredVideo);

  // Distort audio
  audio_distort(inputVideo, distortedAudio);

  // Combine videos
  combine_videos(blurredVideo, distortedAudio, output_path.c_str());

  // Finalize and clean up
  finalize(output_path.c_str());

  // Clean up intermediate files if needed
  remove(blurredVideo);
  remove(distortedAudio);

  return 0;
}
