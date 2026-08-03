FROM ubuntu:24.04


RUN apt update && apt install -y \
    build-essential \
    ffmpeg \
    git \
    nodejs \
    npm

RUN npm install -g @anthropic-ai/claude-code