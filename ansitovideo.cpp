#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

const int CHAR_WIDTH  = 8;
const int CHAR_HEIGHT = 16;

// RGB struct
struct RGB {
    unsigned char r, g, b;
};

// Frame buffer
struct Frame {
    std::vector<unsigned char> pixels; // RGB24
};

// Parse ANSI color escape: "\033[38;2;R;G;Bm"
bool parseAnsiColor(const std::string &s, size_t &pos, RGB &color) {
    if (s[pos] != '\033') return false;
    if (s.substr(pos, 7) != "\033[38;2;") return false;
    pos += 7;
    size_t next = s.find('m', pos);
    if (next == std::string::npos) return false;
    std::string nums = s.substr(pos, next - pos);
    int r, g, b;
    if(sscanf(nums.c_str(), "%d;%d;%d", &r, &g, &b) != 3) return false;
    color = { (unsigned char)r, (unsigned char)g, (unsigned char)b };
    pos = next + 1;
    return true;
}

// Draw character into frame
void drawChar(Frame &frame, int x, int y, const char* c, const RGB &fg, const RGB &bg, int frameW, int frameH) {
    if(strcmp(c,"▀")==0) {
        // Top half foreground, bottom half background
        for(int j=0;j<CHAR_HEIGHT/2;j++){
            if(y+j>=frameH) break;
            for(int i=0;i<CHAR_WIDTH;i++){
                if(x+i>=frameW) break;
                int idx = 3*((y+j)*frameW + (x+i));
                frame.pixels[idx+0] = fg.r;
                frame.pixels[idx+1] = fg.g;
                frame.pixels[idx+2] = fg.b;
            }
        }
        for(int j=CHAR_HEIGHT/2;j<CHAR_HEIGHT;j++){
            if(y+j>=frameH) break;
            for(int i=0;i<CHAR_WIDTH;i++){
                if(x+i>=frameW) break;
                int idx = 3*((y+j)*frameW + (x+i));
                frame.pixels[idx+0] = bg.r;
                frame.pixels[idx+1] = bg.g;
                frame.pixels[idx+2] = bg.b;
            }
        }
    } else if(strcmp(c,"▄")==0) {
        // Top half background, bottom half foreground
        for(int j=0;j<CHAR_HEIGHT/2;j++){
            if(y+j>=frameH) break;
            for(int i=0;i<CHAR_WIDTH;i++){
                if(x+i>=frameW) break;
                int idx = 3*((y+j)*frameW + (x+i));
                frame.pixels[idx+0] = bg.r;
                frame.pixels[idx+1] = bg.g;
                frame.pixels[idx+2] = bg.b;
            }
        }
        for(int j=CHAR_HEIGHT/2;j<CHAR_HEIGHT;j++){
            if(y+j>=frameH) break;
            for(int i=0;i<CHAR_WIDTH;i++){
                if(x+i>=frameW) break;
                int idx = 3*((y+j)*frameW + (x+i));
                frame.pixels[idx+0] = fg.r;
                frame.pixels[idx+1] = fg.g;
                frame.pixels[idx+2] = fg.b;
            }
        }
    } else if(strcmp(c,"█")==0) {
        // Full block foreground
        for(int j=0;j<CHAR_HEIGHT;j++){
            if(y+j>=frameH) break;
            for(int i=0;i<CHAR_WIDTH;i++){
                if(x+i>=frameW) break;
                int idx = 3*((y+j)*frameW + (x+i));
                frame.pixels[idx+0] = fg.r;
                frame.pixels[idx+1] = fg.g;
                frame.pixels[idx+2] = fg.b;
            }
        }
    } else {
        // Default: fill with background
        for(int j=0;j<CHAR_HEIGHT;j++){
            if(y+j>=frameH) break;
            for(int i=0;i<CHAR_WIDTH;i++){
                if(x+i>=frameW) break;
                int idx = 3*((y+j)*frameW + (x+i));
                frame.pixels[idx+0] = bg.r;
                frame.pixels[idx+1] = bg.g;
                frame.pixels[idx+2] = bg.b;
            }
        }
    }
}

