FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1
ENV PORT=8080

# Install g++, valgrind, and python3
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    valgrind \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy project source code
COPY . /app

# Precompile complexity profiler binary
RUN g++ -std=c++17 -O2 analyzer.cc -o complexity_profiler && chmod +x complexity_profiler

# Default port
EXPOSE 8080

CMD ["python3", "frontend/server.py"]
