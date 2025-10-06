ffmpeg_flags=$(pkg-config --cflags --libs libavformat libavcodec libswscale libavutil 2>/dev/null)
freetype_flags=$(pkg-config --cflags --libs freetype2 2>/dev/null)
g++ ansitovideo.cpp -o ansitovideo $ffmpeg_flags $freetype_flags