// UTF-8 helper
size_t utf8CharLength(const char* s) {
    unsigned char c = s[0];
    if((c & 0x80)==0) return 1;
    else if((c & 0xE0)==0xC0) return 2;
    else if((c & 0xF0)==0xE0) return 3;
    else if((c & 0xF8)==0xF0) return 4;
    return 1;
}

int main(int argc, char* argv[]){
    if(argc<3){
        std::cerr << "Usage: " << argv[0] << " input.full output.mp4\n";
        return 1;
    }

    std::ifstream fin(argv[1]);
    if(!fin){ std::cerr<<"Cannot open input\n"; return 1; }

    // Read header
    int FPS=25, W=80, H=50;
    std::string line;
    while(std::getline(fin,line)){
        if(line.rfind("FPS:",0)==0) FPS = std::stoi(line.substr(4));
        else if(line.rfind("W:",0)==0) W = std::stoi(line.substr(2));
        else if(line.rfind("H:",0)==0) H = std::stoi(line.substr(2));
        else break;
    }

    int width  = W*CHAR_WIDTH;
    int height = H*CHAR_HEIGHT;
    std::cerr<<"Video: "<<W<<"x"<<H<<" chars -> "<<width<<"x"<<height<<" px, FPS: "<<FPS<<"\n";
    Frame current;
current.pixels.resize(width*height*3);

RGB fg={255,255,255}, bg={0,0,0};

// FFmpeg pipeline
char cmd[1024];
snprintf(cmd,sizeof(cmd),
    "ffmpeg -y -f rawvideo -pixel_format rgb24 "
    "-video_size %dx%d -framerate %d -i - "
    "-c:v libx264 -pix_fmt yuv420p \"%s\"",
    width,height,FPS,argv[2]);

FILE* pipe = popen(cmd,"w");
if(!pipe){ std::cerr<<"Cannot start ffmpeg\n"; return 1; }

do {
    // clear frame
    std::fill(current.pixels.begin(), current.pixels.end(), 0);

    std::vector<std::string> frameLines;
    frameLines.push_back(line); // first line already read
    for (int row = 1; row < H; ++row) {
        if (!std::getline(fin, line)) break;
        frameLines.push_back(line);
    }
    if ((int)frameLines.size() < H) break; // incomplete frame

    // Draw each line using improved column logic
    for (int row = 0; row < H; ++row) {
    size_t pos = 0;
    const std::string& l = frameLines[row];
    int col = 0;
    while (col < W && pos < l.size()) {
        RGB fg, bg;
        // Parse foreground color
        if (l[pos] == '\033' && l.substr(pos, 7) == "\033[38;2;") {
            pos += 7;
            size_t next = l.find('m', pos);
            std::string nums = l.substr(pos, next - pos);
            int r, g, b;
            sscanf(nums.c_str(), "%d;%d;%d", &r, &g, &b);
            fg = { (unsigned char)r, (unsigned char)g, (unsigned char)b };
            pos = next + 1;
        }
        // Parse background color
        if (l[pos] == '\033' && l.substr(pos, 7) == "\033[48;2;") {
            pos += 7;
            size_t next = l.find('m', pos);
            std::string nums = l.substr(pos, next - pos);
            int r, g, b;
            sscanf(nums.c_str(), "%d;%d;%d", &r, &g, &b);
            bg = { (unsigned char)r, (unsigned char)g, (unsigned char)b };
            pos = next + 1;
        }
        // Parse character
        if ((unsigned char)l[pos] >= 0x20) { // printable
            const char* cptr = &l[pos];
            size_t clen = utf8CharLength(cptr);
            char cbuf[5] = {0};
            std::memcpy(cbuf, cptr, clen);
            drawChar(current, col*CHAR_WIDTH, row*CHAR_HEIGHT, cbuf, fg, bg, width, height);
            pos += clen;
            col++;
        } else {
            pos++; // skip non-printable
        }
    }
}
    // Write frame directly to ffmpeg
    fwrite(current.pixels.data(), 1, current.pixels.size(), pipe);
} while (std::getline(fin, line));

pclose(pipe);
std::cerr<<"Done!\n";
return 0;
    }
