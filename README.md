# Ansitovideo
<img height="503" alt="image" src="https://github.com/user-attachments/assets/6a6e0f2e-2d1b-4e08-96da-aa88df41edb1" />

**DON'T ASK ABOUT VIDEO AND FILENAME...**
This is project that transfers output of initframis to video, but in ASCII form.
## About 
- I created initframis, it works, but .full file is not supported as a video format and I had to do something.
- This script does transfer Initframis 1.1's output to mp4, webm or any other video format.
- Again, I don't know how is this supposed to work on Windows, other than a VM or WSL.
## Dependencies
- C++ libraries
- Ffmpeg
- C++ compiler, I am using g++
## Usage
- This program should be ran in any bash terminal, I think.
- How to run it:
  ```
  ./ansitovideo input.full out.mp4
  ```
### Arguments
- Input file, output of initframis and
- Output file, mp4.
## Compiling
- You can use release .deb file, or
- build it yourself with the script. Note that you might need to do `chmod +x compileatv.sh` to run the script.
## After converting
This is what I like to do, and so do you maybe:
1. Add audio:
    ```
    ffmpeg -i out_of_ansitovideo.mp4 -i original_video_or_audio.mp4 -c:v copy -map 0:v:0 -map 1:a:0 -shortest output_with_audio.mp4
    ```
2. Add subtitle if they exis. How? Let me explain.
   1. Get subtitles from youtube with an extension
   2. Run this, don't worry: `ffmpeg -i output_with_audio.mp4 -i subtitles.srt -c copy -c:s mov_text output_with_subs.mp4`
