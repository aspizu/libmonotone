set -x
gcc $(cat compile_flags.txt) *.c -o libmonotone &&\
valgrind ./libmonotone songs/NO_ROADS.MON output.pcm &&\
ffmpeg -y -f u8 -ar 44100 -ac 2 -i output.pcm output.mp3
