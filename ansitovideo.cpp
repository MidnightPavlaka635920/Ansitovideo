#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ft2build.h>
#include FT_FREETYPE_H

const int MY_CHAR_WIDTH = 8;
const int CHAR_HEIGHT = 16;

// RGB struct
struct RGB {
    unsigned char r, g, b;
};

// Frame buffer
struct Frame {
    std::vector<unsigned char> pixels; // RGB24
};

// UTF-8 helper
size_t utf8CharLength(const char* s) {
    unsigned char c = s[0];
    if((c & 0x80)==0) return 1;
    else if((c & 0xE0)==0xC0) return 2;
    else if((c & 0xF0)==0xE0) return 3;
    else if((c & 0xF8)==0xF0) return 4;
    return 1;
}

// Draw a font-rendered character
void drawFontChar(Frame &frame, int x, int y, const char* c, const RGB &fg, const RGB &bg, int frameW, int frameH, FT_Face face) {
    unsigned int codepoint = 0;
    const unsigned char* s = (const unsigned char*)c;
    if ((s[0] & 0x80) == 0) codepoint = s[0];
    else if ((s[0] & 0xE0) == 0xC0) codepoint = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
    else if ((s[0] & 0xF0) == 0xE0) codepoint = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    else if ((s[0] & 0xF8) == 0xF0) codepoint = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);

    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) return;

    FT_GlyphSlot g = face->glyph;
    for(int row=0; row<g->bitmap.rows; ++row) {
        for(int col=0; col<g->bitmap.width; ++col) {
            int fx = x + col + g->bitmap_left;
            int fy = y + (CHAR_HEIGHT - g->bitmap_top) + row;
            if(fx<0 || fy<0 || fx>=frameW || fy>=frameH) continue;

            unsigned char val = g->bitmap.buffer[row * g->bitmap.pitch + col];
            float alpha = val / 255.0f;
            int idx = 3*(fy*frameW + fx);
            frame.pixels[idx+0] = (unsigned char)(fg.r*alpha + bg.r*(1-alpha));
            frame.pixels[idx+1] = (unsigned char)(fg.g*alpha + bg.g*(1-alpha));
            frame.pixels[idx+2] = (unsigned char)(fg.b*alpha + bg.b*(1-alpha));
        }
    }
}

// Draw block/half block characters
void drawChar(Frame &frame, int x, int y, const char* c, const RGB &fg, const RGB &bg, int frameW, int frameH) {
    if(strcmp(c,"▀")==0) {
        for(int j=0;j<CHAR_HEIGHT/2;++j)
            for(int i=0;i<MY_CHAR_WIDTH;++i)
                if(x+i<frameW && y+j<frameH) {
                    int idx=3*((y+j)*frameW + (x+i));
                    frame.pixels[idx+0]=fg.r; frame.pixels[idx+1]=fg.g; frame.pixels[idx+2]=fg.b;
                }
        for(int j=CHAR_HEIGHT/2;j<CHAR_HEIGHT;++j)
            for(int i=0;i<MY_CHAR_WIDTH;++i)
                if(x+i<frameW && y+j<frameH) {
                    int idx=3*((y+j)*frameW + (x+i));
                    frame.pixels[idx+0]=bg.r; frame.pixels[idx+1]=bg.g; frame.pixels[idx+2]=bg.b;
                }
    } else if(strcmp(c,"▄")==0) {
        for(int j=0;j<CHAR_HEIGHT/2;++j)
            for(int i=0;i<MY_CHAR_WIDTH;++i)
                if(x+i<frameW && y+j<frameH) {
                    int idx=3*((y+j)*frameW + (x+i));
                    frame.pixels[idx+0]=bg.r; frame.pixels[idx+1]=bg.g; frame.pixels[idx+2]=bg.b;
                }
        for(int j=CHAR_HEIGHT/2;j<CHAR_HEIGHT;++j)
            for(int i=0;i<MY_CHAR_WIDTH;++i)
                if(x+i<frameW && y+j<frameH) {
                    int idx=3*((y+j)*frameW + (x+i));
                    frame.pixels[idx+0]=fg.r; frame.pixels[idx+1]=fg.g; frame.pixels[idx+2]=fg.b;
                }
    } else if(strcmp(c,"█")==0) {
        for(int j=0;j<CHAR_HEIGHT;++j)
            for(int i=0;i<MY_CHAR_WIDTH;++i)
                if(x+i<frameW && y+j<frameH) {
                    int idx=3*((y+j)*frameW + (x+i));
                    frame.pixels[idx+0]=fg.r; frame.pixels[idx+1]=fg.g; frame.pixels[idx+2]=fg.b;
                }
    } else {
        for(int j=0;j<CHAR_HEIGHT;++j)
            for(int i=0;i<MY_CHAR_WIDTH;++i)
                if(x+i<frameW && y+j<frameH) {
                    int idx=3*((y+j)*frameW + (x+i));
                    frame.pixels[idx+0]=bg.r; frame.pixels[idx+1]=bg.g; frame.pixels[idx+2]=bg.b;
                }
    }
}

// Parse ANSI reset \033[0m
bool parseAnsiReset(const std::string &s, size_t &pos, RGB &fg, RGB &bg) {
    if(s[pos]!='\033') return false;
    if(s.substr(pos,3) != "\033[0") return false;
    size_t next = s.find('m', pos);
    if(next == std::string::npos) return false;
    fg = {255,255,255};
    bg = {0,0,0};
    pos = next+1;
    return true;
}

