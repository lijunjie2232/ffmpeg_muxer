#!/bin/bash

function FileSuffix() {
    local filename="$1"
    if [ -n "$filename" ]; then
        echo "${filename##*.}"
    fi
}

function FilePrefix() {
    local filename="$1"
    if [ -n "$filename" ]; then
        echo "${filename%.*}"
    fi
}


function CheckPath() {
    local output_path="$1"
    if [ ! -d $output_path ];then
      mkdir -p $output_path
    fi
}


input_path=$(cd $(dirname $0); pwd)
output_path="./output"
EXEC_PARAMS=(${@:1})
param_num=${#EXEC_PARAMS[*]}
current_path=$(cd $(dirname $0); pwd)

output_format="mp4"
video_format="mp4"
audio_format="mp3"


for((i=0;i<${#EXEC_PARAMS[*]};i++));do
  param=${EXEC_PARAMS[$i]}
  param_n=${EXEC_PARAMS[$i+1]}
  if [ "$param" = "--input" ];then
    input_path=$param_n
  elif [ "$param" = "--output" ]; then
    output_path=$param_n
  elif [ "$param" = "--out_fmt" ];then
    output_format=$param_n
  elif [ "$param" = "-vf" -o "$param" = "--video_fmt" ];then
    video_format=$param_n
  elif [ "$param" = "-af" -o "$param" = "--audio_fmt" ];then
    audio_format=$param_n
  fi
done

input_path=$(readlink -e $input_path)

if [[ "$output_path" =~ "^./".* ]]; then
  output_path=$current_path${output_path: 1 }
elif [[ "$output_path" =~ "../".* ]]; then
  output_path=$(dirname $current_path)${output_path: 2 }
elif expr "$output_path" : '[^\/]' > /dev/null ; then
  output_path=$current_path"/"$output_path
fi
if expr "$output_path" : '.*\/$' > /dev/null;then
  output_path=${output_path: :-1}
fi

if [ "$output_path" = "$input_path" ]; then
  echo output path should be different from input path 
  exit
fi

if expr "$output_format" : '[^\.]' > /dev/null;then
  output_format="."$output_format
fi
if expr "$video_format" : '[^\.]' > /dev/null;then
  video_format="."$video_format
fi
if expr "$audio_format" : '[^\.]' > /dev/null;then
  audio_format="."$audio_format
fi


if [ "$video_format" = "$audio_format" ]; then
  echo video format should be different from audio format
  exit
fi

if command -v ffmpeg >/dev/null 2>&1;then 
   echo ffmpeg detece
else 
   echo ffmpeg not detece
   exit
fi

CheckPath $output_path
output_path=$(readlink -e $output_path)

if [ "$output_path" = "$input_path" ]; then
  echo output path should be different from input path 
  exit
fi

echo -e "output: "$output_path"\ninput: "$input_path"\noutput format: "$output_format

for f in `ls $input_path`; do
    if [ ".""$(FileSuffix ${f})" = "$video_format" ] && [ -f $input_path"/""$(FilePrefix ${f})""$audio_format" ]; then
      echo -e "output: "$output_path"/"$(FilePrefix ${f})$output_format"\ninput: "$input_path"/"$f
      if [ $video_format == $output_format ]; then
        ffmpeg -i $input_path$"/"$f  -i $input_path"/"$(FilePrefix ${f})".mp3" -vcodec copy -acodec copy $output_path"/"$(FilePrefix ${f})$output_format -y
      else
        echo "encode and devode are not support yet!"
        ffmpeg -i $input_path$"/"$f  -i $input_path"/"$(FilePrefix ${f})".mp3" -vcodec copy -acodec copy $output_path"/"$(FilePrefix ${f})$output_format -y
      fi
      # ffmpeg -i ${f} -c copy `FilePrefix ${f}`.mp4
    fi
done
