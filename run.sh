set -x
gcc -std=c11 -Wall -Wextra -Werror libmonotone.c main.c -o libmonotone &&\
valgrind ./libmonotone songs/SHADILAY.MON output.pcm &&\
ffmpeg -y -f u8 -ar 44100 -ac 2 -i output.pcm output.mp3