// Parse foreground color \033[38;2;R;G;Bm
bool parseAnsiFG(const std::string &s, size_t &pos, RGB &fg) {
    if(s[pos]!='\033' || s.substr(pos,7)!="\033[38;2;") return false;
    pos+=7;
    size_t next=s.find('m', pos);
    if(next==std::string::npos) return false;
    int r,g,b;
    sscanf(s.substr(pos,next-pos).c_str(), "%d;%d;%d",&r,&g,&b);
    fg={ (unsigned char)r, (unsigned char)g, (unsigned char)b };
    pos=next+1;
    return true;
}

// Parse background color \033[48;2;R;G;Bm
bool parseAnsiBG(const std::string &s, size_t &pos, RGB &bg) {
    if(s[pos]!='\033' || s.substr(pos,7)!="\033[48;2;") return false;
    pos+=7;
    size_t next=s.find('m', pos);
    if(next==std::string::npos) return false;
    int r,g,b;
    sscanf(s.substr(pos,next-pos).c_str(), "%d;%d;%d",&r,&g,&b);
    bg={ (unsigned char)r, (unsigned char)g, (unsigned char)b };
    pos=next+1;
    return true;
}

int main(int argc, char* argv[]){
    if(argc<3){ std::cerr<<"Usage: "<<argv[0]<<" input.full output.mp4\n"; return 1; }

    std::ifstream fin(argv[1]);
    if(!fin){ std::cerr<<"Cannot open input\n"; return 1; }

    FT_Library ft;
    FT_Face face;
    if(FT_Init_FreeType(&ft)){ std::cerr<<"Could not init FreeType\n"; return 1; }
    if(FT_New_Face(ft,"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",0,&face)){ std::cerr<<"Could not load font\n"; return 1; }
    FT_Set_Pixel_Sizes(face,0,CHAR_HEIGHT);

    int W=80,H=50;
    float FPS=25;
    std::string line;
    while(std::getline(fin,line)){
        if(line.rfind("FPS:",0)==0) FPS=std::stof(line.substr(4));
        else if(line.rfind("W:",0)==0) W=std::stoi(line.substr(2));
        else if(line.rfind("H:",0)==0) H=std::stoi(line.substr(2));
        else break;
    }

    int width=W*MY_CHAR_WIDTH, height=H*CHAR_HEIGHT;
    std::cerr<<"Video: "<<W<<"x"<<H<<" chars -> "<<width<<"x"<<height<<" px, FPS: "<<FPS<<"\n";

    Frame current;
    current.pixels.resize(width*height*3);

    char cmd[1024];
    const char* fps_str=(labs(FPS-29.97)<0.01)?"30000/1001":
                        (labs(FPS-23.976)<0.01)?"24000/1001":
                        (labs(FPS-59.94)<0.01)?"60000/1001":nullptr;
    if(fps_str) snprintf(cmd,sizeof(cmd),
        "ffmpeg -y -f rawvideo -pixel_format rgb24 -video_size %dx%d -framerate %s -i - -r %s -c:v libx264 -pix_fmt yuv420p \"%s\"",
        width,height,fps_str,fps_str,argv[2]);
    else snprintf(cmd,sizeof(cmd),
        "ffmpeg -y -f rawvideo -pixel_format rgb24 -video_size %dx%d -framerate %.5f -i - -r %.5f -c:v libx264 -pix_fmt yuv420p \"%s\"",
        width,height,FPS,FPS,argv[2]);

    FILE* pipe=popen(cmd,"w");
    if(!pipe){ std::cerr<<"Cannot start ffmpeg\n"; return 1; }

    do {
        std::fill(current.pixels.begin(),current.pixels.end(),0);

        std::vector<std::string> frameLines;
        frameLines.push_back(line);
        for(int r=1;r<H;++r){
            if(!std::getline(fin,line)) break;
            frameLines.push_back(line);
        }
        if((int)frameLines.size()<H) break;

        for(int row=0; row<H; ++row){
            size_t pos=0;
            int col=0;
            RGB fg={255,255,255}, bg={0,0,0};
            const std::string &l = frameLines[row];
            while(col<W && pos<l.size()){
                // Reset first
                if(parseAnsiReset(l,pos,fg,bg)) continue;
                if(parseAnsiFG(l,pos,fg)) continue;
                if(parseAnsiBG(l,pos,bg)) continue;

                size_t clen=utf8CharLength(&l[pos]);
                char cbuf[5]={0};
                std::memcpy(cbuf,&l[pos],clen);

                if(strcmp(cbuf,"▀")==0 || strcmp(cbuf,"▄")==0 || strcmp(cbuf,"█")==0)
                    drawChar(current,col*MY_CHAR_WIDTH,row*CHAR_HEIGHT,cbuf,fg,bg,width,height);
                else
                    drawFontChar(current,col*MY_CHAR_WIDTH,row*CHAR_HEIGHT,cbuf,fg,bg,width,height,face);

                pos+=clen;
                col++;
            }
        }

        fwrite(current.pixels.data(),1,current.pixels.size(),pipe);
    } while(std::getline(fin,line));

    pclose(pipe);
    std::cerr<<"Done!\n";
    return 0;
}
