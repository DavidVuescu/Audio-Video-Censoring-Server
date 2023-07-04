#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <random>
#include <string>

char* generate_random(bool isWAVFile) {
    const char CHARACTERS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, sizeof(CHARACTERS) - 2); // subtract 2 to account for the null character

    int length = isWAVFile ? 14 : 15; // Allocate length according to file type
    char* random_string = new char[length];  // length includes space for extension and null character

    for (int i = 0; i < 10; ++i) {
        random_string[i] = CHARACTERS[distribution(generator)];
    }

    // Append file extension based on isWAVFile flag
    if(isWAVFile) {
        strcpy(random_string + 10, ".wav");
    } else {
        strcpy(random_string + 10, ".mp4");
    }

    return random_string;
}


void blur_video(const char *inputFilename, const char *outputFilename, const char *output_path, const char *option)
{
    cv::CascadeClassifier cascade;
    cascade.load("haarcascade_frontalface_default.xml"); // Load the Haar cascade XML file

    cv::VideoCapture capture(inputFilename); // Open the input video file
    if (!capture.isOpened())
    {
        printf("Error opening video file\n");
        return;
    }

    cv::VideoWriter writer(outputFilename, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), capture.get(cv::CAP_PROP_FPS),
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

    if (strcmp(option, "1") == 0)
    {
        //const char *audioFilePath = "output_audio.wav";
        const char *audioFilePath = generate_random(true);

        // Build the FFmpeg command
        char command[1024];
        sprintf(command, "ffmpeg -i %s -vn -acodec pcm_s16le -ar 44100 -ac 2 %s  > /dev/null 2>&1", inputFilename, audioFilePath);
        system(command);

        char combineCommand[1024];
        sprintf(combineCommand, "ffmpeg -i %s -i %s -c:v copy -c:a aac -strict experimental %s > /dev/null 2>&1", outputFilename, audioFilePath, output_path);
        system(combineCommand);

        remove(audioFilePath);
        delete[] audioFilePath;
    }
}

void audio_distort(const char *inputFilename, const char *distortedAudio, const char *output_path, const char *option)
{
    char command[512];
    sprintf(command, "ffmpeg -i %s -af \"asetrate=44100*1.5, atempo=0.6667\" -c:v copy %s > /dev/null 2>&1", inputFilename, distortedAudio);
    system(command);

    if (strcmp(option, "2") == 0)
    {
        std::rename(distortedAudio, ("%s.mp4", output_path));
    }
}

void combine_videos(const char *blurredVideo, const char *distortedAudio, const char *output_path)
{
    // Use FFmpeg to combine the videos
    char command[512];
    sprintf(command, "ffmpeg -i %s -i %s -c:v copy -c:a aac -strict experimental %s > /dev/null 2>&1", blurredVideo, distortedAudio, output_path);
    system(command);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: program_name option input_video output_video\n");
        return 1;
    }

    const char *option = argv[1];
    const char *inputVideo = argv[2];
    //const char *blurredVideo = "blurred_video.mp4";
    const char *blurredVideo = generate_random(false);
    //const char *distortedAudio = "distorted_audio.mp4";
    const char *distortedAudio = generate_random(false);
    std::string output_path = argv[3];

    if (strcmp(option, "1") == 0)
    {
        blur_video(inputVideo, blurredVideo, output_path.c_str(), option);
    }

    if (strcmp(option, "2") == 0)
    {
        audio_distort(inputVideo, distortedAudio, output_path.c_str(), option);
    }

    if (strcmp(option, "3") == 0)
    {
        blur_video(inputVideo, blurredVideo, output_path.c_str(), option);
        audio_distort(inputVideo, distortedAudio, output_path.c_str(), option);
        combine_videos(blurredVideo, distortedAudio, output_path.c_str());
    }

    // Finalize and clean up
    // finalize(output_path.c_str());

    // Clean up intermediate files if needed
    remove(blurredVideo);
    remove(distortedAudio);

    delete[] blurredVideo;
    delete[] distortedAudio;

    return 0;
}
