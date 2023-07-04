#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <unistd.h>

// Function to blur the faces in a video using Haar cascade
void blur_video(const char* inputFilename, const char* outputFilename, const char* output_path, const char* option)
{
    cv::CascadeClassifier cascade;
    cascade.load("haarcascade_frontalface_default.xml"); // Load the Haar cascade XML file

    cv::VideoCapture capture(inputFilename); // Open the input video file
    if (!capture.isOpened())
    {
        printf("Error opening video file\n");
        return;
    }

    cv::VideoWriter writer(outputFilename, cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), capture.get(cv::CAP_PROP_FPS),
                           cv::Size(capture.get(cv::CAP_PROP_FRAME_WIDTH), capture.get(cv::CAP_PROP_FRAME_HEIGHT)));

    cv::Mat frame, grayFrame;
    while (capture.read(frame))
    {
        cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY); // Convert frame to grayscale

        std::vector<cv::Rect> faces;
        // Detect faces using Haar cascade
        cascade.detectMultiScale(grayFrame, faces, 1.1, 3, 0, cv::Size(30, 30));

        // Blur the detected faces
        for (const cv::Rect& face : faces)
        {
            cv::Mat faceROI = frame(face);
            cv::GaussianBlur(faceROI, faceROI, cv::Size(55, 55), 0);
        }

        writer.write(frame); // Write the processed frame to the output video
    }

    capture.release();
    writer.release();

    if (strcmp("1", option) == 0)
    {
        // Output audio file path
        const char* audioFilePath = "output_audio.mp3";

        // Build the FFmpeg command
        char command[1024];
        sprintf(command, "ffmpeg -i %s -vn -acodec copy %s > /dev/null 2>&1", inputFilename, audioFilePath);
        system(command);

        char combineCommand[1024];
        sprintf(combineCommand, "ffmpeg -i %s -i %s -c:v copy -c:a copy %s > /dev/null 2>&1", outputFilename, audioFilePath, output_path);
        system(combineCommand);

        std::rename(outputFilename, ("%s.mp4", output_path));

        remove(audioFilePath);
    }
    
}

// Function to distort the audio in a video using FFmpeg
void audio_distort(const char* inputFilename, const char* distortedAudio, const char* output_path, const char* option)
{
    char command[512];
    sprintf(command, "ffmpeg -i %s -af \"asetrate=44100*1.5, atempo=0.6667\" -c:v copy %s > /dev/null 2>&1", inputFilename, distortedAudio);
    system(command);

    if (strcmp("2", option) == 0)
    {
        std::rename(distortedAudio, ("%s.mp4", output_path));
    }
}

// Function to combine the blurred video and distorted audio into a single video using FFmpeg
void combine_videos(const char* blurredVideo, const char* distortedAudio, const char* output_path)
{
    // Use FFmpeg to combine the videos
    char command[512];
    sprintf(command, "ffmpeg -i %s -i %s -c:v copy -c:a aac -strict experimental %s > /dev/null 2>&1", blurredVideo, distortedAudio, output_path);
    system(command);
}

void finalize(const char* inputVideo, const char* blurredVideo, const char* output_path)
{
    // Modify audio pitch
    char cmd_audio[512];
    sprintf(cmd_audio, "ffmpeg -i %s -filter_complex '[0:a]atempo=0.7,asetrate=44100*1.3[out]' -map '[out]' %s_SOUND_MODIFIED.wav -y > > /dev/null 2>&1", inputVideo, output_path);
    system(cmd_audio);

    // Merge video and modified audio
    char cmd[512];
    sprintf(cmd, "ffmpeg -i %s -i %s_SOUND_MODIFIED.wav -c:v copy -c:a aac -strict experimental %s -y > > /dev/null 2>&1", blurredVideo, output_path, output_path);
    system(cmd);

    // Clean up temporary files
    // remove((std::string(output_path) + "_SOUND_MODIFIED.wav").c_str());
    // remove((std::string(output_path) + "_SOUND.wav").c_str());

    printf("VIDEO READY\n");
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {		
        printf("Usage: program_name option input_video output_video\n");
        return 1;
    }

    const char* option = argv[1];
    const char* inputVideo = argv[2];
    const char* blurredVideo = "blurred_video.mp4";
    const char* distortedAudio = "distorted_audio.mp4";
    std::string output_path = argv[3];

    if (strcmp(option, "1") == 0)
    {
        // Blur video
        blur_video(inputVideo, blurredVideo, output_path.c_str(), option);
    }

    if (strcmp(option, "2") == 0)
    {
        // Distort audio
        audio_distort(inputVideo, distortedAudio, output_path.c_str(), option);
    }

    if (strcmp(option, "3") == 0)
    {
        // Combine videos
        blur_video(inputVideo, blurredVideo, output_path.c_str(), option);
        audio_distort(inputVideo, distortedAudio, output_path.c_str(), option);
        combine_videos(blurredVideo, distortedAudio, output_path.c_str());
    }

    // Finalize and clean up
    // finalize(output_path.c_str());

    // Clean up intermediate files if needed
    remove(blurredVideo);
    remove(distortedAudio);

    return 0;
}
