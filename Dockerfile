FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    make \
    cmake \
    libpqxx-dev \
    libpq-dev \
    libssl-dev \
    libargon2-dev \
    libhiredis-dev \
    libboost-all-dev \
    libasio-dev \
    git 

RUN apt-get install -y g++-10 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 100

WORKDIR /app

RUN git clone https://github.com/Thalhammer/jwt-cpp.git /app/jwt-cpp

COPY src/ . 

RUN CXXFLAGS="-I/app/jwt-cpp/include"

RUN g++ -o app *.cpp \
    -I/app/jwt-cpp/include \
    -std=c++20 -lpqxx -largon2 -lssl -lcrypto -lhiredis -pthread -fexceptions

CMD ["./app"]
