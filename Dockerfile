FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PORT=8080

# Install g++, valgrind, python3, and certificates
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    valgrind \
    python3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Official AWS Lambda Web Adapter (enables normal HTTP servers to run in Lambda)
COPY --from=public.ecr.aws/awsguru/aws-lambda-adapter:0.8.4 /lambda-adapter /opt/extensions/lambda-adapter

WORKDIR /app
COPY . /app

RUN g++ -std=c++17 -O2 analyzer.cc -o complexity_profiler && chmod +x complexity_profiler

EXPOSE 8080
CMD ["python3", "frontend/server.py"]
